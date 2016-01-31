#include <math.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>

#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>

#include <jni.h>

#include <stdio.h>
#include <wchar.h>
#include <pthread.h>
#include <unistd.h>

#include "cn_dennishucd_ThreadInJava.h"

/*for android logs*/
#include <android/log.h>

#define LOG_TAG "android-ffmpeg-lihb-test"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__);
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     cn_dennishucd_ThreadInJava
 * Method:    nativePlay
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cn_dennishucd_ThreadInJava_nativePlay(JNIEnv *pEnv, jobject obj, jstring fileName){

    char              *videoFileName;
    AVFormatContext   *formatCtx = NULL;
    int               videoStream;
    AVCodecContext    *codecCtx = NULL;
    AVFrame           *decodedFrame = NULL;
    AVFrame           *frameRGBA = NULL;
    jobject           bitmap;
    void*             buffer;
    struct SwsContext   *sws_ctx = NULL;
    AVCodec         *pCodec = NULL;
    int             i;
    AVDictionary    *optionsDict = NULL;
    AVPacket        packet;
    int             frameFinished;

    videoFileName = (char *)(*pEnv)->GetStringUTFChars(pEnv, fileName, NULL);
    LOGI("video file name is %s", videoFileName);
    // Register all formats and codecs
    av_register_all();
    // Open video file
    if(avformat_open_input(&formatCtx, videoFileName, NULL, NULL)!=0){
      LOGI("Couldn't open file");
      return -1; // Couldn't open file
    }else{
        LOGI("open file success");
    }

    // Retrieve stream information
    if(avformat_find_stream_info(formatCtx, NULL)<0){
       LOGI("Couldn't find stream information");
       return -1; // Couldn't find stream information
    }else{
        LOGI("find stream information success");
    }
    // Dump information about file onto standard error
    av_dump_format(formatCtx, 0, videoFileName, 0);
    // Find the first video stream
    videoStream=-1;
    for(i=0; i<formatCtx->nb_streams; i++) {
      if(formatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
        videoStream=i;
        break;
      }
    }
    if(videoStream==-1)
      return -1; // Didn't find a video stream
    // Get a pointer to the codec context for the video stream
    codecCtx=formatCtx->streams[videoStream]->codec;
    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(codecCtx->codec_id);
    if(pCodec==NULL) {
      fprintf(stderr, "Unsupported codec!\n");
      return -1; // Codec not found
    }
     LOGI("AvCodec name is  %s", pCodec->name);
     LOGI("AvCodec long name is  %s", pCodec->long_name);
    // Open codec
    if(avcodec_open2(codecCtx, pCodec, &optionsDict)<0){
        LOGI("----Could not open codec----");
        return -1; // Could not open codec
    }else{
        LOGI("---- open codec successful----");
    }
    LOGI("naInit() current thread id = %lu", pthread_self());

    // Allocate video frame
    decodedFrame=av_frame_alloc();
    // Allocate an AVFrame structure
    frameRGBA=av_frame_alloc();
    if(frameRGBA==NULL)
      return -1;

    //create a bitmap as the buffer for frameRGBA
    bitmap = createBitmapFromJni(pEnv, codecCtx->width, codecCtx->height);
    if (AndroidBitmap_lockPixels(pEnv, bitmap, &buffer) < 0)
      return -1;
    //get the scaling context
    sws_ctx = sws_getContext(
            codecCtx->width,
            codecCtx->height,
            codecCtx->pix_fmt,
            codecCtx->width,
            codecCtx->height,
            AV_PIX_FMT_RGBA,
            SWS_BILINEAR,
            NULL,
            NULL,
            NULL
    );
     LOGI("naSetup() current thread id = %lu", pthread_self());
    // Assign appropriate parts of bitmap to image planes in pFrameRGBA
    // Note that pFrameRGBA is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)frameRGBA, buffer, AV_PIX_FMT_RGBA,
         codecCtx->width,codecCtx->height);

     // Read frames and save first five frames to disk
      while(av_read_frame(formatCtx, &packet)>=0) {
    //  	 	 usleep(200000); // 300000microsecond = 300millisecond
         // Is this a packet from the video stream?
         if(packet.stream_index==videoStream) {
             // Decode video frame
             avcodec_decode_video2(codecCtx, decodedFrame, &frameFinished, &packet);
             // Did we get a video frame?
             if(frameFinished) {
             LOGI("in if frameFinished, decodedFrame->width = %d", decodedFrame->width);
             LOGI("decodeVideo() current thread id = %lu", pthread_self());
                // Convert the image from its native format to RGBA
              sws_scale
              (
                sws_ctx,
                (uint8_t const * const *)decodedFrame->data,
                decodedFrame->linesize,
                0,
                codecCtx->height,
                frameRGBA->data,
                frameRGBA->linesize
              );

                //获取Java层对应的类
                jclass javaClass = (*pEnv)->GetObjectClass(pEnv, obj);
                if( javaClass == NULL ) {
                    LOGI("Fail to find javaClass");
                    return 0;
                }

              //获取Java层被回调的函数
              jmethodID javaMethod = (*pEnv)->GetStaticMethodID(pEnv,javaClass,"offerInJava","(Landroid/graphics/Bitmap;)Z");
//            jmethodID javaMethod = (*threadEnv)->GetMethodID(threadEnv,javaClass,"saveFrameToPath","(Landroid/graphics/Bitmap;Ljava/lang/String;)V");
              if(javaMethod == NULL) {
                LOGI("Fail to find method offerInJava");
                return 0;
              }

              //回调Java层的函数
               LOGI("call java method to save frame");
//               char szFilename[200];

//             sprintf(szFilename, "/sdcard/aaaaa/frame%d.jpg", i);
//             jstring filePath = (*threadEnv)->NewStringUTF(threadEnv, szFilename);

//            (*threadEnv)->CallVoidMethod(threadEnv,gJavaObj,javaCallback, bitmap, filePath);
              (*pEnv)->CallStaticBooleanMethod(pEnv, javaClass, javaMethod, bitmap);
               LOGI("call java method to save frame done.");
//               LOGI("save frame %d", i);
               // 释放资源
//               (*pEnv)->DeleteLocalRef(pEnv, javaMethod);
               (*pEnv)->DeleteLocalRef(pEnv, javaClass);
//               (*threadEnv)->DeleteLocalRef(threadEnv, filePath);

          }
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);

      }

      //unlock the bitmap
      AndroidBitmap_unlockPixels(pEnv, bitmap);

      // Free the RGB image
      av_free(frameRGBA);

      // Free the YUV frame
      av_free(decodedFrame);

      // Close the codec
      avcodec_close(codecCtx);

      // Close the video file
      avformat_close_input(&formatCtx);

    //  销毁全局对象
//      (*threadEnv)->DeleteGlobalRef(pEnv, gJavaClass);
//        //释放当前线程
//        (*gJavaVM)->DetachCurrentThread(gJavaVM);
//        pthread_exit(0);
        LOGI("thread stopdddd....");

}

jobject createBitmapFromJni(JNIEnv *pEnv, jint pWidth, jint pHeight) {
    int i;
    //get Bitmap class and createBitmap method ID
    jclass javaBitmapClass = (jclass)(*pEnv)->FindClass(pEnv, "android/graphics/Bitmap");
    jmethodID mid = (*pEnv)->GetStaticMethodID(pEnv, javaBitmapClass, "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    //create Bitmap.Config
    //reference: https://forums.oracle.com/thread/1548728

    const wchar_t* configName = L"ARGB_8888";
    int len = wcslen(configName);
    jstring jConfigName;

    if (sizeof(wchar_t) != sizeof(jchar)) {
        //wchar_t is defined as different length than jchar(2 bytes)
        jchar* str = (jchar*)malloc((len+1)*sizeof(jchar));
        for (i = 0; i < len; ++i) {
          str[i] = (jchar)configName[i];
        }
        str[len] = 0;
        jConfigName = (*pEnv)->NewString(pEnv, (const jchar*)str, len);
    } else {
        //wchar_t is defined same length as jchar(2 bytes)
        jConfigName = (*pEnv)->NewString(pEnv, (const jchar*)configName, len);
    }

    jclass bitmapConfigClass = (*pEnv)->FindClass(pEnv, "android/graphics/Bitmap$Config");
    jobject javaBitmapConfig = (*pEnv)->CallStaticObjectMethod(pEnv, bitmapConfigClass,
    (*pEnv)->GetStaticMethodID(pEnv, bitmapConfigClass, "valueOf", "(Ljava/lang/String;)Landroid/graphics/Bitmap$Config;"), jConfigName);

    //create the bitmap
    return (*pEnv)->CallStaticObjectMethod(pEnv, javaBitmapClass, mid, pWidth, pHeight, javaBitmapConfig);
}

jint JNI_OnLoad(JavaVM* pVm, void* reserved) {
	JNIEnv* env;
	if ((*pVm)->GetEnv(pVm, (void **)&env, JNI_VERSION_1_6) != JNI_OK) {
		 return -1;
	}
	return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif
