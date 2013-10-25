TESTS = shrink_TEST

CPPFLAGS := -I$(SUBDIR)/../include -I$(GTEST_DIR)/include -I$(GTEST_DIR) -DGTEST_LANG_CXX11=1

shrink_TEST_SOURCES = main.cc oneof.cc owned_ptr.cc gtest-all.cc

shrink_TEST_LIBRARIES = -lpthread

# hack!
$(eval $(call add-dir,$(GENERATED_SOURCE_DIR)))

$(GENERATED_SOURCE_DIR)/gtest-all.cc: $(GTEST_DIR)/src/gtest-all.cc | $(GENERATED_SOURCE_DIR)
	cp $< $@
