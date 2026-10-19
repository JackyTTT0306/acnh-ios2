# ACNH 最小宿主（headless）：装载 main/sdk/subsdk0 + 系统调用/线程/时间/服务层骨架。
# ACNH minimal host (headless): loader for main/sdk/subsdk0 + SVC/thread/time/service scaffolding.
# 目标平台：macOS 26（Metal 4 后端在 gfx/，本最小集还没接入图形）。
# Target platform: macOS 26 (the Metal 4 backend lives in gfx/ and is not wired into this minimal set yet).
MACOS_MIN ?= 26.0
CC ?= clang
CFLAGS = -O1 -g -Wall -Wno-unused-function -mmacosx-version-min=$(MACOS_MIN) -I.
LDFLAGS = -lpthread

SRC = \
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
	hle_time.c \
	hle_guestthread.c \
	crash.c \
	front_null.c \
	acnh_stub.c

.PHONY: all clean print-macos-min
all: dmhost_acnh

dmhost_acnh: $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

print-macos-min:
	@echo $(MACOS_MIN)

clean:
	rm -f dmhost_acnh
