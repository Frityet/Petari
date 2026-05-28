#pragma once

#include <algorithm>
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
        T *_service{};
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
            requires(std::same_as<Service, typename Scope::Type>)
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

        template <typename Dependency, bool IsScope, typename... Scopes>
        struct normalized_dependency_impl;

        template <typename Dependency, typename... Scopes>
        struct normalized_dependency_impl<Dependency, false, Scopes...> {
            static_assert(contains_interface_v<Dependency, Scopes...>, "Dependency is unknown to this ServiceProvider.");

            using type = service_scope_for_t<Dependency, Scopes...>;
            using service_type = typename type::Type;
        };

        template <typename Dependency, typename... Scopes>
        struct normalized_dependency_impl<Dependency, true, Scopes...> {
            using service_type = typename Dependency::Type;
            using declared_scope = service_scope_for_t<service_type, Scopes...>;
            static_assert(std::same_as<Dependency, declared_scope>,
                          "Dependency scope marker does not match the declaration in this ServiceProvider.");

            using type = Dependency;
        };

        template <typename Dependency, typename... Scopes>
        struct normalized_dependency
            : normalized_dependency_impl<std::remove_cv_t<Dependency>, is_scope_v<std::remove_cv_t<Dependency>>, Scopes...> {};

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

        [[nodiscard]] bool has_live_singleton() const noexcept {
            return _instance != nullptr || _reference_instance != nullptr;
        }

        void add_dependency(std::string dependency) {
            if (_state == State::Unregistered) {
                throw std::logic_error("Cannot add a dependency to an unregistered service.");
            }
            if (std::find(_dependencies.begin(), _dependencies.end(), dependency) == _dependencies.end()) {
                _dependencies.push_back(std::move(dependency));
            }
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

        void release_live_singleton() {
            _reference_instance = nullptr;
            _instance.reset();
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

        State _state{State::Unregistered};
        std::unique_ptr<Service> _instance{};
        std::unique_ptr<FactoryBase> _factory{};
        Service *_reference_instance = nullptr;
        std::unique_ptr<ReferenceFactoryBase> _reference_factory{};
        std::vector<std::string> _dependencies{};
    };

    template <typename... Scopes>
    class ServiceProvider {
    public:
        static_assert(are_unique_service_scopes_v<Scopes...>, "ServiceProvider requires unique service interfaces.");

        using Self = ServiceProvider<Scopes...>;

        struct DependencyGraphCounts {
            std::size_t registered = 0U;
            std::size_t singleton = 0U;
            std::size_t transient = 0U;
            std::size_t instance = 0U;
            std::size_t factory = 0U;
            std::size_t reference = 0U;
            std::size_t edges = 0U;
            std::size_t namespaces = 0U;
        };

        struct DependencyGraphService {
            std::string name{};
            std::string short_name{};
            std::string namespace_name{};
            std::string lifetime{};
            std::string registration_kind{};
            std::size_t dependency_count = 0U;
        };

        struct DependencyGraphEdge {
            std::string provider{};
            std::string consumer{};
        };

        struct DependencyGraphSnapshot {
            DependencyGraphCounts counts{};
            std::vector<std::string> namespaces{};
            std::vector<DependencyGraphService> services{};
            std::vector<DependencyGraphEdge> edges{};
        };

        ServiceProvider() = default;
        ServiceProvider(const ServiceProvider &) = delete;
        ServiceProvider(ServiceProvider &&) = default;
        ServiceProvider &operator=(const ServiceProvider &) = delete;
        ServiceProvider &operator=(ServiceProvider &&) = default;
        ~ServiceProvider() {
            release_singletons_in_dependency_order();
        }

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

            auto dependency_names = std::vector<std::string>{
                service_node_name<typename detail::normalized_dependency<Dependencies, Scopes...>::type::Type>()...};
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

            auto dependency_names = std::vector<std::string>{
                service_node_name<typename detail::normalized_dependency<Dependencies, Scopes...>::type::Type>()...};
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

        template <typename Service, typename Dependency>
        void add_service_dependency() {
            using ServiceType = typename detail::normalized_dependency<Service, Scopes...>::service_type;
            using DependencyType = typename detail::normalized_dependency<Dependency, Scopes...>::service_type;
            slot<ServiceType>().add_dependency(service_node_name<DependencyType>());
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
        struct SingletonReleaseEntry {
            std::string name{};
            std::vector<std::string> dependencies{};
            std::function<void()> release{};
            bool live = false;
        };

        template <typename Dependency>
        [[nodiscard]] constexpr typename Dependency::UsageType resolve_dependency() {
            if constexpr (detail::is_singleton_scope_v<Dependency>) {
                return typename Dependency::UsageType{slot<typename Dependency::Type>().get_singleton(*this)};
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

        void release_singletons_in_dependency_order() {
            auto entries = std::vector<SingletonReleaseEntry>{};
            std::apply(
                [&entries, this](auto &...slots) {
                    ((this->add_singleton_release_entry(slots, entries)), ...);
                },
                _slots);

            auto live_count = std::ranges::count_if(entries, [](const auto &entry) { return entry.live; });
            while (live_count > 0) {
                auto released_one = false;
                for (auto entry = entries.rbegin(); entry != entries.rend(); ++entry) {
                    if (!entry->live || has_live_consumer(entries, entry->name)) {
                        continue;
                    }

                    entry->release();
                    entry->live = false;
                    --live_count;
                    released_one = true;
                    break;
                }

                if (released_one) {
                    continue;
                }

                for (auto entry = entries.rbegin(); entry != entries.rend(); ++entry) {
                    if (!entry->live) {
                        continue;
                    }

                    entry->release();
                    entry->live = false;
                    --live_count;
                }
            }
        }

        static bool has_live_consumer(const std::vector<SingletonReleaseEntry> &entries, std::string_view provider_name) {
            return std::ranges::any_of(entries, [provider_name](const auto &entry) {
                return entry.live && std::ranges::find(entry.dependencies, provider_name) != entry.dependencies.end();
            });
        }

        template <typename Service, typename Provider>
        void add_singleton_release_entry(ServiceSlot<Service, Provider> &slot, std::vector<SingletonReleaseEntry> &entries) const {
            if (!slot.has_live_singleton()) {
                return;
            }

            entries.push_back(SingletonReleaseEntry{
                .name = service_node_name<Service>(),
                .dependencies = slot.dependencies(),
                .release = [&slot]() {
                    slot.release_live_singleton();
                },
                .live = true,
            });
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

        static std::string mermaid_label_escape(std::string_view value) {
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

        static std::string mermaid_identifier(std::string_view value) {
            std::string identifier;
            identifier.reserve(value.size());
            for (const auto character : value) {
                if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
                    (character >= '0' && character <= '9')) {
                    identifier += character;
                } else {
                    identifier += '_';
                }
            }
            if (identifier.empty() || (identifier.front() >= '0' && identifier.front() <= '9')) {
                identifier.insert(identifier.begin(), '_');
            }
            return identifier;
        }

        static std::string mermaid_node_id(std::string_view type_name) {
            return "svc_" + mermaid_identifier(type_name);
        }

        static std::string mermaid_group_id(std::string_view namespace_name) {
            return "ns_" + mermaid_identifier(namespace_name);
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

        static constexpr std::string_view mermaid_class_name(std::string_view registration_kind) {
            if (registration_kind == "factory") {
                return "factory";
            }
            if (registration_kind == "instance") {
                return "instance";
            }
            if (registration_kind == "reference") {
                return "reference";
            }
            return "service";
        }

    public:
        [[nodiscard]] DependencyGraphSnapshot dependency_graph_snapshot() const {
            auto snapshot = DependencyGraphSnapshot{};
            snapshot.counts = collect_dependency_graph_counts();
            snapshot.namespaces = registered_namespaces();
            snapshot.counts.namespaces = snapshot.namespaces.size();
            std::apply(
                [&snapshot, this](const auto &...slots) {
                    ((this->add_slot_dependency_graph_snapshot(slots, snapshot)), ...);
                },
                _slots);
            return snapshot;
        }

        [[nodiscard]] std::string dependencies_to_mermaid() const {
            const auto snapshot = dependency_graph_snapshot();
            const auto &counts = snapshot.counts;
            auto graph = std::string{"%% SMG PC DI dependency graph\n"};
            graph += "%% services=" + std::to_string(counts.registered) + " edges=" + std::to_string(counts.edges) +
                     " singletons=" + std::to_string(counts.singleton) + " transients=" + std::to_string(counts.transient) +
                     " instances=" + std::to_string(counts.instance) + " factories=" + std::to_string(counts.factory) +
                     " references=" + std::to_string(counts.reference) + " namespaces=" + std::to_string(counts.namespaces) + "\n";
            graph += "flowchart LR\n";
            graph += "    classDef service fill:#ffffff,stroke:#64748b,color:#0f172a;\n";
            graph += "    classDef instance fill:#f0fdf4,stroke:#16a34a,color:#0f172a;\n";
            graph += "    classDef factory fill:#ecfeff,stroke:#0891b2,color:#0f172a;\n";
            graph += "    classDef reference fill:#f5f3ff,stroke:#7c3aed,color:#0f172a;\n\n";
            for (const auto &namespace_name : snapshot.namespaces) {
                graph += "    subgraph " + mermaid_group_id(namespace_name) + "[\"" + mermaid_label_escape(namespace_name) + "\"]\n";
                for (const auto &service : snapshot.services) {
                    if (service.namespace_name != namespace_name) {
                        continue;
                    }

                    graph += "        " + mermaid_node_id(service.name) + "[\"" + mermaid_label_escape(service.short_name) + "<br/>" +
                             mermaid_label_escape(service.namespace_name) + "<br/>" + mermaid_label_escape(service.lifetime) + " " +
                             mermaid_label_escape(service.registration_kind) + "<br/>deps " +
                             std::to_string(service.dependency_count) + "\"]:::" +
                             std::string(mermaid_class_name(service.registration_kind)) + "\n";
                    graph += "        %% node: " + service.name + "\n";
                }
                graph += "    end\n\n";
            }
            for (const auto &edge : snapshot.edges) {
                graph += "    " + mermaid_node_id(edge.provider) + " --> " + mermaid_node_id(edge.consumer) + "\n";
                graph += "    %% edge: " + edge.provider + " -> " + edge.consumer + "\n";
            }
            return graph;
        }

    private:
        [[nodiscard]] DependencyGraphCounts
        collect_dependency_graph_counts() const {
            auto counts = DependencyGraphCounts{};
            std::apply(
                [&counts, this](const auto &...slots) {
                    ((this->add_slot_dependency_graph_counts(slots, counts)), ...);
                },
                _slots);
            return counts;
        }

        [[nodiscard]] std::vector<std::string> registered_namespaces() const {
            auto namespaces = std::vector<std::string>{};
            std::apply(
                [&namespaces, this](const auto &...slots) {
                    ((this->add_slot_namespace(slots, namespaces)), ...);
                },
                _slots);
            return namespaces;
        }

        template <typename Service, typename Provider>
        void add_slot_dependency_graph_counts(const ServiceSlot<Service, Provider> &slot, DependencyGraphCounts &counts) const {
            if (not slot.has()) {
                return;
            }

            using Scope = detail::service_scope_for_t<Service, Scopes...>;
            ++counts.registered;
            counts.edges += slot.dependencies().size();
            if constexpr (detail::is_singleton_scope_v<Scope>) {
                ++counts.singleton;
            } else {
                ++counts.transient;
            }

            const auto registration_kind = slot.registration_kind_label();
            if (registration_kind == "instance") {
                ++counts.instance;
            } else if (registration_kind == "factory") {
                ++counts.factory;
            } else if (registration_kind == "reference") {
                ++counts.reference;
            }
        }

        template <typename Service, typename Provider>
        void add_slot_namespace(const ServiceSlot<Service, Provider> &slot, std::vector<std::string> &namespaces) const {
            if (not slot.has()) {
                return;
            }

            auto namespace_name = service_namespace_name(service_node_name<Service>());
            if (std::find(namespaces.begin(), namespaces.end(), namespace_name) == namespaces.end()) {
                namespaces.push_back(std::move(namespace_name));
            }
        }

        template <typename Service, typename Provider>
        void add_slot_dependency_graph_snapshot(const ServiceSlot<Service, Provider> &slot, DependencyGraphSnapshot &snapshot) const {
            if (not slot.has()) {
                return;
            }

            using Scope = detail::service_scope_for_t<Service, Scopes...>;
            const auto service_name = service_node_name<Service>();
            const auto registration_kind = slot.registration_kind_label();
            const auto lifetime = detail::is_singleton_scope_v<Scope> ? std::string_view{"singleton"} : std::string_view{"transient"};

            snapshot.services.push_back(DependencyGraphService{
                .name = service_name,
                .short_name = service_short_name(service_name),
                .namespace_name = service_namespace_name(service_name),
                .lifetime = std::string(lifetime),
                .registration_kind = std::string(registration_kind),
                .dependency_count = slot.dependencies().size(),
            });
            for (const auto &dependency : slot.dependencies()) {
                snapshot.edges.push_back(DependencyGraphEdge{
                    .provider = dependency,
                    .consumer = service_name,
                });
            }
        }

    private:
        std::tuple<ServiceSlot<typename Scopes::Type, Self>...> _slots{};
    };

}  // namespace smgpc::di
