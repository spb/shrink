
#ifndef SHRINK_GUARD_INCLUDE_SHRINK_OWNED_PTR_HH
#define SHRINK_GUARD_INCLUDE_SHRINK_OWNED_PTR_HH 1

#include <stdexcept>
#include <atomic>

namespace shrink
{
    namespace exceptions
    {
        struct ReferencesStillExistException : std::runtime_error
        {
            ReferencesStillExistException()
                : std::runtime_error("Attempted to delete an owned_ptr while references to it still exist")
            { }
        };

        struct ReleasedInvalidOwnedPtrException : std::runtime_error
        {
            ReleasedInvalidOwnedPtrException()
                : std::runtime_error("Attempted to delete an owned_ptr that wasn't valid")
            { }
        };

        struct ReleasedInvalidHandlePtrException : std::runtime_error
        {
            ReleasedInvalidHandlePtrException()
                : std::runtime_error("Attempted to release a handle_ptr that wasn't valid")
            { }
        };

        struct InvalidOwnedPtrException : std::runtime_error
        {
            InvalidOwnedPtrException()
                : std::runtime_error("Attempted to reference an invalid owned_ptr")
            { }
        };

        struct InvalidHandlePtrException : std::runtime_error
        {
            InvalidHandlePtrException()
                : std::runtime_error("Attempted to dereference a handle_ptr to an invalid owned_ptr")
            { }
        };
    }

    template <typename T_>
    class handle_ptr;

    template <typename T_>
    class owned_ptr
    {
        public:
            owned_ptr(T_ * obj)
                : _obj(obj), _references(0)
            { }

            owned_ptr(owned_ptr && rhs)
                : _obj(rhs._obj), _references(rhs._references)
            { }

            void operator=(owned_ptr && rhs)
            {
                if (good())
                    release();

                _obj = rhs._obj;
                rhs._obj = nullptr;
                _references = rhs._references;
            }

            owned_ptr() = delete;
            owned_ptr(const owned_ptr & rhs) = delete;
            void operator=(const owned_ptr & rhs) = delete;

            ~owned_ptr() { if (good()) release(); }

            T_ * operator->() const { check_deref(); return  _obj; }
            T_ & operator* () const { check_deref(); return *_obj; }

            void release()
            {
                if (!good())
                    throw exceptions::ReleasedInvalidOwnedPtrException();

                if (_references > 0)
                    throw exceptions::ReferencesStillExistException();

                delete _obj;
                _obj = nullptr;
            }

            bool good() const { return _obj != nullptr; }

        private:
            T_ * _obj;
            mutable std::atomic_uint _references;

            friend class handle_ptr<T_>;

            void check_deref() const
            {
                if (!good())
                    throw exceptions::InvalidOwnedPtrException();
            }
    };

    template <typename T_>
    class handle_ptr
    {
        public:
            handle_ptr(const owned_ptr<T_> & p)
                : _ptr(&p)
            {
                _ptr->check_deref();

                ++_ptr->_references;
            }

            handle_ptr(handle_ptr && rhs)
                : _ptr(rhs._ptr)
            { rhs._ptr = nullptr; }

            handle_ptr(const handle_ptr & rhs)
                : _ptr(rhs._ptr)
            {
                ++_ptr->_references;
            }

            handle_ptr() = delete;

            const handle_ptr & operator=(const handle_ptr & rhs)
            {
                if (_ptr)
                    release();

                _ptr = rhs._ptr;
                ++_ptr->_references;
            }

            ~handle_ptr()
            {
                if (_ptr)
                    release();
            }

            bool good()
            {
                return _ptr && _ptr->good();
            }

            void release()
            {
                if (!good())
                    throw exceptions::ReleasedInvalidHandlePtrException();

                --_ptr->_references;
                _ptr = nullptr;
            }

            T_ & operator * () const { check_deref(); return *_ptr->_obj; }
            T_ * operator-> () const { check_deref(); return  _ptr->_obj; }


        private:
            const owned_ptr<T_> * _ptr;

            void check_deref() const
            {
                if (!_ptr || !_ptr->good())
                    throw exceptions::InvalidHandlePtrException();
            }
    };
}

#endif


// vim: set sw=4 sts=4 et :
