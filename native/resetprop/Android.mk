LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE    := mclovin_sepatch
LOCAL_SRC_FILES := mclovin_sepatch.c
LOCAL_CFLAGS    := -Wall -std=gnu99
LOCAL_LDFLAGS   := -static
include $(BUILD_EXECUTABLE)
