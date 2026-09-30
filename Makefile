SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS = $(patsubst %.cpp,%.d,$(SOURCES))
HEADERS = $(wildcard src/libdes_*.hpp) src/incbeta.hpp
OS =$(shell uname)
CXXSTD ?= c++23
CSTD = -std=$(CXXSTD)
BUILD ?= release
TEST_BIN ?= test/test
TEST_TOLERANCE ?= 0.01
TEST_RUN_TOLERANCE ?= 0.05
TEST_MIN_RUNS ?= 10
TEST_MIN_CYCLES ?= 30
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
    # -flto lets the compiler inline across source files (e.g. the queue queries in node.cpp)
    BUILD_FLAGS = -DNDEBUG -O3 -flto
else ifeq ($(BUILD),profile)
    BUILD_FLAGS = -DNDEBUG -O3 -g -fno-omit-frame-pointer
else
$(error BUILD must be debug, release, or profile)
endif
CXXFLAGS = $(CSTD) $(BUILD_FLAGS) $(PICFLAGS)
TEST_CXXFLAGS = -DDES_TEST_TOLERANCE=$(TEST_TOLERANCE) -DDES_TEST_RUN_TOLERANCE=$(TEST_RUN_TOLERANCE) -DDES_TEST_MIN_RUNS=$(TEST_MIN_RUNS) -DDES_TEST_MIN_CYCLES=$(TEST_MIN_CYCLES)

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
ifneq ($(OS),Darwin)
			sudo ldconfig
endif

# Linking the executable from the object files
libdes:  $(OBJECTS)
			$(CXX) $(WARNING) $(BUILD_FLAGS) -shared $(LDFLAGS) $^ -o $@.$(SOEXT)
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

# ---------------------------------------------------------------------------
# Test pipeline (see docs/testing.md)
#
# Each configuration compiles the workspace sources (src/) together with the
# test suites into its own directory under $(BUILD_DIR): the installed library
# is never used.
#
#   make check            unit and regression tests
#   make validate         statistical validation against queueing theory
#   make check-sanitize   unit and regression tests under ASan+UBSan, [threads] under TSan
#   make coverage         line coverage of src/ (clang)
#   make bench            benchmark table (not pass/fail)
#   make golden           write the regression golden values for this platform
#   make ci               check, validate and check-sanitize
#   make clean-build      remove $(BUILD_DIR)
#
# TEST_ARGS passes options to the runner, e.g. make check TEST_ARGS=-v.
# To run a subset: build/check/des_tests "[network]" (see --help).
# ---------------------------------------------------------------------------
BUILD_DIR      ?= build
TEST_ARGS      ?=
SUITE_SOURCES   = $(wildcard test/harness/*.cpp test/unit/*.cpp test/regression/*.cpp test/validation/*.cpp)
SUITE_OBJECTS   = $(patsubst %.cpp,%.o,$(SOURCES) $(SUITE_SOURCES))
BENCH_OBJECTS   = $(patsubst %.cpp,%.o,$(SOURCES) test/bench/bench.cpp)
SUITE_INCLUDES  = -I src -I test/harness -I test/support
SUITE_DEFINES   = -DDES_GOLDEN_DIR=\"test/regression/golden\"
SUITE_THREADS   = -pthread

CHECK_FLAGS     = -O2 -g
SANITIZE_FLAGS  = -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=undefined
TSAN_FLAGS      = -O1 -g -fno-omit-frame-pointer -fsanitize=thread
COVERAGE_FLAGS  = -O0 -g -fprofile-instr-generate -fcoverage-mapping
BENCH_FLAGS     = -O3 -DNDEBUG -flto
COVERAGE_IGNORE = (test/|/usr/|/Library/|/opt/)

ifeq ($(OS),Darwin)
  LLVM_COV      ?= xcrun llvm-cov
  LLVM_PROFDATA ?= xcrun llvm-profdata
else
  LLVM_COV      ?= llvm-cov
  LLVM_PROFDATA ?= llvm-profdata
endif

# Rules of one configuration. $(1): name, $(2): name of the variable holding its flags
# (passed by name because the flags may contain commas).
define DES_SUITE
$(BUILD_DIR)/$(1)/%.o: %.cpp
	@mkdir -p $$(@D)
	$$(CXX) $$(CSTD) $$(WARNING) $$($(2)) $$(SUITE_THREADS) $$(SUITE_INCLUDES) $$(SUITE_DEFINES) -MMD -MP -c $$< -o $$@
$(BUILD_DIR)/$(1)/des_tests: $$(addprefix $(BUILD_DIR)/$(1)/,$$(SUITE_OBJECTS))
	$$(CXX) $$($(2)) $$(SUITE_THREADS) $$^ -o $$@
-include $$(wildcard $$(patsubst %.o,$(BUILD_DIR)/$(1)/%.d,$$(SUITE_OBJECTS) test/bench/bench.o))
endef
$(eval $(call DES_SUITE,check,CHECK_FLAGS))
$(eval $(call DES_SUITE,sanitize,SANITIZE_FLAGS))
$(eval $(call DES_SUITE,tsan,TSAN_FLAGS))
$(eval $(call DES_SUITE,coverage,COVERAGE_FLAGS))
$(eval $(call DES_SUITE,bench,BENCH_FLAGS))

$(BUILD_DIR)/bench/des_bench: $(addprefix $(BUILD_DIR)/bench/,$(BENCH_OBJECTS))
	$(CXX) $(BENCH_FLAGS) $(SUITE_THREADS) $^ -o $@

.PHONY: check validate check-sanitize coverage bench golden ci clean-build

check: $(BUILD_DIR)/check/des_tests
	./$< $(TEST_ARGS) "[unit]" "[regression]"

validate: $(BUILD_DIR)/check/des_tests
	./$< $(TEST_ARGS) "[validation]"

check-sanitize: $(BUILD_DIR)/sanitize/des_tests $(BUILD_DIR)/tsan/des_tests
	UBSAN_OPTIONS=print_stacktrace=1 ./$(BUILD_DIR)/sanitize/des_tests $(TEST_ARGS) "[unit]" "[regression]"
	TSAN_OPTIONS=halt_on_error=1 ./$(BUILD_DIR)/tsan/des_tests $(TEST_ARGS) "[threads]"

coverage: $(BUILD_DIR)/coverage/des_tests
	$(RM) $(BUILD_DIR)/coverage/*.profraw
	LLVM_PROFILE_FILE=$(BUILD_DIR)/coverage/des-%p.profraw ./$< "[unit]" "[regression]" "[validation]"
	$(LLVM_PROFDATA) merge -sparse $(BUILD_DIR)/coverage/*.profraw -o $(BUILD_DIR)/coverage/des.profdata
	$(LLVM_COV) report $< -instr-profile=$(BUILD_DIR)/coverage/des.profdata -ignore-filename-regex='$(COVERAGE_IGNORE)' | tee $(BUILD_DIR)/coverage/summary.txt
	$(LLVM_COV) show $< -instr-profile=$(BUILD_DIR)/coverage/des.profdata -ignore-filename-regex='$(COVERAGE_IGNORE)' -format=html -output-dir=$(BUILD_DIR)/coverage/html
	@echo "HTML report: $(BUILD_DIR)/coverage/html/index.html"

bench: $(BUILD_DIR)/bench/des_bench
	./$<

golden: $(BUILD_DIR)/check/des_tests
	./$< --update-golden "[regression]"
	@echo "Review the changes under test/regression/golden before committing them."

ci: check validate check-sanitize

clean-build:
	$(RM) -r $(BUILD_DIR)
