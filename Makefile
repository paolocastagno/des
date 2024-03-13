SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS = $(patsubst %.cpp,%.d,$(SOURCES))
OS =$(shell uname)

ifeq ($(OS),Darwin)
    CXXFLAGS = -std=c++2a -ggdb
	SOEXT = dylib
	CSTD = -std=c++2a
	DYFLAGS =
else
	CXXFLAGS = -std=c++20 -g -ggdb --fPIC
	SOEXT = so
	CSTD = -std=c++20
	DYFLAGS = -fPIC
endif

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS=$(CSTD) -DDEBUG -g -ggdb $(DYFLAGS)
else
    CXXFLAGS=$(CSTD) -O3 -ggdb $(DYFLAGS)
endif

# ADD MORE WARNINGS!
WARNING := -Wall -Wextra
# .PHONY means these rules get executed even if
# files of those names exist.
.PHONY: all clean install
# The first rule is the default, ie. "make",
# "make all" and "make libdes" mean the same
all: libdes clean-dep
# Remove object files and dependences
clean:
			$(RM) $(OBJECTS) $(DEPENDS)
clean-dep:
			$(RM)  $(DEPENDS)
clean-lib:
			$(RM) libdes.$(SOEXT)
install:	
			./install.sh

# Linking the executable from the object files
libdes:  $(OBJECTS)
			$(CXX) $(WARNING) -shared $^ -o $@.$(SOEXT)
-include $(DEPENDS)

%.o: %.cpp Makefile
		$(CXX) $(WARNING) $(CXXFLAGS) -MMD -MP -c $< -o $@

# test: $(CXX) $(WARNING) $(CXXFLAGS) 
# For any doubts check 
# https://stackoverflow.com/questions/52034997/how-to-make-makefile-recompile-when-a-header-file-is-changed

# https://stackoverflow.com/questions/44097902/shared-libraries-c-makefile

# file placement
# https://stackoverflow.com/questions/212492/how-do-you-add-external-libraries-for-compilation-in-vc
