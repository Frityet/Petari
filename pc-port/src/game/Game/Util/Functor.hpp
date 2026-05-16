#pragma once

class JKRHeap;

namespace MR {
class FunctorBase {
public:
    virtual ~FunctorBase() = default;
    virtual void operator()() const = 0;
    [[nodiscard]] virtual FunctorBase *clone(JKRHeap *) const = 0;
};

template < typename T, typename U >
class FunctorV0M : public FunctorBase {
public:
    FunctorV0M(T call, U callee)
        : mCaller(call), mCallee(callee) {
    }

    FunctorV0M() = default;

    void operator()() const override {
        (mCaller->*mCallee)();
    }

    [[nodiscard]] FunctorBase *clone(JKRHeap *) const override {
        return new FunctorV0M(*this);
    }

    T mCaller {};
    U mCallee {};
};

template < class T >
[[nodiscard]] FunctorV0M<T *, void (T::*)()> Functor(T *pObject, void (T::*pMethod)()) {
    return FunctorV0M<T *, void (T::*)()>(pObject, pMethod);
}

template < class T >
[[nodiscard]] FunctorV0M<T *, void (T::*)() const> Functor(T *pObject, void (T::*pMethod)() const) {
    return FunctorV0M<T *, void (T::*)() const>(pObject, pMethod);
}

}  // namespace MR
