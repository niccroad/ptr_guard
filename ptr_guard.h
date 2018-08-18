/**
 * Implementation for a ptr_guard template proposal for C++ standard library. The pointer guard
 * maintains most of the semantics of the pointer (or smart pointer) type the template is
 * instantiated with but restricts access through the pointer to when the pointer is non null.
 *
 * Original work Copyright (c) 2018 Nicolas Croad
 * Modified work Copyright (c) [COPYRIGHT HOLDER]
 */

#ifndef __PTR_GUARD_H__
#define __PTR_GUARD_H__

#include <memory>
#include <functional>

#if __cplusplus > 201402L
#define __CPP17_SUPPORT__
#endif

namespace std {
namespace experimental {
    template <class T>
    class ptr_guard;

    template <class T>
    using is_pointer_type = is_same<typename pointer_traits<T>::pointer, typename remove_reference<T>::type>;

    template <class T>
    using pointer_type_or_pointer_to_type = conditional<is_pointer_type<T>::value, typename pointer_traits<T>::pointer, T*>;

    template <class T>
    using elemenent_type_of_pointer_or_type = conditional<is_pointer_type<T>::value, typename pointer_traits<T>::element_type, typename remove_reference<T>::type>;

    template <class T>
    using guard = ptr_guard<typename pointer_type_or_pointer_to_type<T>::type>;

    namespace __detail {
        template <class G, class P>
        void ptr_guard_swap(G& guard, P& p1, P& p2) {
            static_assert(false, "ptr_guard template parameter has no swap() method.");
        }

        template <class P>
        void ptr_guard_swap(ptr_guard<P>& guard, P& p1, P& p2) {
            p1.swap(p2);
        }

        template <class T>
        typename ptr_guard<T>::element_type& dereference_arg(ptr_guard<T> const& arg);

        template <class T>
        typename ptr_guard<T>::element_type& dereference_arg(ptr_guard<T>& arg);

        template <class T>
        typename ptr_guard<T>::pointer& access_guarded_pointer(ptr_guard<T>& arg);

        template <class T>
        typename ptr_guard<T>::pointer const& access_guarded_pointer(ptr_guard<T> const& arg);

        template <class T>
        typename ptr_guard<T>::pointer&& access_guarded_pointer(ptr_guard<T>&& arg);

        template <class Func, class Ret, class... Args>
        Ret check_all_then_invoke_or_default(Func&& func, Ret&& def, Args&&... args);

        template <class A, class... Args>
        bool all_args_are_safe_to_dereference(A arg, Args&&... args);

        template <class T, class... Args>
        bool all_args_are_safe_to_dereference(ptr_guard<T> const& arg, Args&&... args);

        auto get_use_count = [](auto&& ptr) { return ptr.use_count(); };
        auto release_ptr = [](auto&& ptr) { return ptr.release(); };
        auto lock_ptr = [](auto&& ptr) { return ptr.lock(); };

        template <typename P, typename... Args>
        void reset_ptr(P& p, Args... args) {
            p.reset(args...);
        }

        template <typename P>
        void reset_ptr(P*& p) {
            p = nullptr;
        }

        template <typename P, typename Arg>
        void reset_ptr(P*& p, Arg&& arg) {
            p = arg;
        }

        template <typename P>
        bool test_ptr(const P& p) {
            return static_cast<bool>(p);
        }

        template <typename T>
        bool test_ptr(const weak_ptr<T>& p) {
            return !p.expired();
        }
    }

    template <class T>
    class ptr_guard {
    public:
        typedef typename pointer_type_or_pointer_to_type<T>::type pointer;
        typedef typename elemenent_type_of_pointer_or_type<T>::type element_type;

    public:
        constexpr ptr_guard() noexcept;

        template <class P>
        ptr_guard(P other) noexcept;
        template <class P>
        ptr_guard(ptr_guard<P> const& other) noexcept;
        template <class P>
        ptr_guard(ptr_guard<P>&& other) noexcept;
        // move and copy constructors implicitely defined

        template <class P>
        ptr_guard& operator =(P other) noexcept;
        template <class P>
        ptr_guard& operator =(ptr_guard<P> const& other) noexcept;
        template <class P>
        ptr_guard& operator =(ptr_guard<P>&& other) noexcept;
        // move and copy assignment implicitely defined

        operator bool() const noexcept;

        template <class ReturnType = decltype(__detail::get_use_count(declval<pointer>()))>
        auto use_count() const noexcept -> ReturnType;

        template<class P>
        bool owner_before(const P& other) const noexcept;
        template<class Y>
        bool owner_before(const ptr_guard<Y>& other) const noexcept;

        template <class Deleter = decltype(pointer::deleter_type)>
        Deleter& get_deleter() noexcept;

        template <class Deleter = decltype(pointer::deleter_type)>
        const Deleter& get_deleter() const noexcept;

        template <class Released = decltype(__detail::release_ptr(std::declval<pointer>()))>
        Released release() noexcept;

        template <class L = decltype(__detail::lock_ptr(std::declval<pointer>()))>
        ptr_guard<L> lock() const noexcept;

        template <class... Args>
        void reset(Args&&... args) noexcept;

        void swap(pointer& other) noexcept;
        void swap(ptr_guard& other) noexcept;

        template <class Func, class... Args>
        void call(Func&& func, Args&&... args) const;

        template <class Func, class... Args>
        void call(Func&& func, Args&&... args);

        template <class Func, class Ret, class... Args>
        Ret call_or(Func&& func, Ret&& def, Args&&... args) const;

        template <class Func, class Ret, class... Args>
        Ret call_or(Func&& func, Ret&& def, Args&&... args);

    private:
        friend typename element_type& __detail::dereference_arg(ptr_guard&);
        friend typename element_type& __detail::dereference_arg(ptr_guard const&);

        friend typename pointer& __detail::access_guarded_pointer(ptr_guard&);
        friend typename pointer const& __detail::access_guarded_pointer(ptr_guard const&);
        friend typename pointer&& __detail::access_guarded_pointer(ptr_guard&&);

        typename ptr_guard<T>::element_type const& operator *() const noexcept;
        typename ptr_guard<T>::element_type& operator *() noexcept;

        pointer _ptr = {};
    };

    template <class T, class... Args>
    ptr_guard<T> make_guarded(Args&&... args) {
        return ptr_guard(new T(args...));
    }

    template <class T, class... Args>
    ptr_guard<unique_ptr<T>> make_guarded_unique(Args&&... args) {
        return ptr_guard<unique_ptr<T>>(make_unique(args...));
    }

    template <class T, class... Args>
    ptr_guard<shared_ptr<T>> make_guarded_shared(Args&&... args) {
        return ptr_guard<shared_ptr<T>>(make_shared(args...));
    }

#ifdef __CPP17_SUPPORT
    template <class T, class U>
    ptr_guard<shared_ptr<T>> reinterpret_pointer_cast(const ptr_guard<shared_ptr<U>>& other) noexcept {
        return reinterpret_pointer_cast<T>(other.pointer);
    }
#endif

    template <class T>
    ptr_guard<T>::operator bool() const noexcept { return __detail::test_ptr(_ptr); }

    template <class T>
    constexpr ptr_guard<T>::ptr_guard() noexcept = default;

    template <class T>
    template <class P>
    ptr_guard<T>::ptr_guard(P other) noexcept : _ptr(std::forward<P>(other)) { }

    template <class T>
    template <class P>
    ptr_guard<T>::ptr_guard(ptr_guard<P> const& other) noexcept
      : _ptr(__detail::access_guarded_pointer(other)) { }

    template <class T>
    template <class P>
    ptr_guard<T>::ptr_guard(ptr_guard<P>&& other) noexcept
      : _ptr(__detail::access_guarded_pointer(other)) { }

    template <class T>
    template <class P>
    ptr_guard<T>& ptr_guard<T>::operator =(P other) noexcept {
        _ptr = std::forward<P>(other);
        return *this;
    }

    template <class T>
    template <class P>
    ptr_guard<T>& ptr_guard<T>::operator =(ptr_guard<P> const& other) noexcept {
        _ptr = other._ptr;
        return *this;
    }

    template <class T>
    template <class P>
    ptr_guard<T>& ptr_guard<T>::operator =(ptr_guard<P>&& other) noexcept {
        _ptr = std::move(other._ptr);
        return *this;
    }

    template <class T>
    template <class Deleter>
    Deleter& ptr_guard<T>::get_deleter() noexcept {
        return _ptr.get_deleter();
    }

    template <class T>
    template <class Deleter>
    Deleter const&  ptr_guard<T>::get_deleter() const noexcept {
        return _ptr.get_deleter();
    }

    template <class T>
    template <class ReturnType>
    ReturnType ptr_guard<T>::use_count() const noexcept {
        return __detail::get_use_count(_ptr);
    }

    template <class T>
    template <class P>
    bool ptr_guard<T>::owner_before(const P& other) const noexcept {
        return _ptr.owner_before(other);
    }

    template <class T>
    template <class Y>
    bool ptr_guard<T>::owner_before(const ptr_guard<Y>& other) const noexcept {
        return _ptr.owner_before(other._ptr);
    }

    template <class T>
    template <class... Args>
    void ptr_guard<T>::reset(Args&&... args) noexcept {
        __detail::reset_ptr(_ptr, args...);
    }

    template <class T>
    void ptr_guard<T>::swap(pointer& other) noexcept {
        __detail::ptr_guard_swap(*this, _ptr, other);
    }

    template <class T>
    void ptr_guard<T>::swap(ptr_guard& other) noexcept {
        __detail::ptr_guard_swap(*this, _ptr, other._ptr);
    }

    template <class T>
    template <class R>
    R ptr_guard<T>::release() noexcept {
        return _ptr.release();
    }

    template <class T>
    template <class L>
    ptr_guard<L> ptr_guard<T>::lock() const noexcept {
        return _ptr.lock();
    }

    template <class T>
    typename ptr_guard<T>::element_type const& ptr_guard<T>::operator *() const noexcept { return *_ptr; }

    template <class T>
    typename ptr_guard<T>::element_type& ptr_guard<T>::operator *() noexcept { return *_ptr; }

    template <class T>
    template <class Func, class... Args>
    void ptr_guard<T>::call(Func&& func, Args&&... args) const {
        __detail::check_all_then_invoke<Func, ptr_guard const&, Args...>(
            std::forward<Func&&>(func),
            *this,
            std::forward<Args&&>(args)...);
    }

    template <class T>
    template <class Func, class... Args>
    void ptr_guard<T>::call(Func&& func, Args&&... args) {
        __detail::check_all_then_invoke<Func, ptr_guard&, Args...>(
            std::forward<Func&&>(func),
            *this,
            std::forward<Args&&>(args)...);
    }

    template <class T>
    template <class Func, class Ret, class... Args>
    Ret ptr_guard<T>::call_or(Func&& func, Ret&& def, Args&&... args) const {
        return __detail::check_all_then_invoke_or_default<Func, Ret, ptr_guard const&, Args...>(
            std::forward<Func&&>(func),
            std::forward<Ret&&>(def),
            *this,
            std::forward<Args&&>(args)...);
    }

    template <class T>
    template <class Func, class Ret, class... Args>
    Ret ptr_guard<T>::call_or(Func&& func, Ret&& def, Args&&... args) {
        return __detail::check_all_then_invoke_or_default<Func, Ret, ptr_guard&, Args...>(
            std::forward<Func&&>(func),
            std::forward<Ret&&>(def),
            *this,
            std::forward<Args&&>(args)...);
    }

    namespace __detail {
        inline bool all_args_are_safe_to_dereference() { return true; }

        template <class A>
        bool all_args_are_safe_to_dereference(A arg) {
            return true;
        }

        template <class T>
        bool all_args_are_safe_to_dereference(ptr_guard<T> const& arg) {
            return static_cast<bool>(arg);
        }

        template <class A, class... Args>
        bool all_args_are_safe_to_dereference(A arg, Args&&... args) {
            return all_args_are_safe_to_dereference(args...);
        }

        template <class T, class... Args>
        bool all_args_are_safe_to_dereference(ptr_guard<T> const& arg, Args&&... args) {
            return static_cast<bool>(arg) && all_args_are_safe_to_dereference(args...);
        }

        template <class A>
        A&& dereference_arg(A&& arg) { return std::forward<A&&>(arg); }

        template <class T>
        typename ptr_guard<T>::element_type& dereference_arg(ptr_guard<T> const& arg) { return *const_cast<ptr_guard<T>&>(arg); }

        template <class T>
        typename ptr_guard<T>::element_type& dereference_arg(ptr_guard<T>& arg) { return *arg; }

        template <class T>
        typename ptr_guard<T>::pointer& access_guarded_pointer(ptr_guard<T>& arg) { return arg._ptr; }

        template <class T>
        typename ptr_guard<T>::pointer const& access_guarded_pointer(ptr_guard<T> const& arg) { return arg._ptr; }

        template <class T>
        typename ptr_guard<T>::pointer&& access_guarded_pointer(ptr_guard<T>&& arg) { return std::move(arg._ptr); }

        template <class Func, class... Args>
        void check_all_then_invoke(Func&& func, Args&&... args) {
            if (all_args_are_safe_to_dereference(std::forward<Args&&>(args)...)) {
                std::invoke(func, dereference_arg(args)...);
            }
        }

        template <class Func, class Ret, class... Args>
        Ret check_all_then_invoke_or_default(Func&& func, Ret&& def, Args&&... args) {
            if (!all_args_are_safe_to_dereference(std::forward<Args&&>(args)...)) { return def; }
            return std::invoke(func, dereference_arg(args)...);
        }
    }
}
}

#ifdef __CPP17_SUPPORT__
#undef __CPP17_SUPPORT__
#endif // __CPP17_SUPPORT__

#endif // __PTR_GUARD_H__
