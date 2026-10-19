# ACNH 最小宿主（headless）：装载 main/sdk/subsdk0 + 系统调用/线程/时间/服务层 + NVN→Metal 图形层。
# ACNH minimal host (headless): loader for main/sdk/subsdk0 + SVC/thread/time/service layers + the NVN->Metal graphics layer.
# 目标平台：macOS 26（Metal 4）。
# Target platform: macOS 26 (Metal 4).
MACOS_MIN ?= 26.0
CC ?= clang
CFLAGS = -O1 -g -Wall -Wno-unused-function -mmacosx-version-min=$(MACOS_MIN) -I. -Igfx/nvn -Igfx/nvn/include -Igfx/tex
LDFLAGS = -lpthread \
	-framework AudioToolbox -framework CoreAudio -framework CoreFoundation \
	-framework Metal -framework QuartzCore -framework Foundation -framework CoreGraphics \
	-framework ImageIO -framework UniformTypeIdentifiers

HOSTSRC = \
	main_acnh.c \
	loader.c \
	entry.S \
	hle.c \
	thread.c \
	hle_svc.c \
	hle_thread.c \
	hle_os.c \
	hle_table.c \
	hle_sdkinit.c \
	hle_audio.c \
	hle_account.c \
	hle_fs.c \
	hle_net_acnh.c \
	hle_oe_acnh.c \
	hle_time.c \
	hle_guestthread.c \
	crash.c \
	front_null.c \
	acnh_stub.c

# NVN 空设备 + nn::vi；nv:: 胶水层（gfx/）
# NVN null device + nn::vi; the nv:: glue layer (gfx/)
GFX = $(sort $(wildcard gfx/*.c gfx/nvn/*.c gfx/nvn/*.m gfx/nvn/*.S gfx/tex/*.c gfx/tex/*.m))

SRC = $(HOSTSRC) $(GFX)

.PHONY: all clean print-macos-min
all: dmhost_acnh

dmhost_acnh: $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

print-macos-min:
	@echo $(MACOS_MIN)

clean:
	rm -f dmhost_acnh
