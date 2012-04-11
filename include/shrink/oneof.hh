#ifndef libshrink__oneof_hh
#define libshrink__oneof_hh

#include <string>
#include <memory>
#include <type_traits>
#include <utility>

#include <shrink/storage_policy.hh>

namespace shrink
{
    namespace oneof_internal
    {
        struct UnknownTypeForOneOf;

        template <typename Want_, typename... Types_>
        struct SelectOneOfType;

        template <typename Want_>
        struct SelectOneOfType<Want_>
        {
            typedef UnknownTypeForOneOf Type;
        };

        template <typename Want_, typename Try_, typename... Rest_>
        struct SelectOneOfType<Want_, Try_, Rest_...>
        {
            typedef typename std::conditional<
                std::is_same<Want_, Try_>::value,
                Try_,
                typename SelectOneOfType<Want_, Rest_...>::Type
                    >::type Type;
        };

        template <typename Type_>
        struct ParameterTypes;

        template <typename C_, typename R_, typename P_>
        struct ParameterTypes<R_ (C_::*)(P_)>
        {
            typedef P_ FirstParameterType;
            typedef R_ ReturnType;
        };

        template <typename C_, typename R_, typename P_>
        struct ParameterTypes<R_ (C_::*)(P_) const>
        {
            typedef P_ FirstParameterType;
            typedef R_ ReturnType;
        };

        template <typename Lambda_>
        struct LambdaParameterTypes
        {
            typedef typename ParameterTypes<decltype(&Lambda_::operator())>::FirstParameterType FirstParameterType;
            typedef typename ParameterTypes<decltype(&Lambda_::operator())>::ReturnType ReturnType;
        };

        template <typename Type_>
        struct OneOfVisitorVisit
        {
            virtual void visit(Type_ &) = 0;
        };

        template <typename... Types_>
        struct OneOfVisitor :
            OneOfVisitorVisit<Types_>...
        {
        };

        template <typename Visitor_, typename Underlying_, typename Result_, typename... Types_>
        struct OneOfVisitorWrapperVisit;

        template <typename Visitor_, typename Underlying_, typename Result_>
        struct OneOfVisitorWrapperVisit<Visitor_, Underlying_, Result_> :
            Visitor_
        {
            Underlying_ & underlying;
            std::function<Result_ ()> execute;

            OneOfVisitorWrapperVisit(Underlying_ & u)
                : underlying(u)
            {
            }
        };

        template <typename Visitor_, typename Underlying_, typename Result_, typename Type_, typename... Rest_>
        struct OneOfVisitorWrapperVisit<Visitor_, Underlying_, Result_, Type_, Rest_...> :
            OneOfVisitorWrapperVisit<Visitor_, Underlying_, Result_, Rest_...>
        {
            OneOfVisitorWrapperVisit(Underlying_ & u)
                : OneOfVisitorWrapperVisit<Visitor_, Underlying_, Result_, Rest_...>(u)
            {
            }

            Result_ visit_returning(Type_ & t)
            {
                return this->underlying.visit(t);
            }

            virtual void visit(Type_ & t)
            {
                this->execute = std::bind(&OneOfVisitorWrapperVisit::visit_returning, this, std::ref(t));
            }
        };

        template <typename Underlying_, typename Result_, typename... Types_>
        struct OneOfVisitorWrapper :
            OneOfVisitorWrapperVisit<OneOfVisitor<Types_...>, Underlying_, Result_, Types_...>
        {
            OneOfVisitorWrapper(Underlying_ & u)
                : OneOfVisitorWrapperVisit<OneOfVisitor<Types_...>, Underlying_, Result_, Types_...>(u)
            {
            }
        };

        template <typename... Types_>
        struct OneOfValueBase
        {
            virtual ~OneOfValueBase() = 0;

            virtual void accept(OneOfVisitor<Types_...> &) = 0;
            virtual void accept(OneOfVisitor<const Types_...> &) const = 0;
            virtual OneOfValueBase * clone() = 0;
        };

        template <typename... Types_>
        OneOfValueBase<Types_...>::~OneOfValueBase() = default;

        template <typename Type_, typename... Types_>
        struct OneOfValue :
            OneOfValueBase<Types_...>
        {
            Type_ value;

            OneOfValue(const Type_ & type)
                : value(type)
            {
            }

            OneOfValue(const OneOfValue & other)
                : value(other.value)
            {
            }

            virtual void accept(OneOfVisitor<Types_...> & visitor)
            {
                static_cast<OneOfVisitorVisit<Type_> &>(visitor).visit(value);
            }

            virtual void accept(OneOfVisitor<const Types_...> & visitor) const
            {
                static_cast<OneOfVisitorVisit<const Type_> &>(visitor).visit(value);
            }

            virtual OneOfValue * clone()
            {
                return new OneOfValue(*this);
            }
        };

        template <typename Policy_, typename Value_> struct OneOfStorage;

        template <typename Value_>
        struct OneOfStorage<shrink::storage_policy::unique_storage, Value_>
        {
            std::unique_ptr<Value_> _storage;

            Value_ & operator*() { return *_storage; }
            const Value_ & operator*() const { return *_storage; }

            OneOfStorage(Value_ * v) : _storage(v) { }
            OneOfStorage(OneOfStorage && other) : _storage(std::move(other._storage)) { }
            OneOfStorage(const OneOfStorage &) = delete;
            OneOfStorage & operator= (const OneOfStorage &) = delete;

            void reset(Value_* v) { _storage.reset(v); }
        };

        template <typename Value_>
        struct OneOfStorage<shrink::storage_policy::shared_storage, Value_>
        {
            std::shared_ptr<Value_> _storage;

            Value_ & operator*() { return *_storage; }
            const Value_ & operator*() const { return *_storage; }

            OneOfStorage(Value_ * v) : _storage(v) { }
            OneOfStorage(OneOfStorage && other) : _storage(std::move(other._storage)) { }
            OneOfStorage(const OneOfStorage & other) : _storage(other._storage) { }
            OneOfStorage & operator= (const OneOfStorage & other) { _storage = other._storage; }

            void reset(Value_* v) { _storage.reset(v); }
        };

        template <typename Value_>
        struct OneOfStorage<shrink::storage_policy::clone_storage, Value_>
        {
            std::unique_ptr<Value_> _storage;

            Value_ & operator*() { return *_storage; }
            const Value_ & operator*() const { return *_storage; }

            OneOfStorage(Value_ * v) : _storage(v) { }
            OneOfStorage(OneOfStorage && other) : _storage(std::move(other._storage)) { }
            OneOfStorage(const OneOfStorage & other) : _storage(other._storage->clone()) { }
            OneOfStorage & operator= (const OneOfStorage & other) { _storage = other._storage->clone(); }

            void reset(Value_* v) { _storage.reset(v); }
        };

        template <typename Policy_, typename... Types_>
        class OneOfImpl
        {
            private:
                oneof_internal::OneOfStorage<Policy_, oneof_internal::OneOfValueBase<Types_...> > _value;

            public:
                template <typename Type_>
                OneOfImpl(const Type_ & value)
                    : _value(new oneof_internal::OneOfValue<typename oneof_internal::SelectOneOfType<Type_, Types_...>::Type, Types_...>{value})
                {
                }

                OneOfImpl(const OneOfImpl & other)
                    : _value(other._value)
                {
                }

                OneOfImpl(OneOfImpl && other)
                    : _value(std::move(other._value))
                {
                }

                template <typename Type_>
                OneOfImpl & operator= (const Type_ & value)
                {
                    _value.reset(new oneof_internal::OneOfValue<typename oneof_internal::SelectOneOfType<Type_, Types_...>::Type, Types_...>{value});
                    return *this;
                }

                OneOfImpl & operator= (const OneOfImpl & other)
                {
                    _value = other._value;
                }

                OneOfImpl & operator= (OneOfImpl && other)
                {
                    _value = std::move(other._value);
                    return *this;
                }

                oneof_internal::OneOfValueBase<Types_...> & value()
                {
                    return *_value;
                }

                const oneof_internal::OneOfValueBase<Types_...> & value() const
                {
                    return *_value;
                }
        };

        template <typename Visitor_, typename Result_, typename OneOf_>
        struct OneOfVisitorWrapperTypeFinder;

        template <typename Visitor_, typename Result_, typename Policy_, typename... Types_>
        struct OneOfVisitorWrapperTypeFinder<Visitor_, Result_, const OneOfImpl<Policy_, Types_...> &>
        {
            typedef OneOfVisitorWrapper<Visitor_, Result_, const Types_...> Type;
        };

        template <typename Visitor_, typename Result_, typename Policy_, typename... Types_>
        struct OneOfVisitorWrapperTypeFinder<Visitor_, Result_, OneOfImpl<Policy_, Types_...> &>
        {
            typedef OneOfVisitorWrapper<Visitor_, Result_, Types_...> Type;
        };

        template <typename Result_, typename OneOf_, typename Visitor_>
        Result_
        accept_returning(OneOf_ && one_of, Visitor_ && visitor)
        {
            typename OneOfVisitorWrapperTypeFinder<Visitor_, Result_, OneOf_>::Type visitor_wrapper(visitor);
            one_of.value().accept(visitor_wrapper);
            return visitor_wrapper.execute();
        }

        template <typename OneOf_, typename Visitor_>
        void accept(OneOf_ && one_of, Visitor_ && visitor)
        {
            accept_returning<void>(one_of, visitor);
        }

        template <typename Result_, typename... Funcs_>
        struct LambdaVisitor;

        template <typename Result_>
        struct LambdaVisitor<Result_>
        {
            void visit(struct NotReallyAType);
        };

        template <typename Result_, typename Func_, typename... Rest_>
        struct LambdaVisitor<Result_, Func_, Rest_...> :
            LambdaVisitor<Result_, Rest_...>
        {
            Func_ & func;

            LambdaVisitor(Func_ & f, Rest_ & ... rest)
                : LambdaVisitor<Result_, Rest_...>(rest...),
                  func(f)
            {
            }

            Result_ visit(typename LambdaParameterTypes<Func_>::FirstParameterType & v)
            {
                return func(v);
            }

            using LambdaVisitor<Result_, Rest_...>::visit;
        };

        // Default storage policy for OneOf is defined here
        template <typename... Types_> struct OneOfTypeFinder
        {
            typedef OneOfImpl<shrink::storage_policy::unique_storage, Types_...> Type;
        };

        template <typename... Types_> struct OneOfTypeFinder<shrink::storage_policy::unique_storage, Types_...>
        {
            typedef OneOfImpl<shrink::storage_policy::unique_storage, Types_...> Type;
        };
        template <typename... Types_> struct OneOfTypeFinder<shrink::storage_policy::shared_storage, Types_...>
        {
            typedef OneOfImpl<shrink::storage_policy::shared_storage, Types_...> Type;
        };
        template <typename... Types_> struct OneOfTypeFinder<shrink::storage_policy::clone_storage, Types_...>
        {
            typedef OneOfImpl<shrink::storage_policy::clone_storage, Types_...> Type;
        };
    }

    template <typename... Types_> using OneOf = typename oneof_internal::OneOfTypeFinder<Types_...>::Type;

    template <typename Val_, typename FirstFunc_, typename... Rest_>
    typename oneof_internal::LambdaParameterTypes<FirstFunc_>::ReturnType
    when(Val_ && val, FirstFunc_ && first_func, Rest_ && ... rest)
    {
        return oneof_internal::accept_returning<typename oneof_internal::LambdaParameterTypes<FirstFunc_>::ReturnType>(
                val,
                oneof_internal::LambdaVisitor<typename oneof_internal::LambdaParameterTypes<FirstFunc_>::ReturnType, FirstFunc_, Rest_...>(first_func, rest...));
    }

    template <typename Result_, typename Policy_, typename... Types_>
    const Result_ & extract(const oneof_internal::OneOfImpl<Policy_, Types_...> & oneof)
    {
        return when(oneof, [](const Result_ & r) -> const Result_ & { return r; });
    }

    template <typename Result_, typename Policy_, typename... Types_>
    Result_ & extract(oneof_internal::OneOfImpl<Policy_, Types_...>& oneof)
    {
        return when(oneof, [](Result_ & r) -> Result_ & { return r; });
    }

}

#endif
