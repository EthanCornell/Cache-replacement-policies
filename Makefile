# Simple Working Makefile for Cache Replacement Simulator
CXX = g++
CXXFLAGS = -std=c++20 -O3 -Wall -Wextra -Wpedantic

MAIN_SRC = main.cpp
IMPL_SRC = CacheReplacementSimulator.cpp
TEST_SRC = test_cache_replacement.cpp
HEADER = CacheReplacementSimulator.hpp

MAIN_TARGET = cache_simulator
TEST_TARGET = test_cache
DATA_GEN_TARGET = data_generator
MEMORY_TEST_TARGET = memory_test

all: $(MAIN_TARGET) $(TEST_TARGET)

$(MAIN_TARGET): $(MAIN_SRC) $(IMPL_SRC) $(HEADER)
	$(CXX) $(CXXFLAGS) -o $(MAIN_TARGET) $(MAIN_SRC) $(IMPL_SRC)

$(TEST_TARGET): $(TEST_SRC) $(IMPL_SRC) $(HEADER)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_SRC) $(IMPL_SRC)

$(DATA_GEN_TARGET): large_data_generator.cpp
	$(CXX) $(CXXFLAGS) -o $(DATA_GEN_TARGET) large_data_generator.cpp

$(MEMORY_TEST_TARGET): simple_memory_test.cpp
	$(CXX) $(CXXFLAGS) -o $(MEMORY_TEST_TARGET) simple_memory_test.cpp

large-data: $(DATA_GEN_TARGET)
	@echo "Data generator ready!"
	@echo "Usage: ./$(DATA_GEN_TARGET) locality test_20m.txt 20"

memory-test: $(MEMORY_TEST_TARGET)
	./$(MEMORY_TEST_TARGET) performance

massive-test: $(MEMORY_TEST_TARGET)
	./$(MEMORY_TEST_TARGET) massive

quick-demo: $(DATA_GEN_TARGET) $(MAIN_TARGET)
	./$(DATA_GEN_TARGET) locality quick_demo.txt 5
	./$(MAIN_TARGET) quick_demo.txt a 32 0 0
	rm -f quick_demo.txt

clean:
	rm -f $(MAIN_TARGET) $(TEST_TARGET) $(DATA_GEN_TARGET) $(MEMORY_TEST_TARGET) *.txt

.PHONY: all large-data memory-test massive-test quick-demo clean
