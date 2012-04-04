#include "oneof.hh"

#include <gtest/gtest.h>

using shrink::OneOf;
using shrink::when;

TEST(OneOfTest, OneOfTest)
{
    OneOf<int, std::string> o1(123);
    int result = 0;

    when(o1,
            [&](int & x) { result = x; },
            [&](const std::string & x) { FAIL() << "Called a string function for an int"; }
        );
    ASSERT_EQ(123, result);

    o1 = std::string("hello");

    when(o1,
            [&](int & x) { FAIL() << "Called an int function for a string"; },
            [&](const std::string & x) { result = x.length(); }
        );
    ASSERT_EQ(5, result);
}

struct Base
{
    virtual int f() { return 1; }
};

struct Derived1 : Base
{
    virtual int f() { return 2; }
};

struct Derived2 : Base
{
    virtual int f() { return 3; }
};

struct Derived3 : Derived1
{
    virtual int f() { return 4; }
};

TEST(OneOfTest, DerivedTypes)
{
    OneOf<Base, Derived1, Derived2, Derived3> one((Base()));
    int result = -1;

    result = when(one,
            [](Base &b) { return 1; },
            [](Derived1 &d) { return 2; },
            [](Derived2 &d) { return 3; }
        );
    ASSERT_EQ(1, result);

    one = Derived2();

    result = when(one,
            [](Base &b) { return 1; },
            [](Derived1 &d) { return 2; },
            [](Derived2 &d) { return 3; }
        );
    ASSERT_EQ(3, result);

    one = Derived3();

    result = when(one,
            [](Base &b) { return 1; },
            [](Derived1 &d) { return 2; },
            [](Derived2 &d) { return 3; }
        );
    ASSERT_EQ(2, result);

    result = when(one,
            [](Base &b) { return 1; },
            [](Derived1 &d) { return 2; },
            [](Derived3 &d) { return 3; }
        );
    ASSERT_EQ(3, result);

    // The actual type in one is Derived3
    result = when(one,
            [](Base & b) { return b.f(); }
        );
    ASSERT_EQ(4, result);
}

struct Derived4 : Base
{
    int i;

    Derived4(int i_) : i(i_) { }

    virtual int f() { return i; }
};

TEST(OneOfTest, Extract)
{
    OneOf<Base, Derived1, Derived4> oo((Base()));

    Base & b = shrink::extract<Base>(oo);
    ASSERT_EQ(1, b.f());

    oo = Derived1();
    Base & b2 = shrink::extract<Base>(oo);
    ASSERT_EQ(2, b2.f());

    oo = Derived4(35);
    Base & b3 = shrink::extract<Base>(oo);
    ASSERT_EQ(35, b3.f());

    when(oo, [](Base &) {}, [](Derived4 & d) { d.i = 23; });
    ASSERT_EQ(23, b3.f());
}

/*
TEST(OneOfTest, DerivedPointerTypes)
{
    OneOf<Base*, Derived1*, Derived2*, Derived3*> one(new Base());
    int result = -1;

    result = when(one,
            [](Base *b) { return 1; }
        );
}
*/
