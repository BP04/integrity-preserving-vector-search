CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Isrc -Itest
LDFLAGS = -lcrypto
TARGET = verifiable_vector_db

SRCS = $(wildcard src/*.cpp) $(wildcard test/*.cpp) main.cpp
OBJS = $(patsubst %.cpp,build/%.o,$(notdir $(SRCS)))

vpath %.cpp src test .

.PHONY: run clean

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

build/%.o: %.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)
