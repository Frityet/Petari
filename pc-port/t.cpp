#include <cstdio>
#include <string_view>
#include <string>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <variant>
#include <optional>
#include <functional>
#include <type_traits>

class ConcreteService final {
};

template<template<typename, typename...> class T>
concept is_scope = requires {
    typename T<std::function<std::unique_ptr<ConcreteService>()>>::Dependencies;
    typename T<std::function<std::unique_ptr<ConcreteService>()>>::Service;
};

template <typename T>
struct function_traits : function_traits<decltype(&T::operator())> {};

template<typename TRet, typename ...TArgs>
struct function_traits<TRet(TArgs...)> {
    using TReturn = TRet;
    using TArguments = std::tuple<TArgs...>;
    static constexpr auto parametre_count = sizeof...(TArgs);
};

template<typename TRet, typename ...TArgs>
struct function_traits<std::function<TRet(TArgs...)>> {
    using TReturn = TRet;
    using TArguments = std::tuple<TArgs...>;
    static constexpr auto parametre_count = sizeof...(TArgs);
};


template<typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> : function_traits<R(Args...)> {};
template<typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> : function_traits<R(Args...)> {};


template<typename T>
struct SingletonService {
    using Type = T;
    using UsageType = T &;
    template<typename TFn, typename ...TServices>
    struct Container {
        using Definition = SingletonService<T>;
        using Traits = function_traits<TFn>;
        using Dependencies = std::tuple<TServices...>;
        using Service = T;
        using Builder = TFn;
        std::variant<Builder, std::unique_ptr<Service>> data;

        constexpr explicit Container(Builder fn) : data(std::move(fn)) {}

        constexpr Container(Container &&) = default;
        constexpr Container &operator=(Container &&) = default;

        constexpr UsageType operator()(typename TServices::UsageType ...args)
        {
            if (data.index() == 0) {
                auto fn = std::move(std::get<0>(data));
                data.template emplace<1>(std::invoke(fn, std::forward<typename TServices::UsageType>(args)...));
            }
            return *std::get<1>(data).get();
        }
    };

};

template<typename T>
struct TransientService {
    using Type = T;
    using UsageType = std::unique_ptr<T>;
    template<typename TFn, typename ...TServices>
    struct Container {
        using Definition = TransientService<T>;
        using Traits = function_traits<TFn>;
        using Dependencies = std::tuple<TServices...>;
        using Service = T;
        using Builder = TFn;
        Builder builder;

        constexpr explicit Container(Builder fn) : builder(std::move(fn)) {}

        constexpr UsageType operator()(typename TServices::UsageType ...args)
        {
            return builder(std::forward<typename TServices::UsageType>(args)...);
        }
    };
    static_assert(is_scope<Container>);

};

template<typename T>
concept IsTuple = requires { typename std::tuple_size<std::remove_cvref_t<T>>::type; };

template<typename ...TBuilders>
class ServiceProvider {
    std::tuple<TBuilders...> _storage;

    template <typename T>
    struct Resolve {
        template<typename U> static auto test(int) -> typename U::UsageType;
        template<typename U> static auto test(...) -> void;
        static constexpr bool IsExplicit = !std::is_void_v<decltype(test<T>(0))>;

        static constexpr size_t find_index() {
            constexpr size_t idx = []<size_t... Is>(std::index_sequence<Is...>) {
                size_t found = -1;
                ((std::is_same_v<typename TBuilders::Service, T> ? (found = Is) : 0), ...);
                return found;
            }(std::make_index_sequence<sizeof...(TBuilders)>{});
            return idx;
        }

        // The actual resolved type
        using Type = std::conditional_t<
            IsExplicit,
            T, // Already a wrapper
            typename std::tuple_element_t<find_index(), std::tuple<TBuilders...>>::Definition
        >;
    };
public:
    constexpr explicit ServiceProvider(std::tuple<TBuilders...> &&t): _storage(std::move(t)) {}

    using Self = ServiceProvider<TBuilders...>;
    constexpr ServiceProvider() = default;

    template<typename TService, typename ...TDeps>
    constexpr auto register_service(this auto &&self, auto &&fn)
    {
        // 1. Resolve raw types (IWriter) to Wrappers (SingletonService<IWriter>)
        using ResolvedDepsTuple = std::tuple<typename Resolve<TDeps>::Type...>;

        // 2. Helper lambda to unpack the resolved types into the internal registration logic
        return [&]<typename ...ResolvedDeps>(std::type_identity<std::tuple<ResolvedDeps...>>) {

            // 3. Create factory using ResolvedDeps::UsageType (e.g., IWriter&)
            const auto factory = [fn = std::forward<decltype(fn)>(fn)](typename ResolvedDeps::UsageType ...deps)
                constexpr -> std::unique_ptr<typename TService::Type> {
                return std::make_unique<std::remove_cvref_t<decltype(fn(std::forward<typename ResolvedDeps::UsageType>(deps)...))>>(
                    fn(std::forward<typename ResolvedDeps::UsageType>(deps)...)
                );
            };

            // 4. Instantiate the Container with the Resolved Types
            using NewBuilder = typename TService::template Container<decltype(factory), ResolvedDeps...>;
            auto new_storage = std::tuple_cat(std::move(self._storage), std::make_tuple(NewBuilder(std::move(factory))));

            return ServiceProvider<TBuilders..., NewBuilder>(std::move(new_storage));

        }(std::type_identity<ResolvedDepsTuple>{});
    }

    template<typename T>
    constexpr decltype(auto) fetch_deps(this auto &self)
    {
        if constexpr (IsTuple<T>) {
            return [&]<typename ...Args>(std::type_identity<std::tuple<Args...>>) {
                return std::forward_as_tuple(self.template fetch_deps<Args>()...);
            }(std::type_identity<T>{});
        } else {
            // T is now guaranteed to be a Wrapper (e.g., SingletonService<IWriter>)
            // We extract ::Type (IWriter) to find the container
            return self.template get<typename std::remove_cvref_t<T>::Type>();
        }
    }

    template<typename TService>
    constexpr decltype(auto) get(this auto &self)
    {
        constexpr size_t Index = []<size_t... Is>(std::index_sequence<Is...>) {
            size_t found = -1;
            ((std::is_same_v<typename TBuilders::Service, TService> ? (found = Is) : 0), ...);
            return found;
        }(std::make_index_sequence<sizeof...(TBuilders)>{});

        static_assert(Index != -1, "Service not registered");

        auto &container = std::get<Index>(self._storage);
        return std::apply(container,
            self.template fetch_deps<typename std::remove_cvref_t<decltype(container)>::Dependencies>()
        );
    }
};

//---//

class IWriter {
public:
    IWriter() = default;
    IWriter(IWriter &&) = default;
    virtual ~IWriter() = 0;

    virtual void write(std::string_view data) = 0;
};
IWriter::~IWriter() {}

class IGreeter {
public:
    virtual ~IGreeter() = 0;
    virtual void greet() = 0;
};
IGreeter::~IGreeter() {}

class IGreeter2 {
public:
    virtual ~IGreeter2() = 0;
    virtual void greet() = 0;
};
IGreeter2::~IGreeter2() {}

//---//

class StandardOutputWriter final : public IWriter {
public:
    StandardOutputWriter() = default;
    StandardOutputWriter(StandardOutputWriter &&) = default;

    void write(std::string_view data)
    {
        std::puts(data.data());
    }

    ~StandardOutputWriter() = default;
};

class DefaultGreeter final : public IGreeter {
public:
    DefaultGreeter(IWriter &writer, std::string_view to): _writer(writer), _to(to) {}
    DefaultGreeter(DefaultGreeter &&other): _writer(other._writer), _to(std::move(other._to))
    {}

    void greet()
    {
        _writer.write("Hello, " + _to);
    }

    ~DefaultGreeter() = default;
private:
    IWriter &_writer;
    std::string _to;
};

class OtherGreeter final : public IGreeter2 {
public:
    OtherGreeter(IWriter &writer, std::string_view to): _writer(writer), _to(to) {}
    OtherGreeter(OtherGreeter &&other): _writer(other._writer), _to(std::move(other._to))
    {}

    void greet()
    {
        _writer.write("Goodbye, " + _to);
    }

    ~OtherGreeter() = default;
private:
    IWriter &_writer;
    std::string _to;
};


int main()
{
    auto s = ServiceProvider()
        .register_service<SingletonService<IWriter>>([]() { return StandardOutputWriter(); })
        .register_service<TransientService<IGreeter>, IWriter>([](IWriter &writer) {
            return DefaultGreeter(writer, "Frityet");
        })
        .register_service<SingletonService<IGreeter2>, IGreeter, IWriter>([](std::unique_ptr<IGreeter> greeter, IWriter &writer){
            greeter->greet();
            return OtherGreeter(writer, "Frityet");
        });

    s.get<IGreeter>()->greet();
    s.get<IGreeter2>().greet();
}
