# Solitaire implementation Makefile
# Builds both core library and solitaire implementation
NAMEBIN = solitaire

# Project structure
# ----------------
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = inc
CORE_DIR = ../../core

# Source files
# -----------
# Solitaire sources
SRC_FILES = $(wildcard $(SRC_DIR)/*.c) 
OBJ_FILES = $(addprefix $(OBJ_DIR)/, $(notdir $(SRC_FILES:%.c=%.o)))

# Core library sources
CORE_SRC_DIR = $(CORE_DIR)/src
CORE_INC_DIR = $(CORE_DIR)/inc
CORE_SRC_FILES = $(wildcard $(CORE_SRC_DIR)/*.c)
CORE_OBJ_FILES = $(addprefix $(OBJ_DIR)/, $(notdir $(CORE_SRC_FILES:%.c=%.o)))
CORE_INC_FILES = $(wildcard $(CORE_INC_DIR)/*.h)

# Compiler settings
# ----------------
CC = clang
# Extensive warning flags for maximum code quality
CFLAGS = -std=c11 -Oz $(addprefix -f, merge-all-constants function-sections data-sections lto short-enums no-common stack-protector-strong) \
				      $(addprefix -W, all extra error pedantic conversion null-dereference\
						cast-align unreachable-code strict-prototypes bad-function-cast \
						strict-aliasing shadow double-promotion missing-prototypes \
						missing-declarations pointer-arith format=2)

AR = ar
AFLAGS = rcs
MKDIR = mkdir -p
RM = rm -rf

all: $(NAMEBIN) clean

$(NAMEBIN): $(OBJ_FILES) $(CORE_OBJ_FILES)
	@$(CC) $(CFLAGS) $^ -o $@
	@strip $@

$(OBJ_FILES): | $(OBJ_DIR)

# First compile solitaire files with context
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_FILES)
	@$(CC) $(CFLAGS) -c $< -o $@ -I $(INC_DIR) -I $(CORE_INC_DIR) -include $(INC_DIR)/solitare.h

# Then compile core files with context
$(OBJ_DIR)/%.o: $(CORE_SRC_DIR)/%.c $(CORE_INC_FILES)
	@$(CC) $(CFLAGS) -c $< -o $@ -I $(CORE_INC_DIR) -include $(INC_DIR)/solitare.h

$(OBJ_DIR):
	@$(MKDIR) $@

clean:
	@$(RM) $(OBJ_DIR)

uninstall:
	@$(RM) $(OBJ_DIR)
	@$(RM) $(NAMEBIN)

reinstall: uninstall all

.PHONY: all uninstall clean reinstall
