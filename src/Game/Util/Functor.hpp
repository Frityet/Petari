#pragma once

#include "Inline.hpp"
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

    template < typename T, typename U, typename V >
    class FunctorV1M : public FunctorBase {
    public:
        inline FunctorV1M(T call, U callee, V arg_0) {
            mCaller = call;
            mCallee = callee;
            mArg0 = arg_0;
        }

        inline FunctorV1M() {
        }

        virtual void operator()() const {
            (mCaller->*mCallee)(mArg0);
        }

        virtual FunctorBase* clone(JKRHeap* pHeap) const {
            return new (pHeap, 0) FunctorV1M(*this);
        };

        T mCaller;
        U mCallee;
        V mArg0;
    };

    template < typename T, typename U, typename V, typename W >
    class FunctorV2M : public FunctorBase {
    public:
        inline FunctorV2M(T call, U callee, V arg_0, W arg_1) {
            mCaller = call;
            mCallee = callee;
            mArg0 = arg_0;
            mArg1 = arg_1;
        }

        inline FunctorV2M() {
        }

        virtual void operator()() const {
            (mCaller->*mCallee)(mArg0, mArg1);
        }

        virtual FunctorBase* clone(JKRHeap* pHeap) const {
            return new (pHeap, 0) FunctorV2M(*this);
        };

        T mCaller;
        U mCallee;
        V mArg0;
        W mArg1;
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

    template < class T >
    inline static FunctorV0M< const T*, void (T::*)() const > Functor_InlineC(T* a1, void (T::*a2)() const) {
        return FunctorV0M< const T*, void (T::*)() const >(a1, a2);
    }

    template < class T, typename U >
    static FunctorV1M< T*, void (T::*)(U), U > Functor(T* a1, void (T::*a2)(U), U arg_0) NO_INLINE {
        return FunctorV1M< T*, void (T::*)(U), U >(a1, a2, arg_0);
    }

    template < class T, typename U >
    inline static FunctorV1M< T*, void (T::*)(U), U > Functor_Inline(T* a1, void (T::*a2)(U), U arg_0) {
        return FunctorV1M< T*, void (T::*)(U), U >(a1, a2, arg_0);
    }

    template < class T, typename U, typename V >
    static FunctorV2M< T*, void (T::*)(U, V), U, V > Functor(T* a1, void (T::*a2)(U, V), U arg_0, V arg_1) {
        return FunctorV2M< T*, void (T::*)(U, V), U, V >(a1, a2, arg_0, arg_1);
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
