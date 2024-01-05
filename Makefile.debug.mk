# Directories
SRC := src

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
	$(C) $(STD) -g -DGRR_$(PLATFORM) -DGRR_DEBUG $(C_FLAGS) $(I_FLAGS) -lobjc $(FRAMEWORK_PATHS) $(FRAMEWORKS) -o $(SRC)/debug/maind $(shell find $(SRC) -name "*.c" -or -name "*.m") -ObjC