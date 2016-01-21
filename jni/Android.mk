LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := avcodec-55-prebuilt
LOCAL_SRC_FILES := prebuilt/libavcodec-57.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avfilter-4-prebuilt
LOCAL_SRC_FILES := prebuilt/libavfilter-6.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avformat-55-prebuilt
LOCAL_SRC_FILES := prebuilt/libavformat-57.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE :=  avutil-52-prebuilt
LOCAL_SRC_FILES := prebuilt/libavutil-55.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE :=  avswresample-0-prebuilt
LOCAL_SRC_FILES := prebuilt/libswresample-2.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE :=  swscale-2-prebuilt
LOCAL_SRC_FILES := prebuilt/libswscale-4.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := ffmpeg_codec
LOCAL_SRC_FILES := cn_dennishucd_FFmpegNative.c cn_dennishucd_FFmpegAudioNative.c cn_dennishucd_RtmpPush.c

LOCAL_LDLIBS := -llog -ljnigraphics -lz -landroid -lGLESv2
LOCAL_LDLIBS    += -lOpenSLES

LOCAL_SHARED_LIBRARIES := avcodec-55-prebuilt  avfilter-4-prebuilt avformat-55-prebuilt \
avutil-52-prebuilt swscale-2-prebuilt avswresample-0-prebuilt

include $(BUILD_SHARED_LIBRARY)