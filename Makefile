SUBDIRS = src \
	  test

CXXFLAGS = -std=gnu++11

GTEST_DIR ?= ../gtest

include bs/bs.mk
