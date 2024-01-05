# Directories
DEMO := demo
BUILD := build

# Library name
LIB := grr

# Compiler
C := clang
STD := -std=c99
C_FLAGS := -Wall -Werror
I_FLAGS := -I$(BUILD)/include
L_FLAGS := -l$(LIB) -L$(BUILD)/lib -Wl,-rpath,$(BUILD)/lib

all:
	$(C) $(STD) -g $(C_FLAGS) $(I_FLAGS) $(L_FLAGS) -o $(DEMO)/main $(shell find $(DEMO) -name "*.c")
