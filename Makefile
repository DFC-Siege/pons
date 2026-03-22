-include .env
export WIFI_SSID
export WIFI_PASS
export PICO_SDK_PATH

.PHONY: all build clean clean-all test rebuild \
        build-posix build-esp32 build-rp2040 \
        test-posix test-esp32 \
        compile-commands-posix compile-commands-esp32

# ── POSIX ────────────────────────────────────────────────────────────────────

build-posix:
	cmake -B build-posix -DPLATFORM=posix -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build-posix
	ln -sf build-posix/compile_commands.json compile_commands.json

test-posix:
	cmake -B build-posix -DPLATFORM=posix -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build-posix
	cd build-posix && ctest --output-on-failure

compile-commands-posix:
	cmake -B build-posix -DPLATFORM=posix -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	ln -sf build-posix/compile_commands.json compile_commands.json

# ── ESP32 ────────────────────────────────────────────────────────────────────

hal/esp32/tests/sdkconfig.local:
	@echo 'CONFIG_EXAMPLE_WIFI_SSID=$(WIFI_SSID)' > hal/esp32/tests/sdkconfig.local
	@echo 'CONFIG_EXAMPLE_WIFI_PASSWORD=$(WIFI_PASS)' >> hal/esp32/tests/sdkconfig.local

build-esp32: hal/esp32/tests/sdkconfig.local
	cd hal/esp32/tests && idf.py set-target esp32c3 build
	ln -sf hal/esp32/tests/build/compile_commands.json compile_commands_esp32.json

test-esp32: hal/esp32/tests/sdkconfig.local
	cd hal/esp32/tests && idf.py set-target esp32c3 build flash monitor
	ln -sf hal/esp32/tests/build/compile_commands.json compile_commands_esp32.json

compile-commands-esp32:
	ln -sf hal/esp32/tests/build/compile_commands.json compile_commands_esp32.json

# ── RP2040 ───────────────────────────────────────────────────────────────────

build-rp2040:
	cmake -B build-rp2040 -DPLATFORM=rp2040 \
		-DCMAKE_TOOLCHAIN_FILE=$(PICO_SDK_PATH)/cmake/preload/toolchains/pico_arm_cortex_m0plus_gcc.cmake \
		-G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build-rp2040
	ln -sf build-rp2040/compile_commands.json compile_commands_rp2040.json

compile-commands-rp2040:
	cmake -B build-rp2040 -DPLATFORM=rp2040 -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	ln -sf build-rp2040/compile_commands.json compile_commands_rp2040.json

build-rp2040-tests:
	cmake -B build-rp2040-tests \
		-S hal/rp2040/tests \
		-DCMAKE_TOOLCHAIN_FILE=$(PICO_SDK_PATH)/cmake/preload/toolchains/pico_arm_cortex_m0plus_gcc.cmake \
		-G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build-rp2040-tests

flash-rp2040-tests:
	cp build-rp2040-tests/main/rp2040_hal_tests.uf2 /media/$(USER)/RPI-RP2/

monitor-rp2040:
	minicom -b 115200 -D /dev/ttyACM0

# ── GENERIC ──────────────────────────────────────────────────────────────────

all: build-posix

clean:
	rm -rf build-posix build-rp2040

clean-all:
	rm -rf build-posix build-rp2040 hal/esp32/tests/build hal/esp32/tests/sdkconfig

install:
	cmake --install build-posix

compile-commands: compile-commands-posix compile-commands-esp32 compile-commands-rp2040
