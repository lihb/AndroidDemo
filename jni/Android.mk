LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := avcodec-55-prebuilt
LOCAL_SRC_FILES := prebuilt/libavcodec-55.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := avformat-55-prebuilt
LOCAL_SRC_FILES := prebuilt/libavformat-55.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE :=  avutil-52-prebuilt
LOCAL_SRC_FILES := prebuilt/libavutil-52.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE :=  avswresample-0-prebuilt
LOCAL_SRC_FILES := prebuilt/libswresample-0.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE :=  swscale-2-prebuilt
LOCAL_SRC_FILES := prebuilt/libswscale-2.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)

LOCAL_MODULE := ffmpeg_codec
LOCAL_SRC_FILES := cn_dennishucd_FFmpegNative.c cn_dennishucd_FFmpegAudioNative.c

LOCAL_LDLIBS := -llog -ljnigraphics -lz -landroid -lGLESv2
LOCAL_LDLIBS    += -lOpenSLES

LOCAL_SHARED_LIBRARIES := avformat-55-prebuilt avcodec-55-prebuilt  swscale-2-prebuilt avutil-52-prebuilt avswresample-0-prebuilt

include $(BUILD_SHARED_LIBRARY)