#include "RenderWindow.hpp"
#include "Logger.hpp"

int main()
{
    smgpc::logging::info(smgpc::logging::Category::APP, "Started app");
    auto window = smgpc::render::Window::create(800, 600, "SMG PC Port");
    smgpc::logging::info(smgpc::logging::Category::APP, "Created window with {}x{}", 800, 600);
    window->handle_events();
}
