LOCAL_PATH := $(call my-dir)

# ------------------------------------------------------------------
# Mcl0v1n vs android - native su + daemon
# Builds a single static binary used as su / mclovind / daemonsu
# (behaviour selected by argv[0], see su.c).
# Target: Android 4.4.4 (API 19), armeabi-v7a, GT-I9300.
# ------------------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE    := su
LOCAL_SRC_FILES := su.c client.c daemon.c policy.c

LOCAL_CFLAGS    := -Wall -Wextra -std=gnu99 \
                   -DMCLOVIN -DNDEBUG \
                   -ffunction-sections -fdata-sections

# Static link so the binary has no runtime loader dependency on /system libs
# that may differ between stock/CM ROMs. Keeps it robust on KitKat.
LOCAL_LDFLAGS   := -static -Wl,--gc-sections

include $(BUILD_EXECUTABLE)
