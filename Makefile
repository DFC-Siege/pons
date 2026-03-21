PLATFORM ?= posix
BUILD_DIR := build-$(PLATFORM)

.PHONY: all build clean clean-all test rebuild

all: build

build:
	cmake -B $(BUILD_DIR) -DPLATFORM=$(PLATFORM) -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(BUILD_DIR)

test:
	cmake -B $(BUILD_DIR) -DPLATFORM=$(PLATFORM) -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(BUILD_DIR)
	cd $(BUILD_DIR) && ctest --output-on-failure

install:
	cmake --install $(BUILD_DIR)

clean:
	cmake --build $(BUILD_DIR) --target clean

clean-all:
	rm -rf build-*

rebuild: clean build

compile-commands:
	ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json
