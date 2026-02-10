#include <string_view>
#include <memory>

namespace smgpc::render
{
    class Renderer {
    public:
        Renderer(const Renderer &) = delete;
        Renderer(Renderer &&) = delete;

        virtual void on_frame_enter() = 0;
        virtual void draw() = 0;
        virtual void on_frame_exit() = 0;

        virtual ~Renderer() = 0;
    protected:
        Renderer() = default;
    };

    class Window {
    public:
        static std::unique_ptr<Window> create(int width, int height, std::string_view title);
        virtual Renderer &renderer() = 0;
        virtual void handle_events() = 0;
        virtual ~Window() = 0;
    };
}
