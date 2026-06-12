SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS = $(patsubst %.cpp,%.d,$(SOURCES))
HEADERS = $(wildcard src/libdes_*.hpp) src/incbeta.hpp
OS =$(shell uname)
CXXSTD ?= c++23
CSTD = -std=$(CXXSTD)
BUILD ?= release

ifneq ($(origin DEBUG),undefined)
$(error DEBUG is no longer supported; use BUILD=debug or BUILD=release)
endif

ifeq ($(OS),Darwin)
	SOEXT = dylib
	SODIR = /usr/local/lib
	HDIR = /usr/local/include
	PICFLAGS =
	LDFLAGS = -install_name $(SODIR)/libdes.dylib
else ifeq ($(OS),Linux)
	SOEXT = so
	SODIR = /usr/lib
	HDIR = /usr/include
	PICFLAGS = -fPIC
	LDFLAGS = -fPIC
else
$(error unsupported OS: $(OS))
endif

ifeq ($(BUILD),debug)
    BUILD_FLAGS = -DDEBUG -O0 -g -ggdb
else ifeq ($(BUILD),release)
    BUILD_FLAGS = -DNDEBUG -O3
else
$(error BUILD must be debug or release)
endif
CXXFLAGS = $(CSTD) $(BUILD_FLAGS) $(PICFLAGS)

# Use local source headers, not installed ones
INCLUDES := -I src

# ADD MORE WARNINGS!
WARNING := -Wall -Wextra
# .PHONY means these rules get executed even if
# files of those names exist.
.PHONY: all debug release optimized clean clean-dep clean-lib install uninstall
# The first rule is the default, ie. "make",
# "make all" and "make libdes" mean the same
all: libdes clean-dep
debug:
			$(MAKE) BUILD=debug clean clean-lib all
release:
			$(MAKE) BUILD=release clean clean-lib all
optimized: release
# Remove object files and dependences
clean:
			$(RM) $(OBJECTS) $(DEPENDS)
clean-dep:
			$(RM)  $(DEPENDS)
clean-lib:
			$(RM) libdes.$(SOEXT)
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
