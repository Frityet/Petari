#pragma once

class JKRHeap;

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

        [[nodiscard]] FunctorBase* clone(JKRHeap*) const override {
            return new FunctorV0M(*this);
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
}  // namespace MR
