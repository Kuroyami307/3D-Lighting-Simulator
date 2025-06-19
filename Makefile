# =========================
# Makefile for Emscripten
# =========================

# Output folder and target name
TARGET_DIR = Emiscripten
TARGET = $(TARGET_DIR)/index

# Source files
SRC_CPP = main.cpp
SRC_C = ../Files/include/glad/glad.c

# Include paths
INCLUDE = \
-I../Files/include \
-I/opt/homebrew/include

# Compiler
CC = emcc

# Flags
CFLAGS_CPP = $(INCLUDE) -std=c++17
CFLAGS_C = $(INCLUDE)

LINK_FLAGS = \
-s USE_WEBGL2=1 \
-s USE_GLFW=3 \
--preload-file $(TARGET_DIR)/vShader.vs \
--preload-file $(TARGET_DIR)/fShader.fs \
--preload-file $(TARGET_DIR)/horse.obj

# Object files
OBJ_CPP = $(SRC_CPP:.cpp=.o)
OBJ_C = $(notdir $(SRC_C:.c=.o))

# ===== Targets =====

# Default target
all: prepare $(TARGET).html

# Prepare output directory and shaders
prepare:
	mkdir -p $(TARGET_DIR)
	cp Shaders/vShader.vs Shaders/fShader.fs horse.obj $(TARGET_DIR)

# Compile C++ sources
%.o: %.cpp
	$(CC) $(CFLAGS_CPP) -c $< -o $@

# Compile GLAD
glad.o: ../Files/include/glad/glad.c
	$(CC) $(CFLAGS_C) -c $< -o $@

# Link to output
$(TARGET).html: $(OBJ_CPP) $(OBJ_C)
	$(CC) $^ -o $@ $(LINK_FLAGS)

# Clean build
clean:
	rm -f *.o
	rm -rf $(TARGET_DIR)
