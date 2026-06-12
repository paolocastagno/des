SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS = $(patsubst %.cpp,%.d,$(SOURCES))
HEADERS = $(wildcard src/libdes_*.hpp) src/incbeta.hpp
OS =$(shell uname)
CXXSTD ?= c++23
CSTD = -std=$(CXXSTD)
BUILD ?= release
TEST_BIN ?= test/test
TEST_EVENTS ?= 100000
TEST_RUNS ?= 5
LINK_INSTALLED ?= 1
PROFILE_DIR ?= profile
PROFILE_WARMUP_SECONDS ?= 0.2
PROFILE_SECONDS ?= 2
PROFILE_INTERVAL_MS ?= 1

ifneq ($(origin DEBUG),undefined)
$(error DEBUG is no longer supported; use BUILD=debug or BUILD=release)
endif

ifeq ($(OS),Darwin)
	SOEXT = dylib
	SODIR = /usr/local/lib
	HDIR = /usr/local/include
	PICFLAGS =
	LDFLAGS = -install_name $(SODIR)/libdes.dylib
	TEST_RPATH_FLAGS = -Wl,-rpath,$(CURDIR)
	TIME_CMD = /usr/bin/time -l
else ifeq ($(OS),Linux)
	SOEXT = so
	SODIR = /usr/lib
	HDIR = /usr/include
	PICFLAGS = -fPIC
	LDFLAGS = -fPIC
	TEST_RPATH_FLAGS = -Wl,-rpath,$(CURDIR)
	TIME_CMD = /usr/bin/time -v
else
$(error unsupported OS: $(OS))
endif

ifeq ($(LINK_INSTALLED),1)
	TEST_LIB_PREREQ =
	TEST_LINK_FLAGS = -L$(SODIR) -ldes
else
	TEST_LIB_PREREQ = libdes
	TEST_LINK_FLAGS = -L. -ldes $(TEST_RPATH_FLAGS)
endif

ifeq ($(BUILD),debug)
    BUILD_FLAGS = -DDEBUG -O0 -g -ggdb
else ifeq ($(BUILD),release)
    BUILD_FLAGS = -DNDEBUG -O3
else ifeq ($(BUILD),profile)
    BUILD_FLAGS = -DNDEBUG -O3 -g -fno-omit-frame-pointer
else
$(error BUILD must be debug, release, or profile)
endif
CXXFLAGS = $(CSTD) $(BUILD_FLAGS) $(PICFLAGS)
TEST_CXXFLAGS = -DDES_TEST_EVENTS=$(TEST_EVENTS) -DDES_TEST_RUNS=$(TEST_RUNS)

# Use local source headers, not installed ones
INCLUDES := -I src

# ADD MORE WARNINGS!
WARNING := -Wall -Wextra
# .PHONY means these rules get executed even if
# files of those names exist.
.PHONY: all debug release optimized profile-build test profile-time profile-sample profile profile-build-local profile-time-local profile-sample-local profile-local clean clean-dep clean-lib clean-test clean-profile install uninstall
# The first rule is the default, ie. "make",
# "make all" and "make libdes" mean the same
all: libdes clean-dep
debug:
			$(MAKE) BUILD=debug clean clean-lib all
release:
			$(MAKE) BUILD=release clean clean-lib all
optimized: release
profile-build:
			$(MAKE) BUILD=profile LINK_INSTALLED=$(LINK_INSTALLED) clean clean-lib clean-test $(TEST_LIB_PREREQ) $(TEST_BIN)
test: $(TEST_LIB_PREREQ) $(TEST_BIN)
			./$(TEST_BIN)
profile-time: profile-build
			$(TIME_CMD) ./$(TEST_BIN)
profile: profile-time profile-sample
profile-build-local:
			$(MAKE) LINK_INSTALLED=0 profile-build
profile-time-local:
			$(MAKE) LINK_INSTALLED=0 profile-time
profile-sample-local:
			$(MAKE) LINK_INSTALLED=0 profile-sample
profile-local:
			$(MAKE) LINK_INSTALLED=0 profile
# Remove object files and dependences
clean:
			$(RM) $(OBJECTS) $(DEPENDS)
clean-dep:
			$(RM)  $(DEPENDS)
clean-lib:
			$(RM) libdes.$(SOEXT)
clean-test:
			$(RM) -r $(TEST_BIN) $(TEST_BIN).dSYM
clean-profile:
			$(RM) -r $(PROFILE_DIR)
install: libdes
			sudo install -d "$(SODIR)" "$(HDIR)"
			sudo install -m 0644 $(HEADERS) "$(HDIR)/"
			sudo install -m 0755 "libdes.$(SOEXT)" "$(SODIR)/"
ifneq ($(OS),Darwin)
			sudo ldconfig
endif
uninstall:
			sudo $(RM) "$(SODIR)/libdes.$(SOEXT)"
			sudo $(RM) "$(HDIR)/incbeta.hpp" "$(HDIR)"/libdes_*.hpp

# Linking the executable from the object files
libdes:  $(OBJECTS)
			$(CXX) $(WARNING) -shared $(LDFLAGS) $^ -o $@.$(SOEXT)
-include $(DEPENDS)

%.o: %.cpp Makefile
		$(CXX) $(WARNING) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

$(TEST_BIN): test/test.cpp Makefile $(TEST_LIB_PREREQ)
		$(CXX) $(WARNING) $(CXXFLAGS) $(TEST_CXXFLAGS) $(INCLUDES) $< $(TEST_LINK_FLAGS) -o $@
ifeq ($(OS),Darwin)
ifneq ($(LINK_INSTALLED),1)
		install_name_tool -change $(SODIR)/libdes.$(SOEXT) @rpath/libdes.$(SOEXT) $@
endif
endif

ifeq ($(OS),Darwin)
profile-sample: profile-build
			mkdir -p $(PROFILE_DIR)
			./$(TEST_BIN) > $(PROFILE_DIR)/test-output.txt & pid=$$!; sleep $(PROFILE_WARMUP_SECONDS); /usr/bin/sample $$pid $(PROFILE_SECONDS) $(PROFILE_INTERVAL_MS) -file $(PROFILE_DIR)/sample.txt; wait $$pid
else
profile-sample:
			@echo "profile-sample uses macOS sample; run profile-time on $(OS)."
endif
