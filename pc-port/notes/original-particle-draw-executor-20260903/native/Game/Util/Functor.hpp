#pragma once

#include <JSystem/JKernel/JKRHeap.hpp>

namespace MR {
    class FunctorBase {
    public:
        virtual ~FunctorBase() = default;
        virtual void operator()() const = 0;
        [[nodiscard]] virtual FunctorBase* clone(JKRHeap*) const = 0;
    };

    template < typename T, typename U >
    class FunctorV0M : public FunctorBase {
    public:
        FunctorV0M(T caller, U callee) : mCaller(caller), mCallee(callee) {
        }

        void operator()() const override {
            (mCaller->*mCallee)();
        }

        [[nodiscard]] FunctorBase* clone(JKRHeap* pHeap) const override {
            return new (pHeap, 0) FunctorV0M(*this);
        }

        T mCaller;
        U mCallee;
    };

    template < class T >
    [[nodiscard]] FunctorV0M< T*, void (T::*)() > Functor(T* pCaller, void (T::*pCallee)()) {
        return FunctorV0M< T*, void (T::*)() >(pCaller, pCallee);
    }

    template < class T >
    [[nodiscard]] FunctorV0M< T*, void (T::*)() const > Functor(T* pCaller, void (T::*pCallee)() const) {
        return FunctorV0M< T*, void (T::*)() const >(pCaller, pCallee);
    }

    template < class T >
    [[nodiscard]] FunctorV0M< T*, void (T::*)() > Functor_Inline(T* pCaller, void (T::*pCallee)()) {
        return Functor(pCaller, pCallee);
    }
    template < class T >
    inline static FunctorV0M< T*, void (T::*)() const > Functor_Inline(T* a1, void (T::*a2)() const) {
        return FunctorV0M< T*, void (T::*)() const >(a1, a2);
    }

    template < class T >
    inline static FunctorV0M< const T*, void (T::*)() const > Functor(const T* caller, void (T::*callee)() const) {
        return FunctorV0M< const T*, void (T::*)() const >(caller, callee);
    }

    class FunctorV0F : public FunctorBase {
    public:
        explicit FunctorV0F(void (*func)()) : mFunc(func) {}
        void operator()() const override { (*mFunc)(); }
        FunctorBase* clone(JKRHeap* heap) const override { return new (heap, 0) FunctorV0F(*this); }
        void (*mFunc)();
    };

    inline static FunctorV0F Functor(void (*func)()) { return FunctorV0F(func); }
    inline static FunctorV0F Functor_Inline(void (*func)()) { return FunctorV0F(func); }
}  // namespace MR
