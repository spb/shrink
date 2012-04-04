TESTS = test_TEST

test_TEST_SOURCES = main.cc oneof.cc
test_TEST_LIBRARIES = -lgtest -lpthread
CPPFLAGS = -Iinclude

