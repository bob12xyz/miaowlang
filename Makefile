CXX = g++
CXXFLAGS = -std=c++20 -g -Wall -Wextra $(shell llvm-config --cxxflags)
LDFLAGS = $(shell llvm-config --ldflags --system-libs --libs core)

# Source files
SRCS = miaow.cpp types.cpp intrinsics.cpp parser.cpp compiler.cpp debug.cpp preprocessor.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = miaow

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files

debug.o: debug.cpp debug.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

miaow.o: miaow.cpp types.hpp intrinsics.hpp parser.hpp compiler.hpp debug.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

types.o: types.cpp types.hpp debug.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

intrinsics.o: intrinsics.cpp intrinsics.hpp types.hpp debug.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

parser.o: parser.cpp parser.hpp types.hpp debug.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

compiler.o: compiler.cpp compiler.hpp types.hpp intrinsics.hpp debug.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

preprocessor.o: preprocessor.cpp preprocessor.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
