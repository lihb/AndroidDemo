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

#include "cn_dennishucd_FFmpegNative.h"

/*for android logs*/
#include <android/log.h>

#define LOG_TAG "android-ffmpeg-lihb-test"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__);
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);

#ifdef __cplusplus
extern "C" {
#endif

char        *videoFileName;
AVFormatContext   *formatCtx = NULL;
int         videoStream;
AVCodecContext    *codecCtx = NULL;
AVFrame           *decodedFrame = NULL;
static AVFrame           *frameRGBA = NULL;
void*       buffer;
static struct SwsContext   *sws_ctx = NULL;

static JavaVM      *gJavaVM;
static jobject     mBitmap;
static jclass      gJavaClass;
static jmethodID   gMethodID;

/*
 * Class:     cn_dennishucd_FFmpegNative
 * Method:    naInit
 * Signature: (Ljava/lang/String;)I
 */
jint JNICALL Java_cn_dennishucd_FFmpegNative_naInit(JNIEnv *pEnv, jobject pObj, jstring pFileName){
    AVCodec         *pCodec = NULL;
    int             i;
    AVDictionary    *optionsDict = NULL;

    //全局化变量
    (*pEnv)->GetJavaVM(pEnv, &gJavaVM);
    jclass clazz = (*pEnv)->GetObjectClass(pEnv,pObj);
    gJavaClass = (jclass)(*pEnv)->NewGlobalRef(pEnv,clazz);

    gMethodID = (*pEnv)->GetStaticMethodID(pEnv,gJavaClass,"offer","(Landroid/graphics/Bitmap;)Z");

    videoFileName = (char *)(*pEnv)->GetStringUTFChars(pEnv, pFileName, NULL);
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
    return 0;
  }

/*
* Class:     cn_dennishucd_FFmpegNative
* Method:    naGetVideoRes
* Signature: ()[I
*/
jintArray JNICALL Java_cn_dennishucd_FFmpegNative_naGetVideoRes(JNIEnv *pEnv, jobject pObj){
   jintArray lRes;
    if (NULL == codecCtx) {
      return NULL;
    }
    lRes = (*pEnv)->NewIntArray(pEnv, 2);
    if (lRes == NULL) {
      LOGI("cannot allocate memory for video size.....");
      return NULL;
    }
    jint lVideoRes[2];
    lVideoRes[0] = codecCtx->width;
    lVideoRes[1] = codecCtx->height;
    (*pEnv)->SetIntArrayRegion(pEnv, lRes, 0, 2, lVideoRes);
     LOGI("naGetVideoRes() current thread id = %lu", pthread_self());
    return lRes;

}

/*
* Class:     cn_dennishucd_FFmpegNative
* Method:    naSetup
* Signature: (II)I
*/
jint JNICALL Java_cn_dennishucd_FFmpegNative_naSetup(JNIEnv *pEnv, jobject pObj){
    //create a bitmap as the buffer for frameRGBA
    jobject bitmap = createBitmap(pEnv, codecCtx->width, codecCtx->height);
    mBitmap = (jobject)(*pEnv)->NewGlobalRef(pEnv,bitmap);
    if (AndroidBitmap_lockPixels(pEnv, mBitmap, &buffer) < 0)
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
    return 0;
  }

 void JNICALL Java_cn_dennishucd_FFmpegNative_naPlay(JNIEnv *pEnv,  jobject pObj){
    //create a new thread for video decode and render
     pthread_t decodeThread;
     LOGI("naPlay() current thread id = %lu", pthread_self());
     pthread_create(&decodeThread, NULL, decodeVideo, NULL);


 }

static void* decodeVideo(void *arg){
  AVPacket  packet;
  int       frameFinished;
  JNIEnv    *threadEnv;
 // 注册线程
  int status = (*gJavaVM)->AttachCurrentThread(gJavaVM, &threadEnv, NULL);
  LOGI("decodeVideo() --attach thread, status = %d" , status);

  // Read frames and save first five frames to disk
  while(av_read_frame(formatCtx, &packet)>=0) {
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
          //回调Java层的函数
          LOGI("call java method to offer bitmap");
          (*threadEnv)->CallStaticBooleanMethod(threadEnv, gJavaClass, gMethodID, mBitmap);
          LOGI("call java method to offer bitmap done.");
        }
    }
    // Free the packet that was allocated by av_read_frame
    av_free_packet(&packet);

  }
    //unlock the bitmap
    AndroidBitmap_unlockPixels(threadEnv, mBitmap);

    // Free the RGB image
    av_free(frameRGBA);

    // Free the YUV frame
    av_free(decodedFrame);

    // Close the codec
    avcodec_close(codecCtx);

    // Close the video file
    avformat_close_input(&formatCtx);

    // 销毁全局对象
    (*threadEnv)->DeleteGlobalRef(threadEnv, gJavaClass);
    (*threadEnv)->DeleteGlobalRef(threadEnv, mBitmap);
    //释放当前线程
    (*gJavaVM)->DetachCurrentThread(gJavaVM);
    pthread_exit(0);
    LOGI("thread stopdddd....");

  return 0;

}



jobject createBitmap(JNIEnv *pEnv, jint pWidth, jint pHeight) {
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
//	JNINativeMethod nm[1];
//	nm[0].name = "naMain";
//	nm[0].signature = "(Lroman10/tutorial/android_ffmpeg_tutorial01/MainActivity;Ljava/lang/String;I)I";
//	nm[0].fnPtr = (void*)naMain;
//	jclass cls = (*env)->FindClass(env, "roman10/tutorial/android_ffmpeg_tutorial01/MainActivity");
//	//Register methods with env->RegisterNatives.
//	(*env)->RegisterNatives(env, cls, nm, 1);
	return JNI_VERSION_1_6;
}


#ifdef __cplusplus
}
#endif
