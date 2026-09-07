#pragma once

#include <list>

class JUTTexture;

namespace smgpc::compat {
    class JutTextureConstructionScope;

    // Explicit ownership of heap-constructed SDK textures. The record remains
    // valid when the original heap finalizer or another owner destroys the
    // texture first, including subsequent reuse of the same object address.
    class JutTextureOwnership final {
    public:
        JutTextureOwnership() = default;
        ~JutTextureOwnership();
        JutTextureOwnership(const JutTextureOwnership&) = delete;
        JutTextureOwnership& operator=(const JutTextureOwnership&) = delete;

        void adopt(JutTextureConstructionScope&, JUTTexture*) noexcept;
        void adopt_all(JutTextureConstructionScope&) noexcept;
        void clear() noexcept;

    private:
        struct Record { JUTTexture* texture; };
        std::list<Record> _textures;
        friend class JutTextureConstructionScope;
        friend void record_completed_jut_texture(JUTTexture&);
        friend void forget_completed_jut_texture(JUTTexture&) noexcept;
    };

    // Only bracket audited calls that construct textures with new. Stack and
    // placement objects must not outlive this scope unless destroyed explicitly.
    // A nested disabled scope suspends capture for independent construction.
    class JutTextureConstructionScope final {
    public:
        explicit JutTextureConstructionScope(bool enabled = true) noexcept;
        ~JutTextureConstructionScope();
        JutTextureConstructionScope(const JutTextureConstructionScope&) = delete;
        JutTextureConstructionScope& operator=(const JutTextureConstructionScope&) = delete;
    private:
        JutTextureConstructionScope* _previous;
        JutTextureOwnership _created;
        friend class JutTextureOwnership;
        friend void record_completed_jut_texture(JUTTexture&);
    };

    // SDK constructor/destructor hooks, after initialization has completed.
    void record_completed_jut_texture(JUTTexture&);
    void forget_completed_jut_texture(JUTTexture&) noexcept;
}
