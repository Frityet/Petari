#include "di/ServiceContainer.hpp"
#include "tests/TestHarness.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

struct IPrimaryService {
    virtual ~IPrimaryService() = default;
    [[nodiscard]] virtual int value() const = 0;
};

struct ISecondaryService {
    virtual ~ISecondaryService() = default;
    [[nodiscard]] virtual std::string describe() const = 0;
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

using Graph = smgpc::di::ServiceContainer<IPrimaryService, ISecondaryService>;

}  // namespace

$test("ServiceContainer::has reports missing before registration") {
    Graph graph {};

    $pc_port_require(not graph.has<IPrimaryService>());
    $pc_port_require(not graph.has<ISecondaryService>());
}

$test("ServiceContainer::register_instance and resolve return same instance") {
    Graph graph {};
    auto service = std::make_shared<PrimaryService>(42);

    graph.register_instance<IPrimaryService>(service);

    $pc_port_require(graph.has<IPrimaryService>());
    $pc_port_require(graph.resolve_shared<IPrimaryService>() == service);
    $pc_port_require_eq(graph.resolve<IPrimaryService>().value(), 42);
}

$test("ServiceContainer::register_instance rejects null") {
    Graph graph {};
    bool threw = false;

    try {
        graph.register_instance<IPrimaryService>(nullptr);
    } catch (const std::invalid_argument &) {
        threw = true;
    }

    $pc_port_require(threw);
}

$test("ServiceContainer::register_type constructs implementation with arguments") {
    Graph graph {};

    graph.register_type<IPrimaryService, PrimaryService>(7);

    $pc_port_require(graph.has<IPrimaryService>());
    $pc_port_require_eq(graph.resolve<IPrimaryService>().value(), 7);
}

$test("ServiceContainer::register_factory can resolve dependencies") {
    Graph graph {};

    graph.register_type<IPrimaryService, PrimaryService>(9);
    graph.register_factory<ISecondaryService>([](Graph &services) {
            return std::make_shared<SecondaryService>("value=" + std::to_string(services.resolve<IPrimaryService>().value()));
        });

    $pc_port_require(graph.has<ISecondaryService>());
    $pc_port_require_eq(graph.resolve<ISecondaryService>().describe(), std::string("value=9"));
}

$test("ServiceContainer::register_factory rejects null result") {
    Graph graph {};
    bool threw = false;

    try {
        graph.register_factory<IPrimaryService>([](Graph &) {
                return std::shared_ptr<IPrimaryService> {};
            });
    } catch (const std::invalid_argument &) {
        threw = true;
    }

    $pc_port_require(threw);
}

$test("ServiceContainer::resolve throws for unregistered service") {
    Graph graph {};
    bool threw = false;

    try {
        (void)graph.resolve<IPrimaryService>();
    } catch (const std::logic_error &) {
        threw = true;
    }

    $pc_port_require(threw);
}

$test("ServiceContainer move preserves registered instances") {
    Graph graph {};
    auto primary = std::make_shared<PrimaryService>(13);
    graph.register_instance<IPrimaryService>(primary);

    Graph moved = std::move(graph);

    $pc_port_require(moved.has<IPrimaryService>());
    $pc_port_require_eq(moved.resolve<IPrimaryService>().value(), 13);
    $pc_port_require(moved.resolve_shared<IPrimaryService>() == primary);
}
