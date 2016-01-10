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

#include "cn_dennishucd_FFmpegNative.h"

/*for android logs*/
#include <android/log.h>

#define LOG_TAG "android-ffmpeg-lihb-test"
#define LOGI(...) __android_log_print(4, LOG_TAG, __VA_ARGS__);
#define LOGE(...) __android_log_print(6, LOG_TAG, __VA_ARGS__);

#ifdef __cplusplus
extern "C" {
#endif

char        *videoFileName;
AVFormatContext   *formatCtx = NULL;
int         videoStream;
AVCodecContext    *codecCtx = NULL;
AVFrame           *decodedFrame = NULL;
AVFrame           *frameRGBA = NULL;
jobject       bitmap;
void*       buffer;
struct SwsContext   *sws_ctx = NULL;
int         width;
int         height;

JavaVM      *gJavaVM;
jobject     gJavaObj;


/*
 * Class:     cn_dennishucd_FFmpegNative
 * Method:    avcodec_find_decoder
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_cn_dennishucd_FFmpegNative_avcodec_1find_1decoder
  (JNIEnv *env, jobject obj, jint codecID)
{
  AVCodec *codec = NULL;

//  /* register all formats and codecs */
//  av_register_all();
//
//  codec = avcodec_find_decoder(codecID);

  if (codec != NULL)
  {
    return 0;
  }
  else
  {
    return -1;
  }
}

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
    gJavaObj = (*pEnv)->NewGlobalRef(pEnv,pObj);


//    jclass temp = (*pEnv)->FindClass(pEnv, "cn/dennishucd/FFmpegNative");
//    gFfmegNativecls = (*pEnv)->NewGlobalRef(pEnv, temp);

    videoFileName = (char *)(*pEnv)->GetStringUTFChars(pEnv, pFileName, NULL);
    LOGI("video file name is %s", videoFileName);
    // Register all formats and codecs
    av_register_all();
    // Open video file
    if(avformat_open_input(&formatCtx, videoFileName, NULL, NULL)!=0)
      return -1; // Couldn't open file
    // Retrieve stream information
    if(avformat_find_stream_info(formatCtx, NULL)<0)
      return -1; // Couldn't find stream information
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
    // Open codec
    if(avcodec_open2(codecCtx, pCodec, &optionsDict)<0)
      return -1; // Could not open codec
    // Allocate video frame
    decodedFrame=avcodec_alloc_frame();
    // Allocate an AVFrame structure
    frameRGBA=avcodec_alloc_frame();
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
      LOGI(4, "cannot allocate memory for video size");
      return NULL;
    }
    jint lVideoRes[2];
    lVideoRes[0] = codecCtx->width;
    lVideoRes[1] = codecCtx->height;
    (*pEnv)->SetIntArrayRegion(pEnv, lRes, 0, 2, lVideoRes);
    return lRes;

}

/*
* Class:     cn_dennishucd_FFmpegNative
* Method:    naSetup
* Signature: (II)I
*/
jint JNICALL Java_cn_dennishucd_FFmpegNative_naSetup(JNIEnv *pEnv, jobject pObj, jint pWidth, jint pHeight){
    width = pWidth;
    height = pHeight;
    //create a bitmap as the buffer for frameRGBA
    bitmap = createBitmap(pEnv, pWidth, pHeight);
    if (AndroidBitmap_lockPixels(pEnv, bitmap, &buffer) < 0)
      return -1;
    //get the scaling context
    sws_ctx = sws_getContext(
            codecCtx->width,
            codecCtx->height,
            codecCtx->pix_fmt,
            pWidth,
            pHeight,
            AV_PIX_FMT_RGBA,
            SWS_BILINEAR,
            NULL,
            NULL,
            NULL
    );
    // Assign appropriate parts of bitmap to image planes in pFrameRGBA
    // Note that pFrameRGBA is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)frameRGBA, buffer, AV_PIX_FMT_RGBA,
        pWidth, pHeight);
    return 0;
  }

 void JNICALL Java_cn_dennishucd_FFmpegNative_naPlay(JNIEnv *pEnv,  jobject pObj){
    //create a new thread for video decode and render
        pthread_t decodeThread;

        pthread_create(&decodeThread, NULL, decodeVideo, NULL);


 }

static int decodeVideo(){
  AVPacket  packet;
  int       frameFinished;
  JNIEnv    *threadEnv;
// Read frames and save first five frames to disk
 // 注册线程
  int status = (*gJavaVM)->AttachCurrentThread(gJavaVM, &threadEnv, NULL);
  LOGI("decodeVideo() --attach thread, status = %d" , status);

  int i=0;
  while(av_read_frame(formatCtx, &packet)>=0) {
    // Is this a packet from the video stream?
    if(packet.stream_index==videoStream) {
      // Decode video frame
      avcodec_decode_video2(codecCtx, decodedFrame, &frameFinished,
         &packet);
      // Did we get a video frame?
      if(frameFinished) {
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
        if(i == 150)
            break;

        // Save the frame to disk
        if(++i<=152) {
          LOGI("decodeVideo before saveFrame()");
//          SaveFrame(pEnv, bitmap, codecCtx->width, codecCtx->height);
           LOGI("saveFrame --begin");


            //  char szFilename[200];
            jmethodID sSaveFrameMID;
            //获取Java层对应的类
            	jclass javaClass = (*threadEnv)->GetObjectClass(threadEnv,gJavaObj);
            	if( javaClass == NULL ) {
            		LOGI("Fail to find javaClass");
            		return 0;
            	}

            	//获取Java层被回调的函数
//              jmethodID javaCallback = (*threadEnv)->GetMethodID(threadEnv,javaClass,"offer","(Landroid/graphics/Bitmap;)Z");
              jmethodID javaCallback = (*threadEnv)->GetMethodID(threadEnv,javaClass,"saveFrameToPath","(Landroid/graphics/Bitmap;Ljava/lang/String;)V");
              if( javaCallback == NULL) {
              	LOGI("Fail to find method onNativeCallback");
              	return 0;
              }

              //回调Java层的函数
               LOGI("call java method to save frame");
               char szFilename[200];

               sprintf(szFilename, "/sdcard/aaaaa/frame%d.jpg", i);
               jstring filePath = (*threadEnv)->NewStringUTF(threadEnv, szFilename);

              (*threadEnv)->CallVoidMethod(threadEnv,gJavaObj,javaCallback, bitmap, filePath);
//              (*threadEnv)->CallBooleanMethod(threadEnv,gJavaObj,javaCallback,bitmap);
               LOGI("call java method to save frame done");
              LOGI("save frame %d", i);
        }
      }
    }
    // Free the packet that was allocated by av_read_frame
    av_free_packet(&packet);

  }


  //unlock the bitmap
  AndroidBitmap_unlockPixels(threadEnv, bitmap);

  // Free the RGB image
  av_free(frameRGBA);

  // Free the YUV frame
  av_free(decodedFrame);

  // Close the codec
  avcodec_close(codecCtx);

  // Close the video file
  avformat_close_input(&formatCtx);

//  销毁全局对象
  (*threadEnv)->DeleteGlobalRef(threadEnv, gJavaObj);
    //释放当前线程
    (*gJavaVM)->DetachCurrentThread(gJavaVM);
    LOGI("thread stopdddd... %d.", i);

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



void SaveFrame(JNIEnv *pEnv, jobject pBitmap, int width, int height) {
//  LOGI("saveFrame --begin");
//
//
////  char szFilename[200];
//  jmethodID sSaveFrameMID;
//  //获取Java层对应的类
//  	jclass javaClass = (*env)->GetObjectClass(env,gJavaObj);
//  	if( javaClass == NULL ) {
//  		LOG("Fail to find javaClass");
//  		return 0;
//  	}
//
//  	//获取Java层被回调的函数
//    jmethodID javaCallback = (*env)->GetMethodID(env,javaClass,"offer","(Landroid/graphics/Bitmap;)Z");
//    if( javaCallback == NULL) {
//    	LOG("Fail to find method onNativeCallback");
//    	return 0;
//    }
//
//    //回调Java层的函数
//     LOGI("call java method to save frame");
//    (*env)->CallBooleanMethod(env,gJavaObj,javaCallback,pBitmap);
//     LOGI("call java method to save frame done");


//  sSaveFrameMID = (*pEnv)->GetStaticMethodID(pEnv, gFfmegNativecls, "offer", "(Landroid/graphics/Bitmap;)Z");

//  jstring filePath = (*pEnv)->NewStringUTF(pEnv, szFilename);
//  (*pEnv)->CallStaticBooleanMethod(pEnv, ffmpegNative, sSaveFrameMID, pBitmap);


//  if(sSaveFrameMID != NULL){
//    (*pEnv)->DeleteLocalRef(pEnv, sSaveFrameMID);
//  }



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
