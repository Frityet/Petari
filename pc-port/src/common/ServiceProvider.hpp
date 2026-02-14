#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <functional>

namespace smgpc::di {

template <typename T>
class DependencyReference final {
public:
    explicit constexpr DependencyReference(T &service)
        : _service(std::addressof(service)) {
    }

    DependencyReference(const DependencyReference &) = delete;
    DependencyReference &operator=(const DependencyReference &) = delete;
    constexpr DependencyReference(DependencyReference &&) = default;
    DependencyReference &operator=(DependencyReference &&) = delete;

    [[nodiscard]] constexpr T &get() const {
        return *_service;
    }

    [[nodiscard]] constexpr operator T &() const {
        return *_service;
    }

    [[nodiscard]] constexpr T &operator*() const {
        return *_service;
    }

    [[nodiscard]] constexpr T *operator->() const {
        return _service;
    }

private:
    T *_service {};
};

using namespace std::string_literals;

class NullDependencyReference : public std::bad_optional_access {
public:
    const std::type_info &type_id;

    inline NullDependencyReference(const std::type_info &tid): type_id(tid), _msg("dependency '"s + type_id.name() + "' not found") {}

    const char *what() const noexcept override
    { return _msg.c_str(); }

private:
    std::string _msg;
};

template <typename T>
class OptionalDependencyReference final {
public:
    constexpr OptionalDependencyReference() = default;
    constexpr OptionalDependencyReference(std::nullptr_t) : _service(nullptr) {
    }

    constexpr OptionalDependencyReference(T *service) : _service(service) {
    }

    explicit constexpr OptionalDependencyReference(T &service)
        : _service(std::addressof(service)) {
    }

    constexpr OptionalDependencyReference(DependencyReference<T> &service)
        : _service(service.operator->()) {
    }

    constexpr OptionalDependencyReference(const DependencyReference<T> &service)
        : _service(service.operator->()) {
    }

    [[nodiscard]] constexpr bool has_value() const noexcept {
        return _service != nullptr;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return has_value();
    }

    [[nodiscard]] constexpr T *get() const {
        if (not has_value())
            throw NullDependencyReference(typeid(T));
        return _service;
    }

    [[nodiscard]] constexpr T &operator*() const {
        return *get();
    }

    [[nodiscard]] constexpr T *operator->() const {
        return get();
    }

private:
    T *_service {};
};

template <typename T>
struct SingletonService {
    using Type = T;
    using UsageType = DependencyReference<T>;
};

template <typename T>
struct TransientService {
    using Type = T;
    using UsageType = std::unique_ptr<T>;
};

namespace detail {

template <typename...>
struct always_false : std::false_type {};

template <typename Interface, typename... Scopes>
inline constexpr bool contains_interface_v = (std::same_as<Interface, typename Scopes::Type> or ...);

template <typename T>
struct scope_traits {
    static constexpr bool valid = false;
    static constexpr bool singleton = false;
};

template <typename T>
struct scope_traits<SingletonService<T>> {
    static constexpr bool valid = true;
    static constexpr bool singleton = true;
};

template <typename T>
struct scope_traits<TransientService<T>> {
    static constexpr bool valid = true;
    static constexpr bool singleton = false;
};

template <typename T>
inline constexpr bool is_scope_v = scope_traits<std::remove_cv_t<T>>::valid;

template <typename T>
inline constexpr bool is_singleton_scope_v = scope_traits<std::remove_cv_t<T>>::valid and scope_traits<std::remove_cv_t<T>>::singleton;

template <typename T>
inline constexpr bool is_transient_scope_v = scope_traits<std::remove_cv_t<T>>::valid and not is_singleton_scope_v<T>;

template <typename... Scopes>
struct unique_service_scopes : std::true_type {};

template <typename Scope, typename... Rest>
struct unique_service_scopes<Scope, Rest...> : std::bool_constant<not contains_interface_v<typename Scope::Type, Rest...> and unique_service_scopes<Rest...>::value> {};

template <typename Service, typename... Scopes>
struct service_scope_for;

template <typename Service>
struct service_scope_for<Service> {
    static_assert(always_false<Service>::value, "Service type is not registered in ServiceProvider.");
};

template <typename Service, typename Scope, typename... Rest>
    requires (std::same_as<Service, typename Scope::Type>)
struct service_scope_for<Service, Scope, Rest...> {
    using type = Scope;
};

template <typename Service, typename Scope, typename... Rest>
struct service_scope_for<Service, Scope, Rest...> {
    using type = typename service_scope_for<Service, Rest...>::type;
};

template <typename Service, typename... Scopes>
using service_scope_for_t = typename service_scope_for<Service, Scopes...>::type;

template <typename Service, typename... Scopes>
struct service_index;

template <typename Service>
struct service_index<Service> {
    static constexpr std::size_t value = static_cast<std::size_t>(-1);
};

template <typename Service, typename Scope, typename... Rest>
struct service_index<Service, Scope, Rest...> {
    static constexpr std::size_t next = service_index<Service, Rest...>::value;
    static constexpr std::size_t value = std::same_as<Service, typename Scope::Type> ? 0U : (next == static_cast<std::size_t>(-1) ? static_cast<std::size_t>(-1) : 1U + next);
};

template <typename Service, typename... Scopes>
static consteval std::size_t service_index_v() {
    const auto index = service_index<Service, Scopes...>::value;
    static_assert(index != static_cast<std::size_t>(-1), "Service type is not registered in ServiceProvider.");
    return index;
}

template <typename Dependency, typename... Scopes>
struct normalized_dependency {
    using raw_dependency = std::remove_cv_t<Dependency>;
    static_assert(
        is_scope_v<raw_dependency> or contains_interface_v<raw_dependency, Scopes...>,
        "Dependency is unknown to this ServiceProvider.");

    using declared_scope = std::conditional_t<
        is_scope_v<raw_dependency>,
        raw_dependency,
        service_scope_for_t<raw_dependency, Scopes...>>;

    using service_type = typename declared_scope::Type;
    using type = std::conditional_t<
        is_scope_v<raw_dependency>,
        raw_dependency,
        service_scope_for_t<raw_dependency, Scopes...>>;
    static_assert(
        not is_scope_v<raw_dependency> or std::same_as<raw_dependency, service_scope_for_t<service_type, Scopes...>>,
        "Dependency scope marker does not match the declaration in this ServiceProvider.");
};

}  // namespace detail

template <typename... Scopes>
inline constexpr bool are_unique_service_scopes_v = detail::unique_service_scopes<Scopes...>::value;

template <typename Service, typename Provider>
class ServiceSlot final {
public:
    using Factory = std::function<std::unique_ptr<Service>(Provider &)>;

    [[nodiscard]] constexpr bool has() const noexcept {
        return _state != State::Unregistered;
    }

    template <typename ConcreteService>
    void register_instance(std::unique_ptr<ConcreteService> service) {
        if (_state != State::Unregistered) {
            throw std::logic_error("Service has already been registered.");
        }
        if (not service) {
            throw std::invalid_argument("Cannot register a null service instance.");
        }

        _instance = std::unique_ptr<Service>(std::move(service));
        _state = State::Instance;
    }

    template <typename FactoryFn>
    void register_factory(FactoryFn &&factory) {
        if (_state != State::Unregistered) {
            throw std::logic_error("Service has already been registered.");
        }

        _factory = std::make_unique<FactoryAdapter<std::remove_cv_t<FactoryFn>>>(std::forward<FactoryFn>(factory));
        _state = State::Factory;
    }

    [[nodiscard]] Service &get_singleton(Provider &provider) {
        if (_state == State::Unregistered) {
            throw std::logic_error("Service has not been registered.");
        }
        if (_state == State::Factory and not _instance) {
            _instance = _factory->create(provider);
            if (not _instance) {
                throw std::logic_error("Factory produced a null singleton service.");
            }
        }
        if (not _instance) {
            throw std::logic_error("Singleton service was not constructible from a factory.");
        }
        return *_instance;
    }

    [[nodiscard]] std::unique_ptr<Service> get_transient(Provider &provider) {
        if (_state == State::Unregistered) {
            throw std::logic_error("Service has not been registered.");
        }
        if (_state == State::Instance) {
            throw std::logic_error("Cannot resolve a transient service from a singleton registration.");
        }

        auto product = _factory->create(provider);
        if (not product) {
            throw std::logic_error("Factory produced a null transient service.");
        }
        return product;
    }

private:
    struct FactoryBase {
        virtual ~FactoryBase() = default;
        virtual std::unique_ptr<Service> create(Provider &) = 0;
    };

    template <typename Fn>
    class FactoryAdapter final : public FactoryBase {
    public:
        explicit FactoryAdapter(Fn factory)
            : _factory(std::move(factory)) {
        }

        [[nodiscard]] std::unique_ptr<Service> create(Provider &provider) override {
            return _factory(provider);
        }

    private:
        Fn _factory;
    };

    enum class State {
        Unregistered,
        Instance,
        Factory,
    };

    State _state {State::Unregistered};
    std::unique_ptr<Service> _instance {};
    std::unique_ptr<FactoryBase> _factory {};
};

template <typename... Scopes>
class ServiceProvider {
public:
    static_assert(are_unique_service_scopes_v<Scopes...>, "ServiceProvider requires unique service interfaces.");

    using Self = ServiceProvider<Scopes...>;

    ServiceProvider() = default;
    ServiceProvider(const ServiceProvider &) = delete;
    ServiceProvider(ServiceProvider &&) = default;
    ServiceProvider &operator=(const ServiceProvider &) = delete;
    ServiceProvider &operator=(ServiceProvider &&) = default;
    ~ServiceProvider() = default;

    template <typename Service>
    [[nodiscard]] bool has() const {
        return slot<Service>().has();
    }

    template <typename Scope, typename ServiceImplementation>
    void register_service(std::unique_ptr<ServiceImplementation> service) {
        using CleanScope = std::remove_cv_t<Scope>;
        static_assert(detail::is_scope_v<CleanScope>, "register_service requires a scope marker.");
        static_assert(detail::is_singleton_scope_v<CleanScope>, "Only singleton services can be registered from a pre-constructed pointer.");
        using DeclaredScope = detail::service_scope_for_t<typename CleanScope::Type, Scopes...>;
        static_assert(std::same_as<CleanScope, DeclaredScope>, "Scope does not match declaration in this ServiceProvider.");
        static_assert(std::convertible_to<std::unique_ptr<ServiceImplementation>, std::unique_ptr<typename CleanScope::Type>>,
            "Pre-constructed singleton is not convertibly assignable to declared interface type.");

        slot<typename CleanScope::Type>().register_instance(std::unique_ptr<typename CleanScope::Type>(std::move(service)));
    }

    template <typename Scope, typename... Dependencies, typename Factory>
    void register_service(Factory &&factory) {
        using CleanScope = std::remove_cv_t<Scope>;
        static_assert(detail::is_scope_v<CleanScope>, "register_service requires a scope marker.");
        using DeclaredScope = detail::service_scope_for_t<typename CleanScope::Type, Scopes...>;
        static_assert(std::same_as<CleanScope, DeclaredScope>, "Scope does not match declaration in this ServiceProvider.");

        using ServiceType = typename CleanScope::Type;
        using Product = std::invoke_result_t<Factory, typename detail::normalized_dependency<Dependencies, Scopes...>::type::UsageType...>;

        static_assert(
            std::invocable<Factory, typename detail::normalized_dependency<Dependencies, Scopes...>::type::UsageType...>,
            "Factory signature does not match resolved dependency usage types.");
        static_assert(std::convertible_to<Product, std::unique_ptr<ServiceType>>, "Factory must return a value convertible to std::unique_ptr<ServiceType>.");

        slot<ServiceType>().register_factory([factory = std::forward<Factory>(factory)](Self &provider) mutable -> std::unique_ptr<ServiceType> {
            return std::apply(
                [&factory](auto &&...deps) mutable {
                    return std::invoke(factory, std::move(deps)...);
                },
                provider.resolve_dependencies<typename detail::normalized_dependency<Dependencies, Scopes...>::type...>());
        });
    }

    template <typename Service>
    [[nodiscard]] decltype(auto) get() {
        using Scope = detail::service_scope_for_t<Service, Scopes...>;
        if constexpr (detail::is_singleton_scope_v<Scope>) {
            return slot<Service>().get_singleton(*this);
        } else {
            return slot<Service>().get_transient(*this);
        }
    }

private:
    template <typename Dependency>
    [[nodiscard]] constexpr typename Dependency::UsageType resolve_dependency() {
        if constexpr (detail::is_singleton_scope_v<Dependency>) {
            return typename Dependency::UsageType { slot<typename Dependency::Type>().get_singleton(*this) };
        } else {
            return slot<typename Dependency::Type>().get_transient(*this);
        }
    }

    template <typename... Dependencies>
    [[nodiscard]] constexpr std::tuple<typename Dependencies::UsageType...> resolve_dependencies() {
        return std::tuple<typename Dependencies::UsageType...>(resolve_dependency<Dependencies>()...);
    }

    template <typename Service>
    [[nodiscard]] auto &slot() {
        constexpr auto service_index = detail::service_index_v<Service, Scopes...>();
        return std::get<service_index>(_slots);
    }

    template <typename Service>
    [[nodiscard]] const auto &slot() const {
        constexpr auto service_index = detail::service_index_v<Service, Scopes...>();
        return std::get<service_index>(_slots);
    }

    std::tuple<ServiceSlot<typename Scopes::Type, Self>...> _slots {};
};

}  // namespace smgpc::di
