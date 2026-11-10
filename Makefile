CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pthread -Iinclude
LDFLAGS = -pthread

SRC_DIR = src
INC_DIR = include
OBJ_DIR = build
BIN_DIR = bin

TARGET = $(BIN_DIR)/kernel_simulator

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Detect Windows or POSIX environment
ifeq ($(OS),Windows_NT)
    MKDIR_P = if not exist $(subst /,\\,$(1)) mkdir $(subst /,\\,$(1))
    RM_RF = if exist $(subst /,\\,$(1)) rmdir /s /q $(subst /,\\,$(1))
    TARGET_BIN = $(TARGET).exe
else
    MKDIR_P = mkdir -p $(1)
    RM_RF = rm -rf $(1)
    TARGET_BIN = $(TARGET)
endif

.PHONY: all clean

all: $(TARGET_BIN)

$(TARGET_BIN): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	@$(call MKDIR_P, $(OBJ_DIR))

$(BIN_DIR):
	@$(call MKDIR_P, $(BIN_DIR))

clean:
	@$(call RM_RF, $(OBJ_DIR))
	@$(call RM_RF, $(BIN_DIR))
