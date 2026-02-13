#include "ServiceProvider.hpp"
#include "tests/TestHarness.hpp"

#include <memory>
#include <string>
#include <type_traits>

namespace {

struct IPrimaryService {
    virtual ~IPrimaryService() = default;
    [[nodiscard]] virtual int value() const = 0;
};

struct ISecondaryService {
    virtual ~ISecondaryService() = default;
    [[nodiscard]] virtual std::string describe() const = 0;
};

struct IConsumerService {
    virtual ~IConsumerService() = default;
    [[nodiscard]] virtual const IPrimaryService &singleton_reference() const = 0;
};

class PrimaryService final : public IPrimaryService {
public:
    explicit PrimaryService(int number)
        : _number(number) {
    }

    [[nodiscard]] int value() const override {
        return _number;
    }

private:
    int _number {};
};

class SecondaryService final : public ISecondaryService {
public:
    explicit SecondaryService(std::string message)
        : _message(std::move(message)) {
    }

    [[nodiscard]] std::string describe() const override {
        return _message;
    }

private:
    std::string _message {};
};

class ConsumerService final : public IConsumerService {
public:
    explicit ConsumerService(smgpc::di::DependencyReference<IPrimaryService> primary)
        : _primary(std::move(primary)) {
    }

    [[nodiscard]] const IPrimaryService &singleton_reference() const override {
        return _primary.get();
    }

private:
    smgpc::di::DependencyReference<IPrimaryService> _primary;
};

}  // namespace

using Graph = smgpc::di::ServiceProvider<smgpc::di::SingletonService<IPrimaryService>, smgpc::di::TransientService<ISecondaryService>, smgpc::di::TransientService<IConsumerService>>;

static_assert(std::same_as<smgpc::di::SingletonService<IPrimaryService>::UsageType, smgpc::di::DependencyReference<IPrimaryService>>,
    "Singleton services should use DependencyReference for dependency injection.");
static_assert(std::same_as<smgpc::di::TransientService<ISecondaryService>::UsageType, std::unique_ptr<ISecondaryService>>,
    "Transient services should use std::unique_ptr for dependency injection.");

static_assert(!smgpc::di::are_unique_service_scopes_v<smgpc::di::SingletonService<IPrimaryService>, smgpc::di::SingletonService<IPrimaryService>>,
    "Duplicate singleton declarations should not be accepted by ServiceProvider.");

$test("ServiceProvider::has/resolve singleton returns same instance") {
    Graph graph {};

    graph.register_service<smgpc::di::SingletonService<IPrimaryService>>([]() {
        return std::make_unique<PrimaryService>(42);
    });

    const auto &first = graph.get<IPrimaryService>();
    const auto &second = graph.get<IPrimaryService>();

    $pc_port_require(&first == &second);
    $pc_port_require_eq(first.value(), 42);
}

$test("ServiceProvider::get transient resolves distinct instances") {
    Graph graph {};

    graph.register_service<smgpc::di::SingletonService<IPrimaryService>>([]() {
        return std::make_unique<PrimaryService>(11);
    });
    graph.register_service<smgpc::di::TransientService<ISecondaryService>>([]() {
        return std::make_unique<SecondaryService>("hello");
    });

    const auto first = graph.get<ISecondaryService>();
    const auto second = graph.get<ISecondaryService>();

    $pc_port_require(first != second);
    $pc_port_require_eq(first->describe(), "hello");
}

$test("ServiceProvider injects singleton dependency as DependencyReference") {
    Graph graph {};
    bool observed = false;
    IPrimaryService *injected_ref = nullptr;

    graph.register_service<smgpc::di::SingletonService<IPrimaryService>>([]() {
        return std::make_unique<PrimaryService>(9);
    });
    graph.register_service<smgpc::di::TransientService<IConsumerService>, IPrimaryService>(
        [&observed, &injected_ref](smgpc::di::DependencyReference<IPrimaryService> primary) {
            observed = true;
            injected_ref = &primary.get();
            return std::make_unique<ConsumerService>(std::move(primary));
        });

    const auto consumer = graph.get<IConsumerService>();
    const auto &primary = graph.get<IPrimaryService>();

    $pc_port_require(observed);
    $pc_port_require(injected_ref == &primary);
    $pc_port_require(&consumer->singleton_reference() == &primary);
}

$test("ServiceProvider::register_service with pre-built singleton unique_ptr") {
    Graph graph {};
    auto prepared = std::make_unique<PrimaryService>(7);
    const auto *prepared_ptr = prepared.get();

    graph.register_service<smgpc::di::SingletonService<IPrimaryService>>(std::move(prepared));

    $pc_port_require(&graph.get<IPrimaryService>() == prepared_ptr);
}

$test("ServiceProvider rejects duplicate registration") {
    Graph graph {};
    graph.register_service<smgpc::di::SingletonService<IPrimaryService>>([]() {
        return std::make_unique<PrimaryService>(5);
    });

    bool threw = false;
    try {
        graph.register_service<smgpc::di::SingletonService<IPrimaryService>>([]() {
            return std::make_unique<PrimaryService>(6);
        });
    } catch (const std::logic_error &) {
        threw = true;
    }

    $pc_port_require(threw);
}
