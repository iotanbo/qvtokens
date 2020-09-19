
# Check if C compiler specified by the env var,
# set default to gcc
ifeq ($(CC),"")
CC=gcc;
endif

# Check if C++ compiler specified by the env var,
# set default to g++
ifeq ($(CXX),"")
CXX=g++;
endif


# Use local libuv instead of system
UV_PATH=$(shell pwd)/../../libuv
UV_LIB=$(UV_PATH)/.libs/libuv.a
LIBS=

SRC_DIR=$(shell pwd)/src/app
TEST_SRC_DIR=$(shell pwd)/tests

BUILD_DIR=$(shell pwd)/build
EXE_FILE_NAME=$(BUILD_DIR)/qvtokens

uname_S=$(shell uname -s)

ifeq (Linux, $(uname_S))
# Specify additional libs
LIBS=$(UV_LIB) -lrt -ldl -lm -pthread -lstdc++
endif

# Wildcards in makefile:
# https://stackoverflow.com/a/2908351/3824328


OBJ_DIR := $(BUILD_DIR)

# Create list of all .cpp files inside src dir
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)

# Create list of obj files based on the source file list
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Linker flags
LDFLAGS :=

# C flags
CFLAGS=-g -MM -Wall -I$(UV_PATH)/include

# C preprocessor flags
CPPFLAGS :=

# C++ flags
# -MD for generating dependency files that allow detect changes in headers
CXXFLAGS := -std=c++17 -O1 -g -MD -Wall -I$(UV_PATH)/include



# All targets
all: check_build_dir_exists remove_old_exe $(EXE_FILE_NAME)

# Build and run the program
run: $(EXE_FILE_NAME)
	$(EXE_FILE_NAME)

# Build the program and run with valgrind
memcheck: $(EXE_FILE_NAME)
	valgrind --leak-check=yes --track-origins=yes $(EXE_FILE_NAME)

# Main executable file
$(EXE_FILE_NAME): $(OBJ_FILES) $(OBJ_DIR)/main.o
	$(CXX) -o $(EXE_FILE_NAME) $^ $(LDFLAGS) $(LIBS)

# Compile sources into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

# Compile main.cpp separately as it contains main() func
$(OBJ_DIR)/main.o: $(SRC_DIR)/../main.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<


# == TESTS SECTION BEGIN ==

TESTS_EXE_FILE_NAME := $(EXE_FILE_NAME)_tests

# Create list of all .cpp files inside test dir
TEST_SRC_FILES := $(wildcard $(TEST_SRC_DIR)/*.cpp)

# Create list of obj files based on the TEST_SRC_FILES
TEST_OBJ_FILES := $(patsubst $(TEST_SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(TEST_SRC_FILES))

tests: check_build_dir_exists remove_old_exe $(TESTS_EXE_FILE_NAME)

runtests: $(TESTS_EXE_FILE_NAME)
	$(TESTS_EXE_FILE_NAME)

# Create test executable
$(TESTS_EXE_FILE_NAME): $(TEST_OBJ_FILES) $(OBJ_FILES)
	$(CXX) -o $(TESTS_EXE_FILE_NAME) $^ $(LDFLAGS) $(LIBS)

# Compile test sources
$(OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

# === TESTS SECTION END ===


.PHONY: check_build_dir_exists
check_build_dir_exists:
	# *** Create build dir if not exists ***
	if [ -d "$(BUILD_DIR)" ]; then \
		echo "=> Build directory exists: $(BUILD_DIR)"; \
	else \
		echo "=> Creating build dir: $(BUILD_DIR)"; \
		mkdir -p "$(BUILD_DIR)"; \
	fi


.PHONY: remove_old_exe
remove_old_exe:
	# *** Remove old executable ***
	if [ -f "$(EXE_FILE_NAME)" ]; then \
		echo "=> Removing old executable $(EXE_FILE_NAME)"; \
		rm $(EXE_FILE_NAME); \
	fi

	# *** Remove old executable tests ***
	if [ -f "$(TESTS_EXE_FILE_NAME)" ]; then \
		echo "=> Removing old executable $(TESTS_EXE_FILE_NAME)"; \
		rm $(TESTS_EXE_FILE_NAME); \
	fi

.PHONY: clean
clean:
	if [ -d $(BUILD_DIR) ]; then \
	echo "=> Cleaning build dir"; \
	rm -rf $(BUILD_DIR); \
	fi

# Required to track header file changes
-include $(OBJ_FILES:.o=.d)
-include $(TEST_OBJ_FILES:.o=.d)
