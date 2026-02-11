#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace smgpc::di {

template <typename T, typename... Ts>
inline constexpr bool contains_type_v = (std::same_as<T, Ts> or ...);

template <typename... Ts>
struct are_unique : std::true_type {};

template <typename T, typename... Rest>
struct are_unique<T, Rest...> : std::bool_constant<not contains_type_v<T, Rest...> and are_unique<Rest...>::value> {};

template <typename... Services>
class ServiceContainer {
    static_assert(are_unique<Services...>::value, "ServiceContainer requires unique service interface types.");

public:
    ServiceContainer(): _instances({}) {}
    ServiceContainer(const ServiceContainer &) = delete;
    ServiceContainer &operator=(const ServiceContainer &) = delete;
    ServiceContainer(ServiceContainer &&) = default;
    ServiceContainer &operator=(ServiceContainer &&) = default;
    ~ServiceContainer() = default;

    template <typename Service>
    requires(contains_type_v<Service, Services...>)
    void register_instance(std::shared_ptr<Service> service) {
        if (not service) {
            throw std::invalid_argument("Cannot register a null service instance.");
        }

        std::get<std::shared_ptr<Service>>(_instances) = std::move(service);
    }

    template <typename Service, typename Implementation = Service, typename... Args>
    requires(contains_type_v<Service, Services...> and std::derived_from<Implementation, Service>)
    void register_type(Args &&...args) {
        register_instance<Service>(std::make_shared<Implementation>(std::forward<Args>(args)...));
    }

    template <typename Service, typename Factory> requires(contains_type_v<Service, Services...> and std::invocable<Factory, ServiceContainer &> and std::convertible_to<std::invoke_result_t<Factory, ServiceContainer &>, std::shared_ptr<Service>>)
    void register_factory(Factory &&factory) {
        auto produced = std::invoke(std::forward<Factory>(factory), *this);
        register_instance<Service>(std::shared_ptr<Service>(std::move(produced)));
    }

    template <typename Service> requires contains_type_v<Service, Services...>
    [[nodiscard]] bool has() const noexcept
    { return std::get<std::shared_ptr<Service>>(_instances) != nullptr; }

    template <typename Service> requires contains_type_v<Service, Services...>
    [[nodiscard]] const std::shared_ptr<Service> resolve_shared() const
    {
        const auto &instance = std::get<std::shared_ptr<Service>>(_instances);
        if (not instance) {
            throw std::logic_error("Requested service was not registered.");
        }
        return instance;
    }

    template <typename Service> requires contains_type_v<Service, Services...>
    [[nodiscard]] Service &resolve() const {
        return *resolve_shared<Service>();
    }

private:
    std::tuple<std::shared_ptr<Services>...> _instances;
};

}  // namespace smgpc::di
