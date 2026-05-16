#pragma once

#include <cstdint>
#include <memory>

namespace smgpc::render::layout {
    class LayoutDrawList;
}

class LayoutActor;

namespace smgpc::game {

    enum class FileSelectPreviewCompletion {
        None,
        CreatedNewFile,
        LoadedExistingFile,
        Failed,
    };

    class FileSelectPreview {
    public:
        FileSelectPreview();
        ~FileSelectPreview();

        FileSelectPreview(const FileSelectPreview&) = delete;
        FileSelectPreview& operator=(const FileSelectPreview&) = delete;
        FileSelectPreview(FileSelectPreview&&) noexcept;
        FileSelectPreview& operator=(FileSelectPreview&&) noexcept;

        void appear();
        void movement();
        void appendDrawCommands(render::layout::LayoutDrawList* pDrawList) const;
        void appendDrawCommands(render::layout::LayoutDrawList* pDrawList, std::uint64_t skyFrame) const;

        [[nodiscard]] bool isEnd() const;
        [[nodiscard]] FileSelectPreviewCompletion completion() const;
        [[nodiscard]] const LayoutActor* layoutForSize() const;

    private:
        class Impl;
        std::unique_ptr< Impl > mImpl;
    };

}  // namespace smgpc::game
