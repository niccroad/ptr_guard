#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "ptr_guard.h"

#if __cplusplus > 201402L
#ifndef __CPP17_SUPPORT__
#define __CPP17_SUPPORT__
#endif // __CPP17_SUPPORT__
#endif

namespace {
    class TestContext;

    TestContext* context = nullptr;

    class TestContext {
    public:
        TestContext() {
            context = this;
        }

        ~TestContext() {
            context = nullptr;
        }

        int pointeeDestructorCalls = 0;
    };
}

struct Pointee {
    Pointee() = default;
    Pointee(int id) : identifier(id) { }
    Pointee(const Pointee& other) = default;
    Pointee(Pointee&& other) = default;
    virtual ~Pointee() { if (context) context->pointeeDestructorCalls++; }

    int identifier = 0;
};

struct DerivedFromPointee : public Pointee { };

template <typename T>
struct PtrWithout {
public:
    PtrWithout() {}
    PtrWithout(T* p) : ptr(p) {}

    operator bool() const noexcept { return static_cast<bool>(ptr); }
    T& operator *() { return *ptr; }

    T* ptr = nullptr;
};

namespace std {
    template <class T>
    class pointer_traits<PtrWithout<T>> {
    public:
        typedef PtrWithout<T> pointer;
        typedef T element_type;
    };
}

struct ExplicitelyConvertibleToPointee {
    operator Pointee () const { return pointee; }
    Pointee pointee;
};

using namespace std;
using namespace std::experimental;

template <typename T>
bool const_pointee_is_accessible(const ptr_guard<T>& guard) {
    bool lambda1Called = false;
    guard.call([&](const Pointee& a) { lambda1Called = true; });

    bool lambda2Called = false;
    guard.call([&](const Pointee a) { lambda2Called = true; });
    REQUIRE(lambda1Called == lambda2Called);

    bool lambda3Called = false;
    guard.call([&](Pointee a) { lambda3Called = true; });
    REQUIRE(lambda1Called == lambda3Called);

    bool lambda5Called = false;
    guard.call(
        [&](const Pointee& a, int one, int two, int three) {
            lambda5Called = true;
        }, 1, 2, 3);
    REQUIRE(lambda1Called == lambda5Called);

    bool lambda6Called = false;
    guard.call(
        [&](const Pointee& a, int one, const Pointee& b, int two, const Pointee& c, int three) {
        lambda6Called = true;
    }, 1, guard, 2, guard, 3);
    REQUIRE(lambda1Called == lambda6Called);

    bool lambda7Called = false;
    ptr_guard<Pointee*> otherGuard;
    guard.call(
        [&](const Pointee& a, int one, const Pointee& b, int two, const Pointee& c, int three) {
        lambda7Called = true;
    }, 1, otherGuard, 2, otherGuard, 3);
    REQUIRE_FALSE(lambda7Called);

    bool lambda8Called = false;
    Pointee other;
    otherGuard.reset(&other);
    guard.call(
        [&](const Pointee& a, int one, const Pointee& b, int two, const Pointee& c, int three) {
        lambda8Called = true;
    }, 1, otherGuard, 2, otherGuard, 3);
    REQUIRE(lambda1Called == lambda8Called);

    bool lambda9Called = false;
    guard.call(
        [&](const Pointee& a, int one, Pointee& b, int two, Pointee& c, int three) {
        lambda9Called = true;
    }, 1, otherGuard, 2, otherGuard, 3);
    REQUIRE(lambda1Called == lambda9Called);

    bool weakPointerParamLambdaCalled = false;
    shared_ptr<Pointee> sharedPtr(new Pointee);
    ptr_guard<weak_ptr<Pointee>> weakPtrGuard(sharedPtr);
    guard.call(
        [&](const Pointee& a, const Pointee& b) {
        weakPointerParamLambdaCalled = true;
    }, weakPtrGuard.lock());
    REQUIRE(lambda1Called == weakPointerParamLambdaCalled);

    {
        bool lambda10Called = false;
        ExplicitelyConvertibleToPointee convertableOther;
        ptr_guard<ExplicitelyConvertibleToPointee*> convertableGuard(&convertableOther);
        guard.call(
            [&](const Pointee& a, Pointee b) {
            lambda10Called = true;
        }, convertableGuard);
        REQUIRE(lambda1Called == lambda10Called);
    }

    int expectedRet = lambda1Called ? 1 : 0;
    { // Getting a return type or default from the call.
        bool lambda10Called = false;
        int ret = guard.call_or(
            [&](const Pointee& a) -> int {
                lambda10Called = true;
                return 1;
            }, 0);
        REQUIRE(lambda1Called == lambda10Called);
        REQUIRE(expectedRet == ret);
    }

    { // Return type and default don't match exactly
        bool lambda11Called = false;
        int ret = guard.call_or(
            [&](const Pointee& a) -> int {
            lambda11Called = true;
            return 1;
        }, 0u);
        REQUIRE(lambda1Called == lambda11Called);
        REQUIRE(expectedRet == ret);
    }

    { // A guard is passed as multiple arguments.
        bool lambda12Called = false;
        int ret = guard.call_or(
            [&](const Pointee& a, const Pointee& b, const Pointee& c) -> int {
            lambda12Called = true;
            return 1;
        }, 0, guard, guard);
        REQUIRE(lambda1Called == lambda12Called);
        REQUIRE(expectedRet == ret);
    }
    { // Another guard is passed
        bool lambda12Called = false;
        int ret = guard.call_or(
            [&](const Pointee& a, const Pointee& b, const Pointee& c) -> int {
            lambda12Called = true;
            return 1;
        }, 0, otherGuard, otherGuard);
        REQUIRE(lambda1Called == lambda12Called);
        REQUIRE(expectedRet == ret);
    }
    { // The other guard is null.
        otherGuard.reset();
        bool lambda13Called = false;
        int ret = guard.call_or(
            [&](const Pointee& a, const Pointee& b, const Pointee& c) -> int {
            lambda13Called = true;
            return 1;
        }, 0, otherGuard, otherGuard);
        REQUIRE_FALSE(lambda13Called);
        REQUIRE(0 == ret);
    }

    return lambda1Called;
}

template <typename T>
bool pointee_is_accessible(ptr_guard<T>& guard) {
    bool lambda1Called = const_pointee_is_accessible(guard);

    bool lambda2Called = false;
    guard.call([&](Pointee& a) { lambda2Called = true; });
    REQUIRE(lambda1Called == lambda2Called);

    bool lambda4Called = false;
    ptr_guard<Pointee*> otherGuard;
    guard.call(
        [&](Pointee& a, int one, const Pointee& b, int two, const Pointee& c, int three) {
        lambda4Called = true;
    }, 1, otherGuard, 2, otherGuard, 3);
    REQUIRE_FALSE(lambda4Called);

    bool lambda5Called = false;
    Pointee other;
    otherGuard.reset(&other);
    guard.call(
        [&](Pointee& a, int one, const Pointee& b, int two, const Pointee& c, int three) {
        lambda5Called = true;
    }, 1, otherGuard, 2, otherGuard, 3);
    REQUIRE(lambda1Called == lambda5Called);

    bool lambda6Called = false;
    guard.call(
        [&](Pointee& a, int one, Pointee& b, int two, Pointee& c, int three) {
        lambda6Called = true;
    }, 1, otherGuard, 2, otherGuard, 3);
    REQUIRE(lambda1Called == lambda6Called);

    bool weakPointerParamLambdaCalled = false;
    shared_ptr<Pointee> sharedPtr(new Pointee);
    ptr_guard<weak_ptr<Pointee>> weakPtrGuard(sharedPtr);
    guard.call(
        [&](Pointee& a, Pointee& b) {
        weakPointerParamLambdaCalled = true;
    }, weakPtrGuard.lock());
    REQUIRE(lambda1Called == weakPointerParamLambdaCalled);

    int expectedRet = lambda1Called ? 1 : 0;
    { // Getting a return type or default from the call.
        bool lambda7Called = false;
        int ret = guard.call_or(
            [&](Pointee& a) -> int {
                lambda7Called = true;
                return 1;
            }, 0);
        REQUIRE(lambda1Called == lambda7Called);
        REQUIRE(expectedRet == ret);
    }
    { // Return type and default don't match exactly
        bool lambda8Called = false;
        int ret = guard.call_or(
            [&](Pointee& a) -> int {
            lambda8Called = true;
            return 1;
        }, 0u);
        REQUIRE(lambda1Called == lambda8Called);
        REQUIRE(expectedRet == ret);
    }
    { // A guard is passed as multiple arguments.
        bool lambda9Called = false;
        int ret = guard.call_or(
            [&](Pointee& a, Pointee& b, Pointee& c) -> int {
            lambda9Called = true;
            return 1;
        }, 0u, guard, guard);
        REQUIRE(lambda1Called == lambda9Called);
        REQUIRE(expectedRet == ret);
    }
    { // Another guard is passed
        bool lambda10Called = false;
        int ret = guard.call_or(
            [&](Pointee& a, Pointee& b, Pointee& c) -> int {
            lambda10Called = true;
            return 1;
        }, 0, otherGuard, otherGuard);
        REQUIRE(lambda1Called == lambda10Called);
        REQUIRE(expectedRet == ret);
    }
    { // The other guard is null.
        otherGuard.reset();
        bool lambda11Called = false;
        int ret = guard.call_or(
            [&](Pointee& a, Pointee& b, Pointee& c) -> int {
            lambda11Called = true;
            return 1;
        }, 0, otherGuard, otherGuard);
        REQUIRE_FALSE(lambda11Called);
        REQUIRE(0 == ret);
    }

    return lambda1Called;
}

template <typename T>
bool ptr_guards_and_contents_are_passed_by_reference(const ptr_guard<T>& guard) {
    TestContext context;
    guard.call([&](const Pointee& a) { });
    REQUIRE(0 == context.pointeeDestructorCalls);

    guard.call([&](Pointee& a) { });
    REQUIRE(0 == context.pointeeDestructorCalls);

    return true;
}

TEST_CASE(R"standardese(
20.8.x      Class template ptr_guard
1   A pointer guard is an object which owns a pointer and allows pointer dereferencing only through a valid pointer.
    More precisely, a pointer guard is an object g that stores a pointer p to a second object h or p may be a nullptr.
    The pointer guard will allow access to o only when p is pointing to h.
2   Ownership of h is inherited from the ownership semantics of template parameter pointer p.
3   Each object of type U instantiated from the ptr_guard template specified in the subclause will have the same MoveConstructable,
    MoveAssignable, CopyConstructable and CopyAssignable semantics as the template parameter it was instantiated with.

    template <class T>
    class ptr_guard {
    public:
        typedef (see note 4) pointer;
        typedef (see note 5) element_type;

    public:
        // 20.8.x, constructors
        constexpr ptr_guard() noexcept;

        template <class P>
        ptr_guard(P other) noexcept;

        template <class P>
        ptr_guard(ptr_guard<P> const& other) noexcept;
        template <class P>
        ptr_guard(ptr_guard<P>&& other) noexcept;

        ptr_guard(const ptr_guard& other, see note 6);
        ptr_guard(ptr_guard&& other, see note 6);

        // 20.8.x, destructor
        ~ptr_guard();

        // 20.8.x, assignment
        template <class P>
        ptr_guard& operator =(P other) noexcept;

        template <class P>
        ptr_guard& operator =(ptr_guard<P> const& other) noexcept;
        template <class P>
        ptr_guard& operator =(ptr_guard<P>&& other) noexcept;

        ptr_guard& operator =(const ptr_guard& other, see note 6) noexcept;
        ptr_guard& operator =(ptr_guard&& other, see note 6) noexcept;

        // 20.8.x, observers
        operator bool() const noexcept;

        auto use_count() const noexcept -> (see note 7);

        template<class P>
        bool owner_before(const P& other) const noexcept;
        template<class Y>
        bool owner_before(const ptr_guard<Y>& other) const noexcept;

        template <class Deleter = (see note 8)>
        Deleter& get_deleter() noexcept;

        template <class Deleter = (see note 8)>
        const Deleter& get_deleter() const noexcept;

        template <typename L = (see note 9)>
        auto lock() const noexcept -> ptr_guard<L>;

        void reset() noexcept;
        void reset(nullptr_t) noexcept;
        void reset(element_type* value) noexcept;

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
    };

4   If the type remove_reference<T>::type::pointer exists, then ptr_guard<T>::pointer shall be a synonym for
    remove_reference<T>::type::pointer. Otherwise ptr_guard<T>::pointer shall by a synonym for T*. The type
    ptr_guard<T>::pointer shall satisfy the requirements of NullablePointer (17.6.3.3).
5   If the type remove_reference<T>::type::element_type exists, then ptr_guard<T>::element_type shall be a
    synonym for remove_reference<T>::type::element_type. Otherwise element type shall be a synonym for
    remove_reference<T>::type.
6   The copy and move constructors, copy and move assignment operators should exist when the guarded pointer
    has matching copy and/or move constructors or assignment operators. This probably means these should be
    compiler generated.
7   If the method T::use_count exists then ptr_guard<T>::use_count will exist having a matching return type.
8   If the type ptr_guard<T>::pointer::deleter_type exists then ptr_guard<T>::get_deleter will exist and have
    ptr_guard<T>::pointer::deleter_type as the return type.
9   If the method T::lock accepting no arguments exists then ptr_guard<T>::lock will exist having a return
    type of a ptr_guard template instanciated with the return type of the lock method.
)standardese") {

    //static_assert(std::is_same<typename ptr_guard<Pointee>::pointer, Pointee*>::value, "Pointer type T is a T*");
    static_assert(std::is_same<typename ptr_guard<Pointee*>::pointer, Pointee*>::value, "Pointer type T* is a T*");
    static_assert(std::is_same<typename ptr_guard<unique_ptr<Pointee>>::pointer, unique_ptr<Pointee>>::value, "Pointer type unique_ptr<T> is a unique_ptr<T>");
    static_assert(std::is_same<typename ptr_guard<shared_ptr<Pointee>>::pointer, shared_ptr<Pointee>>::value, "Pointer type shared_ptr<T> is a shared_ptr<T>");
    static_assert(std::is_same<typename ptr_guard<weak_ptr<Pointee>>::pointer, weak_ptr<Pointee>>::value, "Pointer type shared_ptr<T> is a shared_ptr<T>");
}

TEST_CASE(R"standardese(
    // 20.8.x ptr_guard constructors
    constexpr ptr_guard() noexcept;
1       Requires: T shall satisfy the requirements of DefaultConstructible (Table 19), and that construction
shall not throw an exception.
2       Effects: Constructs a ptr_guard object that guards a default constructed pointer.

    template <class P>
    ptr_guard(P p) noexcept;
3       Effects: Equivalent to invoking constructor of the guarded pointer with the parameter p.

    template <class P>
    ptr_guard(ptr_guard<P> const& other) noexcept;
4       Effects: Equivalent to invoking the copy constructor on the guarded pointer with the pointer
        guarded by other.

    template <class P>
    ptr_guard(ptr_guard<P>&& other) noexcept;
5       Effects: Equivalent to invoking the move constructor on the guarded pointer with the
        pointer guarded by other.

    ptr_guard(const ptr_guard& other);
    ptr_guard(ptr_guard&& other);
6       Defined where copy and/or move constructors are defined for the guarded pointer. In practice should be
        implicitely defined by the compiler for an implementation where the guarded pointer is a member
        variable.
)standardese") {
    {
        ptr_guard<Pointee*> guard;
        REQUIRE(!guard);
    }
    {
        ptr_guard<unique_ptr<Pointee>> guard;
        REQUIRE(!guard);
    }
    {
        ptr_guard<shared_ptr<Pointee>> guard;
        REQUIRE(!guard);
    }
    {
        ptr_guard<weak_ptr<Pointee>> guard;
        REQUIRE(!guard);
    }
}

TEST_CASE(R"standardese(
    // ptr_guard destructor
    ~ptr_guard();
1       Effects: Equivalent to invoking the destructor of the guarded pointer.
)standardese") {

    TestContext context;

    Pointee stackPointee(1);
    {
        ptr_guard<Pointee*> observer(&stackPointee);
    }
    REQUIRE(0 == context.pointeeDestructorCalls);

    {
        ptr_guard<unique_ptr<Pointee>> owner(new Pointee(1));
    }
    REQUIRE(1 == context.pointeeDestructorCalls);

    shared_ptr<Pointee> owner(new Pointee(1));
    {
        ptr_guard<shared_ptr<Pointee>> other(owner);
        REQUIRE(2 == owner.use_count());
    }
    REQUIRE(1 == owner.use_count());
}

TEST_CASE(R"standardese(
    // 20.8.x ptr_guard assignment
    template <class P>
    ptr_guard& operator =(P p) noexcept;
1       Effects: Equivalent to invoking assignment operator on the guarded pointer with the parameter p.

    template <class P>
    ptr_guard& operator =(ptr_guard<P> const& other) noexcept;
2       Effects: Equivalent to invoking copy assignment operator on the guarded pointer with the pointer
        guarded by other.

    template <class P>
    ptr_guard& operator =(ptr_guard<P>&& other) noexcept;
3       Effects: Equivalent to invoking move assignment operator on the guarded pointer with the pointer
        guarded by other.

    ptr_guard& operator =(const ptr_guard& other) noexcept;
    ptr_guard& operator =(ptr_guard&& other) noexcept;
4       Defined where copy and/or move assignment operators are defined for the guarded pointer. In
        practice should be implicitely defined by the compiler for an implementation where the guarded
        pointer is a member variable.
)standardese") {
    {
        Pointee p;
        ptr_guard<Pointee*> guard;
        guard = &p;
        REQUIRE(guard);
    }
    {
        ptr_guard<unique_ptr<Pointee>> guard;
        ptr_guard<unique_ptr<Pointee>> other(new Pointee);
        guard = std::move(other);
        REQUIRE(guard);
        REQUIRE(!other);
    }
    {
        ptr_guard<shared_ptr<Pointee>> guard;
        ptr_guard<shared_ptr<Pointee>> other(new Pointee);
        guard = other;
        REQUIRE(guard);
        REQUIRE(other);
    }
    {
        ptr_guard<weak_ptr<Pointee>> guard;
        ptr_guard<shared_ptr<Pointee>> owner(new Pointee);
        ptr_guard<weak_ptr<Pointee>> other(owner);
        guard = other;
        REQUIRE(guard);
        REQUIRE(other);
    }

}

TEST_CASE(R"standardese(
    // ptr_guard observers
    operator bool() const noexcept;
1       Returns:Equivalent of operator bool on the guarded pointer.
)standardese") {

    Pointee stackPointee(1);
    {
        ptr_guard<Pointee*> observer(&stackPointee);
        REQUIRE(observer);
        observer = nullptr;
        REQUIRE(!observer);
    }

    {
        ptr_guard<unique_ptr<Pointee>> owner(new Pointee(1));
        REQUIRE(owner);
        owner.reset();
        REQUIRE(!owner);
    }
    {
        ptr_guard<shared_ptr<Pointee>> owner(new Pointee(1));
        REQUIRE(owner);
        owner.reset();
        REQUIRE(!owner);
    }
}

TEST_CASE(R"standardese(
    template <typename L>
    auto lock() const noexcept -> ptr_guard<L>;
2       Returns:Equivalent to constructing a ptr_guard on the result of calling lock on the guarded
        pointer.
3       Note:Only available if guarded pointer has a lock method accepting no arguments and
        returning a type for which a pointer guard constructor can be invoked.
)standardese") {
    shared_ptr<Pointee> owner(new Pointee(1));
    ptr_guard<weak_ptr<Pointee>> observer(owner);

    {
        ptr_guard<shared_ptr<Pointee>> locked = observer.lock();
        REQUIRE(locked);
    }
    {
        owner.reset();
        ptr_guard<shared_ptr<Pointee>> locked = observer.lock();
        REQUIRE(!locked);
    }
}

TEST_CASE(
    "    // ptr_guard modifiers \n"
    "    void reset(see note 2) noexcept;\n"
    "1.      Effects:The result of calling reset on the guarded pointer with the same arguments.\n"
    "2.      Note:Only available if the guarded pointer defines T::reset.\n\n") {
    TestContext context;
    {
        ptr_guard<unique_ptr<Pointee>> owner(new Pointee(1));

        owner.reset(new Pointee(2));
        REQUIRE(owner);
        REQUIRE(1 == context.pointeeDestructorCalls);

        owner.reset();
        REQUIRE(!owner);
        REQUIRE(2 == context.pointeeDestructorCalls);
    }
    {
        context.pointeeDestructorCalls = 0;
        ptr_guard<shared_ptr<Pointee>> owner(new Pointee(1));

        owner.reset(new Pointee(2));
        REQUIRE(owner);
        REQUIRE(1 == context.pointeeDestructorCalls);

        owner.reset();
        REQUIRE(!owner);
        REQUIRE(2 == context.pointeeDestructorCalls);
    }
}

TEST_CASE(
    "    T* release() noexcept;\n"
    "3.      Returns:The result of calling release on the guarded pointer.\n"
    "4.      Note:Only available if the guarded pointer defines T::release.\n\n") {
    TestContext context;
    Pointee pointee(1);
    {
        ptr_guard<unique_ptr<Pointee>> owner(&pointee);

        REQUIRE(owner.release());
        REQUIRE(0 == context.pointeeDestructorCalls);
    }
    REQUIRE(0 == context.pointeeDestructorCalls);
}

TEST_CASE(
    "20.8.x     Class template specialization ptr_guard<weak_ptr> \n\n") { }

TEST_CASE(
    "    // ptr_guard<weak_ptr> constructors \n"
    "    constexpr ptr_guard() noexcept;\n"
    "1       Effects:Constructs a ptr_guard for an internally constructed empty weak_ptr<T>.\n") {

}

TEST_CASE("Using a ptr_guard<T*>") {
    SECTION("A default constructed ptr_guard") {
        ptr_guard<Pointee*> guard;

        REQUIRE(!guard);
        REQUIRE(!pointee_is_accessible(guard));
        REQUIRE(ptr_guards_and_contents_are_passed_by_reference(guard));
    }
    SECTION("A ptr_guard constructed with a non null pointer") {
        Pointee pointee, pointee1;
        ptr_guard<Pointee*> guard(&pointee);

        REQUIRE(guard);
        REQUIRE(pointee_is_accessible(guard));
        REQUIRE(ptr_guards_and_contents_are_passed_by_reference(guard));

        SECTION("After reset of the pointer guard to a different pointee.") {
            guard = &pointee1;

            REQUIRE(guard);
            REQUIRE(pointee_is_accessible(guard));
        }
        SECTION("After reset of the pointer guard.") {
            guard = nullptr;

            REQUIRE(!guard);
            REQUIRE(!pointee_is_accessible(guard));
        }
    }
    SECTION("A ptr_guard for a type Derived from Pointee type") {
        DerivedFromPointee pointee;
        ptr_guard<DerivedFromPointee*> guard(&pointee);

        REQUIRE(guard);
        REQUIRE(pointee_is_accessible(guard));
        REQUIRE(ptr_guards_and_contents_are_passed_by_reference(guard));

        SECTION("After reset of the pointer guard.") {
            guard = nullptr;

            REQUIRE(!guard);
            REQUIRE(!pointee_is_accessible(guard));
            REQUIRE(ptr_guards_and_contents_are_passed_by_reference(guard));
        }
    }
}

TEST_CASE("Using a ptr_guard<unique_ptr>") {
    SECTION("A default constructed ptr_guard") {
        ptr_guard<unique_ptr<Pointee>> guard;

        REQUIRE(!guard);
        REQUIRE(!pointee_is_accessible(guard));
        REQUIRE(ptr_guards_and_contents_are_passed_by_reference(guard));
    }
    SECTION("A ptr_guard constructed with a non null pointer") {
        ptr_guard<unique_ptr<Pointee>> guard(new Pointee);

        REQUIRE(guard);
        REQUIRE(pointee_is_accessible(guard));
        REQUIRE(ptr_guards_and_contents_are_passed_by_reference(guard));

        SECTION("After reset of the pointer guard to a different pointee.") {
            guard.reset(new Pointee);

            REQUIRE(guard);
            REQUIRE(pointee_is_accessible(guard));
        }
        SECTION("After reset of the pointer guard.") {
            guard.reset();

            REQUIRE(!guard);
            REQUIRE(!pointee_is_accessible(guard));
        }
    }
}

TEST_CASE("Using a ptr_guard<shared_ptr>") {
    SECTION("A default constructed ptr_guard") {
        ptr_guard<shared_ptr<Pointee>> guard;

        REQUIRE(!guard);
        REQUIRE(!pointee_is_accessible(guard));
        REQUIRE(ptr_guards_and_contents_are_passed_by_reference(guard));
    }
    SECTION("A ptr_guard constructed with a non null pointer") {
        ptr_guard<shared_ptr<Pointee>> guard(new Pointee);

        REQUIRE(guard);
        REQUIRE(pointee_is_accessible(guard));
        REQUIRE(ptr_guards_and_contents_are_passed_by_reference(guard));

        SECTION("After reset of the pointer guard to a different pointee.") {
            guard.reset(new Pointee);

            REQUIRE(guard);
            REQUIRE(pointee_is_accessible(guard));
        }
        SECTION("After reset of the pointer guard.") {
            guard.reset();

            REQUIRE(!guard);
            REQUIRE(!pointee_is_accessible(guard));
        }
    }
}

TEST_CASE("Using a ptr_guard<weak_ptr>") {
    SECTION("A default constructed ptr_guard") {
        ptr_guard<weak_ptr<Pointee>> guard;

        REQUIRE(!guard.lock());
        REQUIRE(!pointee_is_accessible(guard.lock()));
        REQUIRE(ptr_guards_and_contents_are_passed_by_reference(guard.lock()));
    }
    SECTION("A ptr_guard constructed with a non null pointer") {
        shared_ptr<Pointee> owner(new Pointee);
        ptr_guard<weak_ptr<Pointee>> guard(owner);

        REQUIRE(guard.lock());
        REQUIRE(pointee_is_accessible(guard.lock()));
        REQUIRE(ptr_guards_and_contents_are_passed_by_reference(guard.lock()));

        SECTION("After reset of the pointer guard to a different pointee.") {
            owner.reset(new Pointee);
            guard = owner;

            REQUIRE(guard.lock());
            REQUIRE(pointee_is_accessible(guard.lock()));
        }
        SECTION("After reset of the pointer guard.") {
            guard.reset();

            REQUIRE(!guard.lock());
            REQUIRE(!pointee_is_accessible(guard.lock()));
        }
    }
}

TEST_CASE("A raw pointer guard can be implicitely constructed") {
    bool functionCalled = false;
    auto func = [&](ptr_guard<Pointee*> guard) {
        functionCalled = true;
    };
    Pointee pointee;
    func(&pointee);
    REQUIRE(functionCalled);
}

TEST_CASE("Swapping a ptr_guard<unique_ptr> with a unique_ptr") {
    ptr_guard<unique_ptr<Pointee>> guard(new Pointee(1));
    unique_ptr<Pointee> other(new Pointee(2));
    guard.swap(other);
    REQUIRE(guard);
    REQUIRE(other);
    REQUIRE(1 == other->identifier);
    guard.call([](const Pointee& pointee) { REQUIRE(pointee.identifier == 2); });
}

TEST_CASE("Swapping a ptr_guard<unique_ptr> with a ptr_guard<unique_ptr>") {
    ptr_guard<unique_ptr<Pointee>> guard(new Pointee(1));
    ptr_guard<unique_ptr<Pointee>> other(new Pointee(2));
    guard.swap(other);
    REQUIRE(guard);
    REQUIRE(other);
    other.call([](const Pointee& pointee) { REQUIRE(pointee.identifier == 1); });
    guard.call([](const Pointee& pointee) { REQUIRE(pointee.identifier == 2); });
}

TEST_CASE("Swapping a ptr_guard<shared_ptr> with a shared_ptr") {
    ptr_guard<shared_ptr<Pointee>> guard(new Pointee(1));
    shared_ptr<Pointee> other(new Pointee(2));
    guard.swap(other);
    REQUIRE(guard);
    REQUIRE(other);
    REQUIRE(1 == other->identifier);
    guard.call([](const Pointee& pointee) { REQUIRE(pointee.identifier == 2); });
}

TEST_CASE("Swapping a ptr_guard<shared_ptr> with a ptr_guard<shared_ptr>") {
    ptr_guard<shared_ptr<Pointee>> guard(new Pointee(1));
    ptr_guard<shared_ptr<Pointee>> other(new Pointee(2));
    guard.swap(other);
    REQUIRE(guard);
    REQUIRE(other);
    other.call([](const Pointee& pointee) { REQUIRE(pointee.identifier == 1); });
    guard.call([](const Pointee& pointee) { REQUIRE(pointee.identifier == 2); });
}

TEST_CASE("Swapping a ptr_guard<weak_ptr> with a weak_ptr") {
    shared_ptr<Pointee> owner(new Pointee(1));
    ptr_guard<weak_ptr<Pointee>> guard(owner);
    shared_ptr<Pointee> otherOwner(new Pointee(2));
    weak_ptr<Pointee> other(otherOwner);
    guard.swap(other);
    REQUIRE(guard.lock());
    REQUIRE(other.lock());
    REQUIRE(1 == other.lock()->identifier);
    guard.lock().call([](const Pointee& pointee) { REQUIRE(pointee.identifier == 2); });
}

TEST_CASE("Swapping a ptr_guard<weak_ptr> with a ptr_guard<weak_ptr>") {
    shared_ptr<Pointee> owner(new Pointee(1));
    ptr_guard<weak_ptr<Pointee>> guard(owner);
    shared_ptr<Pointee> otherOwner(new Pointee(2));
    ptr_guard<weak_ptr<Pointee>> other(otherOwner);
    guard.swap(other);
    REQUIRE(guard.lock());
    REQUIRE(other.lock());
    other.lock().call([](const Pointee& pointee) { REQUIRE(pointee.identifier == 1); });
    guard.lock().call([](const Pointee& pointee) { REQUIRE(pointee.identifier == 2); });
}

TEST_CASE("Move construct a ptr_guard<unique_ptr>") {
    ptr_guard<unique_ptr<Pointee>> other(new Pointee);
    ptr_guard<unique_ptr<Pointee>> guard(std::move(other));
    REQUIRE(guard);
    REQUIRE(!other);
}

TEST_CASE("Construct a ptr_guard<unique_ptr> from a moved unique_ptr") {
    unique_ptr<Pointee> other(new Pointee);
    ptr_guard<unique_ptr<Pointee>> guard(std::move(other));
    REQUIRE(guard);
    REQUIRE(!other);
}

TEST_CASE("Assignment of a ptr_guard<unique_ptr> from nullptr") {
    TestContext context;
    ptr_guard<unique_ptr<Pointee>> guard(new Pointee);
    guard = nullptr;
    REQUIRE(!guard);
    REQUIRE(1 == context.pointeeDestructorCalls);
}

TEST_CASE("Assignment of a ptr_guard<unique_ptr> from unique_ptr") {
    TestContext context;
    ptr_guard<unique_ptr<Pointee>> guard(new Pointee(1));
    unique_ptr<Pointee> other(new Pointee(2));

    guard = std::move(other);

    REQUIRE(1 == context.pointeeDestructorCalls);
    REQUIRE(guard);
    guard.call([](const Pointee& pointee) { REQUIRE(pointee.identifier == 2); });
    
}

TEST_CASE("Assignment of a ptr_guard<unique_ptr> from ptr_guard") {
    TestContext context;
    ptr_guard<unique_ptr<Pointee>> guard(new Pointee(1));
    ptr_guard<unique_ptr<Pointee>> other(new Pointee(2));

    guard = std::move(other);

    REQUIRE(1 == context.pointeeDestructorCalls);
    REQUIRE(guard);
    guard.call([](const Pointee& pointee) { REQUIRE(pointee.identifier == 2); });
}

TEST_CASE("Move construct a ptr_guard<shared_ptr>") {
    ptr_guard<shared_ptr<Pointee>> other(new Pointee);
    ptr_guard<shared_ptr<Pointee>> guard(std::move(other));
    REQUIRE(guard);
    REQUIRE(!other);
}

TEST_CASE("Construct a ptr_guard<shared_ptr> from a moved unique_ptr") {
    shared_ptr<Pointee> other(new Pointee);
    ptr_guard<shared_ptr<Pointee>> guard(std::move(other));
    REQUIRE(guard);
    REQUIRE(!other);
}

TEST_CASE("Assignment of a ptr_guard<shared_ptr> from nullptr") {
    TestContext context;
    ptr_guard<shared_ptr<Pointee>> guard(new Pointee);
    guard = nullptr;
    REQUIRE(!guard);
    REQUIRE(1 == context.pointeeDestructorCalls);
}

TEST_CASE("Copy assignment of a ptr_guard<shared_ptr> from shared_ptr") {
    TestContext context;
    ptr_guard<shared_ptr<Pointee>> guard(new Pointee(1));
    shared_ptr<Pointee> other(new Pointee(2));

    guard = other;

    REQUIRE(1 == context.pointeeDestructorCalls);
    REQUIRE(guard);
    guard.call([](const Pointee& pointee) { REQUIRE(pointee.identifier == 2); });

}

TEST_CASE("Copy assignment of a ptr_guard<shared_ptr> from ptr_guard") {
    TestContext context;
    ptr_guard<shared_ptr<Pointee>> guard(new Pointee(1));
    ptr_guard<shared_ptr<Pointee>> other(new Pointee(2));

    guard = other;

    REQUIRE(1 == context.pointeeDestructorCalls);
    REQUIRE(guard);
    guard.call([](const Pointee& pointee) { REQUIRE(pointee.identifier == 2); });
}

TEST_CASE("Assignment of a ptr_guard<shared_ptr> from shared_ptr") {
    TestContext context;
    ptr_guard<shared_ptr<Pointee>> guard(new Pointee(1));
    shared_ptr<Pointee> other(new Pointee(2));

    guard = std::move(other);

    REQUIRE(1 == context.pointeeDestructorCalls);
    REQUIRE(guard);
    guard.call([](const Pointee& pointee) { REQUIRE(pointee.identifier == 2); });

}

TEST_CASE("Assignment of a ptr_guard<shared_ptr> from ptr_guard") {
    TestContext context;
    ptr_guard<shared_ptr<Pointee>> guard(new Pointee(1));
    ptr_guard<shared_ptr<Pointee>> other(new Pointee(2));

    guard = std::move(other);

    REQUIRE(1 == context.pointeeDestructorCalls);
    REQUIRE(guard);
    guard.call([](const Pointee& pointee) { REQUIRE(pointee.identifier == 2); });
}

/*TEST_CASE("Static cast of a ptr_guard<shared_ptr>") {
    ptr_guard<DerivedFromPointee, shared_ptr<DerivedFromPointee>> guard(new DerivedFromPointee);
    auto other = std::static_pointer_guard_cast<Pointee, DerivedFromPointee>(guard);
    REQUIRE(guard);
}

TEST_CASE("Dynamic cast of a ptr_guard<shared_ptr>") {
    ptr_guard<Pointee, shared_ptr<Pointee>> guard(new DerivedFromPointee);
    auto other = std::dynamic_pointer_cast<DerivedFromPointee, Pointee>(guard);
    REQUIRE(guard);
}

TEST_CASE("Const cast of a ptr_guard<shared_ptr>") {
    const ptr_guard<Pointee, shared_ptr<Pointee>> guard(new Pointee);
    auto other = std::const_pointer_cast<Pointee, Pointee>(guard);
    REQUIRE(guard);
}*/

#ifdef __CPP17_SUPPORT__
TEST_CASE("Reinterpret cast of a ptr_guard<shared_ptr>") {
    ptr_guard<Pointee, shared_ptr<Pointee>> guard(new Pointee);
    auto other = std::reinterpret_pointer_cast<DerivedFromPointee>(guard);
    REQUIRE(guard);
}
#endif
