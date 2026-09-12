#pragma once

template < typename T >
class SingletonHolder {
public:
    static void init() {
        if (sInstance == nullptr) {
            sInstance = new T();
        }
    }

    static T* get() {
        return sInstance;
    }

#if defined(TARGET_PC)
    // Native process retirement: drain borrowers and hold the guest CPU gate
    // before releasing the actual singleton to its typed lifetime owner.
    static T* release() noexcept {
        T* instance = sInstance;
        sInstance = nullptr;
        return instance;
    }
#endif

private:
    static T* sInstance;
};

template < typename T >
T* SingletonHolder< T >::sInstance;

template < typename T >
class AudSingletonHolder {
public:
    static void init() {
        if (sInstance == nullptr) {
            sInstance = new T();
        }
    }

    static T* get() {
        return sInstance;
    }

#if defined(TARGET_PC)
    // Native owners publish complete original objects and restore the previous
    // borrower only after all users of their process lifetime have retired.
    static T* exchange(T* instance) noexcept {
        T* previous = sInstance;
        sInstance = instance;
        return previous;
    }
#endif

private:
    static T* sInstance;
};

template < typename T >
T* AudSingletonHolder< T >::sInstance;
