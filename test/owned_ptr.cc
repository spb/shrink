#include <shrink/owned_ptr.hh>

#include <gtest/gtest.h>

using shrink::owned_ptr;
using shrink::handle_ptr;
using namespace shrink::exceptions;

TEST(OwnedPtrTest, OwnedPtrTest)
{
    owned_ptr<int> p (new int(3));
    ASSERT_TRUE(p.good());
    ASSERT_EQ(*p, 3);
}

TEST(OwnedPtrTest, Releasing)
{
    owned_ptr<int> p (new int(3));
    ASSERT_TRUE(p.good());
    p.release();
    ASSERT_FALSE(p.good());

    try {
        p.release();
        FAIL() << "Second release didn't throw";
    }
    catch (ReleasedInvalidOwnedPtrException)
    {
        SUCCEED() << "Second release threw as expected";
    }
}

TEST(OwnedPtrTest, ReleaseReallyDoesRelease)
{
    struct ZeroOnDestruction
    {
        int *p;
        ZeroOnDestruction(int *x) : p(x) { }
        ~ZeroOnDestruction() { *p = 0; }
    };

    int x = 3;
    {
        owned_ptr<ZeroOnDestruction> p(new ZeroOnDestruction(&x));
        ASSERT_TRUE(p.good());
        ASSERT_EQ(x, 3);
    }
    ASSERT_EQ(x, 0);
}

TEST(OwnedPtrTest, DerefReleasedPtrThrows)
{
    owned_ptr<int> p(new int (3));
    p.release();

    try
    {
        int x = *p;
        FAIL() << "Dereferencing a released pointer didn't fail";
    }
    catch (InvalidOwnedPtrException)
    {
    }
}

TEST(OwnedPtrTest, SimpleHandles)
{
    owned_ptr<int> p (new int(3));
    ASSERT_TRUE(p.good());

    {
        handle_ptr<int> h(p);
        ASSERT_EQ(*h, 3);
    }
}

TEST(OwnedPtrTest, ReleaseWithHandlesExisting)
{
    owned_ptr<int> p (new int(3));
    handle_ptr<int> h(p);

    try
    {
        p.release();
        FAIL() << "Release with handles existing didn't throw";
    }
    catch (ReferencesStillExistException)
    {
    }
}

TEST(OwnedPtrTest, ReleaseAfterHandleReleased)
{
    owned_ptr<int> p (new int(3));
    handle_ptr<int> h(p);

    ASSERT_TRUE(p.good());
    ASSERT_TRUE(h.good());

    h.release();
    ASSERT_TRUE(p.good());
    ASSERT_FALSE(h.good());

    p.release();
    ASSERT_FALSE(p.good());
}

TEST(OwnedPtrTest, ReleaseAfterMultipleHandlesReleased)
{
    owned_ptr<int> p (new int(3));
    handle_ptr<int> h1(p);
    handle_ptr<int> h2(p);

    ASSERT_TRUE(p.good());
    ASSERT_TRUE(h1.good());
    ASSERT_TRUE(h2.good());

    h1.release();

    ASSERT_TRUE(p.good());
    ASSERT_FALSE(h1.good());
    ASSERT_TRUE(h2.good());

    h2.release();

    ASSERT_TRUE(p.good());
    ASSERT_FALSE(h1.good());
    ASSERT_FALSE(h2.good());

    p.release();

    ASSERT_FALSE(p.good());
}

TEST(OwnedPtrTest, CopyingHandles)
{
    owned_ptr<int> p(new int(3));
    handle_ptr<int> h1(p);
    handle_ptr<int> h2(h1);

    ASSERT_EQ(*p, 3);
    ASSERT_EQ(*h1, 3);
    ASSERT_EQ(*h2, 3);

    ASSERT_TRUE(p.good());
    ASSERT_TRUE(h1.good());
    ASSERT_TRUE(h2.good());

    h2.release();

    ASSERT_TRUE(p.good());
    ASSERT_TRUE(h1.good());
    ASSERT_FALSE(h2.good());

    h1.release();

    ASSERT_TRUE(p.good());
    ASSERT_FALSE(h1.good());
    ASSERT_FALSE(h2.good());

    p.release();

    ASSERT_FALSE(p.good());
    ASSERT_FALSE(h1.good());
    ASSERT_FALSE(h2.good());
}

TEST(OwnedPtrTest, ReleaseReleasedHandleThrows)
{
    owned_ptr<int> p(new int(3));
    handle_ptr<int> h(p);

    h.release();

    try
    {
        h.release();
        FAIL() << "Dereferencing released handle didn't throw";
    }
    catch (ReleasedInvalidHandlePtrException)
    {
    }
}

TEST(OwnedPtrTest, DerefReleasedHandleThrows)
{
    owned_ptr<int> p(new int(3));
    handle_ptr<int> h(p);

    h.release();

    try
    {
        int x = *h;
        FAIL() << "Dereferencing released handle didn't throw";
    }
    catch (InvalidHandlePtrException)
    {
    }
}



// vim: set sw=4 sts=4 et :
