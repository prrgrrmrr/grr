# Directories
SRC := src
BUILD := build

# Library name
LIB := grr

# Compiler
C := clang
STD := -std=c99
C_FLAGS := -Wall -Werror
I_FLAGS := -I$(SRC)

# Resolve platform
OS := $(shell uname -s)
ifeq ($(OS), Darwin)
# macOS
PLATFORM := PLATFORM_MACOS
FRAMEWORKS := -framework vulkan -framework AppKit -framework QuartzCore
FRAMEWORK_PATHS := -F$(VULKAN_SDK)/Frameworks -Wl,-rpath,$(VULKAN_SDK)/Frameworks
else
$(error $(OS) is not supported)
endif

all:
	$(C) $(STD) -DGRR_$(PLATFORM) -DGRR_RELEASE -dynamiclib -fPIC $(C_FLAGS) $(I_FLAGS) -lobjc $(FRAMEWORK_PATHS) $(FRAMEWORKS) -o $(BUILD)/lib/lib$(LIB).dylib $(shell find $(SRC) -name "*.c" -or -name "*.m") -ObjC -install_name @rpath/lib$(LIB).dylib