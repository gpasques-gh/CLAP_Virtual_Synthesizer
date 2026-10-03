# Compiler
CC = gcc

# Binary
TARGET = synth

# Directories
SRC_DIR = src
INC_DIR = include
ASSETS_DIR = assets
BIN_DIR = bin
OBJ_DIR = obj
EXTERNAL_DIR = external

# Submodules: stamp file so the init/sparse-checkout below only runs once
SUBMODULE_STAMP = $(EXTERNAL_DIR)/.submodules-initialized
RAYLIB_LIB = $(EXTERNAL_DIR)/lib_raylib/src/libraylib.a

# Files
CORE_SRCS = $(wildcard $(SRC_DIR)/core/*.c)
STANDALONE_SRCS = $(wildcard $(SRC_DIR)/standalone/*.c)
STANDALONE_NO_GUI_SRCS = $(SRC_DIR)/standalone/midi.c $(SRC_DIR)/standalone/audio_thread.c $(SRC_DIR)/standalone/main.c
CLAP_SRCS = $(wildcard $(SRC_DIR)/clap/*.c) $(wildcard $(SRC_DIR)/clap/gui/*.c)
SRCS =

# OS Detection
ifeq ($(OS),Windows_NT)
	DETECTED_OS = Windows
else
	DETECTED_OS = $(shell uname -s)
endif

# Common flags
CFLAGS = -Wall -Wextra -O2 -I$(INC_DIR) -MMD -MP
LDFLAGS =

# OS specific flags
ifeq ($(DETECTED_OS),Windows)
	RM = powershell -NoProfile -Command Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
	MKDIR = powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(1)' | Out-Null"
	TOUCH = powershell -NoProfile -Command "New-Item -ItemType File -Force -Path '$(1)' | Out-Null"
else
	RM = rm -rf
	MKDIR = mkdir -p $(1)
	TOUCH = touch $(1)
endif

COMPILE_MODE=STANDALONE
ifeq ($(COMPILE_MODE),CLAP)
	TARGET := $(TARGET).clap
	CFLAGS += -D__CLAP__ -fPIC -I$(EXTERNAL_DIR)/
	LDFLAGS += -shared
	SRCS = $(CORE_SRCS) $(CLAP_SRCS)

	# Libs for plugin GUI
	ifeq ($(DETECTED_OS),Windows)
		LDFLAGS += -lgdi32 -luser32
	else ifeq ($(DETECTED_OS),Linux)
		LDFLAGS += -lX11
	else ifeq ($(DETECTED_OS),Darwin)
		LDFLAGS += -framework Cocoa
	endif
else ifeq ($(DETECTED_OS),Windows)
	TARGET := $(TARGET).exe
	ifeq ($(COMPILE_MODE),NO_GUI)
		CFLAGS += -D_WIN32_WINNNT=0x0601 -D__WINDOWS__ -D__NO_GUI__
		LDFLAGS += -lm -lksuser -lwinmm -lole32 -luuid -lshell32 -lws2_32
		SRCS = $(CORE_SRCS) $(STANDALONE_NO_GUI_SRCS)
	else
		CFLAGS += -D_WIN32_WINNT=0x0601 -D__WINDOWS__ -I$(EXTERNAL_DIR)/ -I$(EXTERNAL_DIR)/libxml/include/ -I$(EXTERNAL_DIR)/lib_raylib/src/ -I$(EXTERNAL_DIR)/lib_raygui/src/
		LDFLAGS += -L$(EXTERNAL_DIR)/lib_raylib/src/ -lraylib -L$(EXTERNAL_DIR)/libxml/lib/ -lxml2 -lm -lksuser -lwinmm -lgdi32 -lopengl32 -lole32 -luuid -lshell32 -lws2_32
		SRCS = $(CORE_SRCS) $(STANDALONE_SRCS)
	endif 
else ifeq ($(DETECTED_OS),Linux)
	ifeq ($(COMPILE_MODE),NO_GUI)
		CFLAGS += -D__LINUX__ -D__NO_GUI__
		LDFLAGS += -lasound -lm -lpthread
		SRCS = $(CORE_SRCS) $(STANDALONE_NO_GUI_SRCS)
	else
		CFLAGS += -I/usr/include/libxml2 -D__LINUX__ -I$(EXTERNAL_DIR)/ -I$(EXTERNAL_DIR)/lib_raylib/src/ -I$(EXTERNAL_DIR)/lib_raygui/src/
		LDFLAGS += -lasound -lm -lraylib -lxml2 -lX11 -lpthread
		SRCS = $(CORE_SRCS) $(STANDALONE_SRCS)
	endif 
else ifeq ($(DETECTED_OS),Darwin)
	CFLAGS += -I/usr/local/include/libxml2 -I/opt/homebrew/include/libxml2 -D__MACOS__
	LDFLAGS += -lm -lraylib -lxml2
	SRCS = $(CORE_SRCS) $(STANDALONE_SRCS)
endif


OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS = $(OBJS:.o=.d)

# Default
all: $(SUBMODULE_STAMP) $(BIN_DIR)/$(TARGET)

# Initialize submodules 
$(SUBMODULE_STAMP):
	git submodule update --init --filter=blob:none -- $(EXTERNAL_DIR)/lib_raylib $(EXTERNAL_DIR)/lib_raygui $(EXTERNAL_DIR)/lib_clap $(EXTERNAL_DIR)/lib_stb
	git -C $(EXTERNAL_DIR)/lib_raylib sparse-checkout init --cone
	git -C $(EXTERNAL_DIR)/lib_raylib sparse-checkout set src
	git -C $(EXTERNAL_DIR)/lib_raygui sparse-checkout init --cone
	git -C $(EXTERNAL_DIR)/lib_raygui sparse-checkout set src
	git -C $(EXTERNAL_DIR)/lib_clap sparse-checkout init --cone
	git -C $(EXTERNAL_DIR)/lib_clap sparse-checkout set include
	git -C $(EXTERNAL_DIR)/lib_stb sparse-checkout init --no-cone
	git -C $(EXTERNAL_DIR)/lib_stb sparse-checkout set /stb_truetype.h
	$(call TOUCH,$(SUBMODULE_STAMP))

# Forcing a re-initialization of submodules
submodules-refresh:
	$(RM) $(SUBMODULE_STAMP)
	$(MAKE) $(SUBMODULE_STAMP)

# Link
ifeq ($(COMPILE_MODE),STANDALONE)
$(BIN_DIR)/$(TARGET) : $(RAYLIB_LIB)

$(RAYLIB_LIB): $(SUBMODULE_STAMP)
	$(MAKE) -C $(EXTERNAL_DIR)/lib_raylib/src PLATOFRM=PLATFORM_DESKTOP
endif
$(BIN_DIR)/$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(SUBMODULE_STAMP)
	-@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories if needed
$(BIN_DIR):
	$(call MKDIR,$(BIN_DIR))

$(OBJ_DIR):
	$(call MKDIR,$(OBJ_DIR))

# Clean
clean:
	$(RM) $(BIN_DIR)
	$(RM) $(OBJ_DIR)

# Rebuild
re: clean all

run:
	./$(BIN_DIR)/$(TARGET)

-include $(DEPS)
.PHONY: all clean re run submodules-refresh