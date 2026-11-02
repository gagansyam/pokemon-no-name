#
# Makefile for Desktop C++ Pokemon Real-Time Battle Engine
#

CXX      := clang++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra \
            -Iinclude -Isrc -Isrc/core -Isrc/entities -Isrc/combat -Isrc/ai -Isrc/camera -Isrc/ui \
            $(shell sdl2-config --cflags)
LDFLAGS  := $(shell sdl2-config --libs) -lm

SRC_DIR  := src
BUILD    := build
TARGET   := pokemon_battle

SRCS     := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS     := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD)/%.o,$(SRCS))

all: $(TARGET)

$(BUILD)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@
	@echo "Build complete: ./$(TARGET)"

clean:
	rm -rf $(BUILD) $(TARGET)

sprites:
	python3 tools/process_sprite_sheet.py --batch assets/sprites -o src/core/sprite_manifest.hpp

run: $(TARGET)
	./$(TARGET)

TEST_OBJS := $(filter-out $(BUILD)/main.o, $(OBJS))
test: $(TARGET)
	@mkdir -p tests
	$(CXX) $(CXXFLAGS) tests/test_battlegrounds_sim.cpp $(TEST_OBJS) $(LDFLAGS) -o tests/test_sim
	./tests/test_sim

.PHONY: all clean run sprites test
