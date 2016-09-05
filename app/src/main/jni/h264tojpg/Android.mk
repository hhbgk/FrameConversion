LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := jpg
LOCAL_SRC_FILES += h264_to_jpg_jni.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(MY_APP_JNI_ROOT)/ffmpeg/armv7a/include

LOCAL_SHARED_LIBRARIES := ijkffmpeg

LOCAL_LDLIBS := -llog -landroid
include $(BUILD_SHARED_LIBRARY)