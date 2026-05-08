CXX      := c++
CXXFLAGS := -std=c++17 -Wall -I$(shell sdl2-config --prefix)/include $(shell sdl2-config --cflags)
LDFLAGS  := $(shell sdl2-config --libs)

TARGET   := build/c8v
SRCDIR   := c8v
SRCS     := $(SRCDIR)/main.cpp

$(TARGET): $(SRCS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -rf build

.PHONY: clean
