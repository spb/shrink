TESTS = shrink_TEST

shrink_TEST_SOURCES = main.cc oneof.cc
shrink_TEST_LIBRARIES = -lgtest -lpthread
CPPFLAGS := -I$(SUBDIR)/../include

