#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

#if defined(__GNUG__)
#include <cstdlib>
#include <cxxabi.h>
#endif

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

    [[nodiscard]] std::string_view registration_kind_label() const noexcept {
        switch (_state) {
        case State::Instance:
            return "instance";
        case State::Factory:
            return "factory";
        case State::ReferenceFactory:
            return "reference";
        case State::Unregistered:
            break;
        }
        return "unregistered";
    }

    [[nodiscard]] const std::vector<std::string> &dependencies() const noexcept {
        return _dependencies;
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
        _reference_instance = nullptr;
        _reference_factory.reset();
        _dependencies.clear();
        _state = State::Instance;
    }

    template <typename FactoryFn>
    void register_factory(FactoryFn &&factory, std::vector<std::string> dependencies = {}) {
        if (_state != State::Unregistered) {
            throw std::logic_error("Service has already been registered.");
        }

        _factory = std::make_unique<FactoryAdapter<std::remove_cv_t<FactoryFn>>>(std::forward<FactoryFn>(factory));
        _reference_instance = nullptr;
        _reference_factory.reset();
        _dependencies = std::move(dependencies);
        _state = State::Factory;
    }

    template <typename FactoryFn>
    void register_reference_factory(FactoryFn &&factory, std::vector<std::string> dependencies = {}) {
        if (_state != State::Unregistered) {
            throw std::logic_error("Service has already been registered.");
        }

        _reference_factory = std::make_unique<ReferenceFactoryAdapter<std::remove_cv_t<FactoryFn>>>(std::forward<FactoryFn>(factory));
        _dependencies = std::move(dependencies);
        _state = State::ReferenceFactory;
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
        if (_state == State::ReferenceFactory and _reference_instance == nullptr) {
            _reference_instance = std::addressof(_reference_factory->get(provider));
            if (_reference_instance == nullptr) {
                throw std::logic_error("Reference factory produced a null singleton service.");
            }
        }
        if (_state == State::ReferenceFactory) {
            return *_reference_instance;
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
        if (_state == State::ReferenceFactory) {
            throw std::logic_error("Cannot resolve a transient service from a borrowed singleton registration.");
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

    struct ReferenceFactoryBase {
        virtual ~ReferenceFactoryBase() = default;
        virtual Service &get(Provider &) = 0;
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

    template <typename Fn>
    class ReferenceFactoryAdapter final : public ReferenceFactoryBase {
    public:
        explicit ReferenceFactoryAdapter(Fn factory)
            : _factory(std::move(factory)) {
        }

        [[nodiscard]] Service &get(Provider &provider) override {
            return _factory(provider);
        }

    private:
        Fn _factory;
    };

    enum class State {
        Unregistered,
        Instance,
        Factory,
        ReferenceFactory,
    };

    State _state {State::Unregistered};
    std::unique_ptr<Service> _instance {};
    std::unique_ptr<FactoryBase> _factory {};
    Service *_reference_instance = nullptr;
    std::unique_ptr<ReferenceFactoryBase> _reference_factory {};
    std::vector<std::string> _dependencies {};
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

        auto dependency_names = std::vector<std::string> {
            service_node_name<typename detail::normalized_dependency<Dependencies, Scopes...>::type::Type>()...
        };
        slot<ServiceType>().register_factory(
            [factory = std::forward<Factory>(factory)](Self &provider) mutable -> std::unique_ptr<ServiceType> {
                return std::apply(
                    [&factory](auto &&...deps) mutable {
                        return std::invoke(factory, std::move(deps)...);
                    },
                    provider.resolve_dependencies<typename detail::normalized_dependency<Dependencies, Scopes...>::type...>());
            },
            std::move(dependency_names));
    }

    template <typename Scope, typename... Dependencies, typename Factory>
    void register_service_reference(Factory &&factory) {
        using CleanScope = std::remove_cv_t<Scope>;
        static_assert(detail::is_scope_v<CleanScope>, "register_service_reference requires a scope marker.");
        static_assert(detail::is_singleton_scope_v<CleanScope>, "Borrowed services must be registered as singletons.");
        using DeclaredScope = detail::service_scope_for_t<typename CleanScope::Type, Scopes...>;
        static_assert(std::same_as<CleanScope, DeclaredScope>, "Scope does not match declaration in this ServiceProvider.");

        using ServiceType = typename CleanScope::Type;
        using Product = std::invoke_result_t<Factory, typename detail::normalized_dependency<Dependencies, Scopes...>::type::UsageType...>;

        static_assert(
            std::invocable<Factory, typename detail::normalized_dependency<Dependencies, Scopes...>::type::UsageType...>,
            "Reference factory signature does not match resolved dependency usage types.");
        static_assert(std::is_lvalue_reference_v<Product> && std::convertible_to<Product, ServiceType &>,
            "Reference factory must return a non-owning lvalue reference to ServiceType.");

        auto dependency_names = std::vector<std::string> {
            service_node_name<typename detail::normalized_dependency<Dependencies, Scopes...>::type::Type>()...
        };
        slot<ServiceType>().register_reference_factory(
            [factory = std::forward<Factory>(factory)](Self &provider) mutable -> ServiceType & {
                return std::apply(
                    [&factory](auto &&...deps) mutable -> ServiceType & {
                        return std::invoke(factory, std::move(deps)...);
                    },
                    provider.resolve_dependencies<typename detail::normalized_dependency<Dependencies, Scopes...>::type...>());
            },
            std::move(dependency_names));
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

    static std::string demangled_type_name(const std::type_info &type_info) {
#if defined(__GNUG__)
        int status = 0;
        const auto *demangled = abi::__cxa_demangle(type_info.name(), nullptr, nullptr, &status);
        std::string result = (status == 0 && demangled != nullptr) ? demangled : type_info.name();
        std::free(const_cast<char *>(demangled));
        return result;
#else
        return type_info.name();
#endif
    }

    template <typename Service>
    static std::string service_node_name() {
        return demangled_type_name(typeid(Service));
    }

    static std::string dot_escape(std::string_view value) {
        std::string escaped;
        escaped.reserve(value.size());
        for (const auto character : value) {
            switch (character) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            default:
                escaped += character;
                break;
            }
        }
        return escaped;
    }

    static std::string html_escape(std::string_view value) {
        std::string escaped;
        escaped.reserve(value.size());
        for (const auto character : value) {
            switch (character) {
            case '&':
                escaped += "&amp;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            default:
                escaped += character;
                break;
            }
        }
        return escaped;
    }

    static std::string without_project_prefix(std::string_view type_name) {
        constexpr auto prefix = std::string_view{"smgpc::"};
        if (type_name.starts_with(prefix)) {
            type_name.remove_prefix(prefix.size());
        }
        return std::string(type_name);
    }

    static std::string service_short_name(std::string_view type_name) {
        const auto template_start = type_name.find('<');
        const auto search_end = template_start == std::string_view::npos ? type_name.size() : template_start;
        const auto namespace_end = type_name.rfind("::", search_end);
        if (namespace_end == std::string_view::npos) {
            return without_project_prefix(type_name);
        }
        return std::string(type_name.substr(namespace_end + 2U));
    }

    static std::string service_namespace_name(std::string_view type_name) {
        const auto template_start = type_name.find('<');
        const auto search_end = template_start == std::string_view::npos ? type_name.size() : template_start;
        const auto namespace_end = type_name.rfind("::", search_end);
        if (namespace_end == std::string_view::npos) {
            return "global";
        }
        return without_project_prefix(type_name.substr(0U, namespace_end));
    }

    template <typename Scope>
    static constexpr std::string_view scope_border_color() {
        if constexpr (detail::is_singleton_scope_v<Scope>) {
            return "#38bdf8";
        } else {
            return "#f59e0b";
        }
    }

    static constexpr std::string_view registration_background(std::string_view registration_kind) {
        if (registration_kind == "factory") {
            return "#164e63";
        }
        if (registration_kind == "instance") {
            return "#166534";
        }
        if (registration_kind == "reference") {
            return "#6d28d9";
        }
        return "#334155";
    }

public:
    [[nodiscard]] std::string dependencies_to_graphviz() const {
        auto graph = std::string{
            "digraph ServiceGraph {\n"
            "    graph [rankdir=LR, bgcolor=\"#0f172a\", pad=\"0.35\", nodesep=\"0.55\", ranksep=\"0.85\", splines=ortho, outputorder=edgesfirst, fontname=\"Inter\"];\n"
            "    node [shape=plain, fontname=\"Inter\"];\n"
            "    edge [color=\"#a78bfa\", penwidth=2, arrowsize=0.8, fontname=\"Inter\", fontcolor=\"#dbeafe\"];\n"
            "    labelloc=\"t\";\n"
            "    label=<\n"
            "        <FONT POINT-SIZE=\"26\" COLOR=\"#e5e7eb\"><B>SMG PC Dependency Graph</B></FONT>\n"
            "    >;\n\n"
            "    \"__legend\" [label=<\n"
            "        <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"7\" COLOR=\"#475569\">\n"
            "            <TR><TD BGCOLOR=\"#111827\" COLSPAN=\"2\"><FONT COLOR=\"#f8fafc\"><B>Legend</B></FONT></TD></TR>\n"
            "            <TR><TD BGCOLOR=\"#166534\"><FONT COLOR=\"#ecfeff\">instance</FONT></TD><TD BGCOLOR=\"#1e293b\"><FONT COLOR=\"#cbd5e1\">prebuilt singleton</FONT></TD></TR>\n"
            "            <TR><TD BGCOLOR=\"#164e63\"><FONT COLOR=\"#ecfeff\">factory</FONT></TD><TD BGCOLOR=\"#1e293b\"><FONT COLOR=\"#cbd5e1\">DI-created singleton</FONT></TD></TR>\n"
            "            <TR><TD BGCOLOR=\"#6d28d9\"><FONT COLOR=\"#ecfeff\">reference</FONT></TD><TD BGCOLOR=\"#1e293b\"><FONT COLOR=\"#cbd5e1\">borrowed runtime service</FONT></TD></TR>\n"
            "        </TABLE>\n"
            "    >];\n"
            "    { rank=sink; \"__legend\"; }\n\n"
        };
        std::apply(
            [&graph, this](const auto &...slots) {
                ((graph += this->slot_node_to_graphviz(slots)), ...);
            },
            _slots);
        graph += "\n";
        std::apply(
            [&graph, this](const auto &...slots) {
                ((graph += this->slot_edges_to_graphviz(slots)), ...);
            },
            _slots);
        graph += "}\n";
        return graph;
    }

private:
    template <typename Service, typename Provider>
    [[nodiscard]] std::string slot_node_to_graphviz(const ServiceSlot<Service, Provider> &slot) const {
        if (not slot.has()) {
            return {};
        }

        using Scope = detail::service_scope_for_t<Service, Scopes...>;
        const auto service_name = service_node_name<Service>();
        const auto registration_kind = slot.registration_kind_label();
        const auto lifetime = detail::is_singleton_scope_v<Scope> ? std::string_view{"singleton"} : std::string_view{"transient"};

        auto graph = std::string{};
        graph += "    \"" + dot_escape(service_name) + "\" [label=<\n";
        graph += "        <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"8\" COLOR=\"" +
                 std::string(scope_border_color<Scope>()) + "\">\n";
        graph += "            <TR><TD BGCOLOR=\"#111827\"><FONT POINT-SIZE=\"16\" COLOR=\"#f8fafc\"><B>" +
                 html_escape(service_short_name(service_name)) + "</B></FONT></TD></TR>\n";
        graph += "            <TR><TD BGCOLOR=\"#1e293b\"><FONT POINT-SIZE=\"10\" COLOR=\"#93c5fd\">" +
                 html_escape(service_namespace_name(service_name)) + "</FONT></TD></TR>\n";
        graph += "            <TR><TD BGCOLOR=\"" + std::string(registration_background(registration_kind)) +
                 "\"><FONT POINT-SIZE=\"10\" COLOR=\"#ecfeff\">" + html_escape(lifetime) + " " +
                 html_escape(registration_kind) + "</FONT></TD></TR>\n";
        graph += "        </TABLE>\n";
        graph += "    >];\n";
        return graph;
    }

    template <typename Service, typename Provider>
    [[nodiscard]] std::string slot_edges_to_graphviz(const ServiceSlot<Service, Provider> &slot) const {
        if (not slot.has()) {
            return {};
        }

        const auto service_name = service_node_name<Service>();
        auto graph = std::string{};
        for (const auto &dependency : slot.dependencies()) {
            graph += "    \"" + dot_escape(dependency) + "\" -> \"" + dot_escape(service_name) +
                     "\" [color=\"#22d3ee\", tooltip=\"" + dot_escape(dependency + " -> " + service_name) + "\"];\n";
        }
        return graph;
    }

private:
    std::tuple<ServiceSlot<typename Scopes::Type, Self>...> _slots {};
};

}  // namespace smgpc::di
