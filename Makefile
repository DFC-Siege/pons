-include .env
export WIFI_SSID
export WIFI_PASSPLATFORM ?= posix

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

hal/esp32/tests/sdkconfig.local:
	@echo 'CONFIG_EXAMPLE_WIFI_SSID=$(WIFI_SSID)' > hal/esp32/tests/sdkconfig.local
	@echo 'CONFIG_EXAMPLE_WIFI_PASSWORD=$(WIFI_PASS)' >> hal/esp32/tests/sdkconfig.local

test-esp32: hal/esp32/tests/sdkconfig.local
	cd hal/esp32/tests && idf.py set-target esp32c3 build flash monitor

install:
	cmake --install $(BUILD_DIR)

clean:
	cmake --build $(BUILD_DIR) --target clean

clean-all:
	rm -rf build-*

rebuild: clean build

compile-commands:
	ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json
