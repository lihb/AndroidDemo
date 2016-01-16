#include <math.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
#include <libswresample/swresample.h>

#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>

#include <jni.h>
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <pthread.h>
#include <unistd.h>

// 添加opensl es支持
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "cn_dennishucd_FFmpegAudioNative.h"

/*for android logs*/
#include <android/log.h>

#define LOG_TAG "android-ffmpeg-lihb-test"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__);
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);
#ifdef __cplusplus
extern "C" {
#endif



AVFormatContext *pFormatCtx = NULL;
int             audioStream, delay_time, videoFlag = 0;
AVCodecContext  *aCodecCtx;
AVCodec         *aCodec;
AVFrame         *aFrame;
AVPacket        packet;
int  frameFinished = 0;

// engine interfaces
static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine;

// output mix interfaces
static SLObjectItf outputMixObject = NULL;
static SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;

// buffer queue player interfaces
static SLObjectItf bqPlayerObject = NULL;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLEffectSendItf bqPlayerEffectSend;
static SLMuteSoloItf bqPlayerMuteSolo;
static SLVolumeItf bqPlayerVolume;


// pointer and size of the next player buffer to enqueue, and number of remaining buffers
static short *nextBuffer;
static unsigned nextSize;
static int nextCount;

JavaVM      *gJavaVM;
jobject     gJavaObj;

jstring glocal_title;
jstring goutput_title;

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio


// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    assert(bq == bqPlayerBufferQueue);
    assert(NULL == context);
    // for streaming playback, replace this test by logic to find and fill the next buffer
    if (--nextCount > 0 && NULL != nextBuffer && 0 != nextSize) {
        SLresult result;
        // enqueue another buffer
        result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, nextBuffer, nextSize);
        // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
        // which for this code example would indicate a programming error
        assert(SL_RESULT_SUCCESS == result);
    }
}


void createEngine()
{
    SLresult result;

    // create engine
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);

    // realize the engine
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);

    // create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
//    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, 0, 0);
    assert(SL_RESULT_SUCCESS == result);

    // realize the output mix
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
//    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
//            &outputMixEnvironmentalReverb);
//    if (SL_RESULT_SUCCESS == result) {
//        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
//                outputMixEnvironmentalReverb, &reverbSettings);
//    }
    // ignore unsuccessful result codes for environmental reverb, as it is optional for this example

//    SLresult result;
//
//    	// create engine
//    	result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
//    	assert(SL_RESULT_SUCCESS == result);
//    	(void) result;
//
//    	// realize the engine
//    	result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
//    	assert(SL_RESULT_SUCCESS == result);
//    	(void) result;
//
//    	// get the engine interface, which is needed in order to create other objects
//    	result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
//    	assert(SL_RESULT_SUCCESS == result);
//    	(void) result;
//
//    	// create output mix,
//    	result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, 0, 0);
//    	assert(SL_RESULT_SUCCESS == result);
//    	(void) result;
//
//    	// realize the output mix
//    	result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
//    	assert(SL_RESULT_SUCCESS == result);
//    	(void) result;
}

void createBufferQueueAudioPlayer(int rate, int channel,int bitsPerSample)
{
   	SLresult result;
   	// configure audio source
   	SLDataLocator_AndroidSimpleBufferQueue loc_bufq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
//   	SLDataFormat_PCM format_pcm = { SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_48,
//   	SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
//   	SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN };

   	 SLDataFormat_PCM format_pcm;
     format_pcm.formatType = SL_DATAFORMAT_PCM;
     format_pcm.numChannels = channel;
     format_pcm.samplesPerSec = rate * 1000;
     format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
     format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;

    if(channel == 2)
    	format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    else
   		format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;

    format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

   	SLDataSource audioSrc = { &loc_bufq, &format_pcm };

   	// configure audio sink
   	SLDataLocator_OutputMix loc_outmix = { SL_DATALOCATOR_OUTPUTMIX, outputMixObject };
   	SLDataSink audioSnk = { &loc_outmix, NULL };

   	// create audio player
   	const SLInterfaceID ids[3] = { SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME };
   	const SLboolean req[3] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
   	result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk, 3, ids, req);
   	assert(SL_RESULT_SUCCESS == result);
   	(void) result;

   	// realize the player
   	result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
   	assert(SL_RESULT_SUCCESS == result);
   	(void) result;

   	// get the play interface
   	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
   	assert(SL_RESULT_SUCCESS == result);
   	(void) result;

   	// get the buffer queue interface
   	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue);
   	assert(SL_RESULT_SUCCESS == result);
   	(void) result;

//   	mThread = new PlaybackThread(utf8Uri);
   	// register callback on the buffer queue
   	result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
   	assert(SL_RESULT_SUCCESS == result);
   	(void) result;

   	// get the effect send interface
   	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND, &bqPlayerEffectSend);
   	assert(SL_RESULT_SUCCESS == result);
   	(void) result;

   	// get the volume interface
   	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
   	assert(SL_RESULT_SUCCESS == result);
   	(void) result;

   	// set the player's state to playing
   	result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
   	assert(SL_RESULT_SUCCESS == result);
   	(void) result;
   	//pthread_t id;

//   	mThread->start();
//   	env->ReleaseStringUTFChars(uri, utf8Uri);
//   	ALOGD("createAudioPlayer finish");
//   	return 0;

}

/*
 * Class:     cn_dennishucd_FFmpegAudioNative
 * Method:    startAudioPlayer
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cn_dennishucd_FFmpegAudioNative_startAudioPlayer(JNIEnv *env, jobject obj, jstring fileName,
    jstring outputFile){

   //全局化变量
     (*env)->GetJavaVM(env, &gJavaVM);
     gJavaObj = (*env)->NewGlobalRef(env,obj);

     glocal_title = (*env)->NewGlobalRef(env,fileName);
     goutput_title = (*env)->NewGlobalRef(env,outputFile);


     pthread_t decodeThread;

     pthread_create(&decodeThread, NULL, decodeAudio, NULL);

  }


static void* decodeAudio(void *arg){
  JNIEnv    *threadEnv;
      // 注册线程
      int status = (*gJavaVM)->AttachCurrentThread(gJavaVM, &threadEnv, NULL);

      const char* local_title = (*threadEnv)->GetStringUTFChars(threadEnv, glocal_title, NULL);
      const char* output_title = (*threadEnv)->GetStringUTFChars(threadEnv, goutput_title, NULL);

	  LOGI("fileName is %s", local_title);
	  LOGI("out fileName is %s", output_title);
//	  LOGI("thread is %d", thread_self());
	  AVFormatContext *pFormatCtx;
      int             i, audioStream;
      AVCodecContext  *pCodecCtx;
      AVCodec         *pCodec;
      AVPacket        *packet;
      uint8_t         *out_buffer;
      AVFrame         *pFrame;
      int ret;
      uint32_t len = 0;
      int got_picture;
      int index = 0;
      int64_t in_channel_layout;
      struct SwrContext *au_convert_ctx;

      FILE *pFile=fopen(output_title, "wb");

      av_register_all();
      avformat_network_init();
      pFormatCtx = avformat_alloc_context();
      //Open
      if(avformat_open_input(&pFormatCtx,local_title,NULL,NULL)!=0){
          LOGI("Couldn't open input stream.\n");

      }
      // Retrieve stream information
      if(avformat_find_stream_info(pFormatCtx,NULL)<0){
          LOGI("Couldn't find stream information.\n");
      }
      // Dump valid information onto standard error
      av_dump_format(pFormatCtx, -1, local_title, 0);

      // Find the first audio stream
      audioStream=-1;
      for(i=0; i < pFormatCtx->nb_streams; i++)
          if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
              audioStream=i;
              break;
          }

      if(audioStream==-1){
          LOGI("Didn't find a audio stream.\n");
      }

      // Get a pointer to the codec context for the audio stream
      pCodecCtx=pFormatCtx->streams[audioStream]->codec;

      // Find the decoder for the audio stream
      pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
      if(pCodec==NULL){
          LOGI("Codec not found.\n");
      }

      // Open codec
      if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
          LOGI("Could not open codec.\n");
      }

      packet=(AVPacket *)av_malloc(sizeof(AVPacket));
      av_init_packet(packet);

      //Out Audio Param
      uint64_t out_channel_layout=AV_CH_LAYOUT_STEREO;
      //nb_samples: AAC-1024 MP3-1152
      int out_nb_samples=pCodecCtx->frame_size;
//          AVSampleFormat out_sample_fmt=AV_SAMPLE_FMT_S16;
      int out_sample_rate= 48000;
      int out_channels=av_get_channel_layout_nb_channels(out_channel_layout);
      //Out Buffer Size
      int out_buffer_size=av_samples_get_buffer_size(NULL,out_channels ,out_nb_samples, AV_SAMPLE_FMT_S16, 1);

      LOGI("out_sample_rate  :%d，channels : %d, out_buffer_size :%d", out_sample_rate,out_channels,out_buffer_size);

      out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
      pFrame=av_frame_alloc();

      //FIX:Some Codec's Context Information is missing
      in_channel_layout=av_get_default_channel_layout(pCodecCtx->channels);
      //Swr
      au_convert_ctx = swr_alloc();
      au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,out_channel_layout, AV_SAMPLE_FMT_S16, out_sample_rate,
          in_channel_layout,pCodecCtx->sample_fmt , pCodecCtx->sample_rate,0, NULL);
      swr_init(au_convert_ctx);
      createEngine();
      int flag_start = 0;
      while(av_read_frame(pFormatCtx, packet)>=0){
          if(packet->stream_index==audioStream){

              ret = avcodec_decode_audio4( pCodecCtx, pFrame, &got_picture, packet);
              if ( ret < 0 ) {
                  LOGI("Error in decoding audio frame.\n");
              }
              if ( got_picture > 0 ){
                  swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)pFrame->data , pFrame->nb_samples);

                  LOGI("index:%5d\t pts:%lld\t packet size:%d\n",index,packet->pts,packet->size);
                  //Write PCM
//                      fwrite(out_buffer, 1, out_buffer_size, pFile);
                  index++;
                  if(flag_start == 0)
                    {
                        flag_start = 1;
                        createBufferQueueAudioPlayer(pCodecCtx->sample_rate, pCodecCtx->channels, SL_PCMSAMPLEFORMAT_FIXED_16);
                    }
                    LOGI("audioDecodec  :%d : %d, :%d    :%d", out_buffer_size,pCodecCtx->channels,
                    pFrame->nb_samples,pCodecCtx->sample_rate);

                    (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, out_buffer, out_buffer_size);
              }
          }
          usleep(11000);
          av_free_packet(packet);
      }

      swr_free(&au_convert_ctx);

      fclose(pFile);

      av_free(out_buffer);
      // Close the codec
      avcodec_close(pCodecCtx);
      // Close the video file
      avformat_close_input(&pFormatCtx);
       LOGI("audioDecodec--decode finshed..........");

//      (*threadEnv)->ReleaseStringUTFChars(threadEnv, fileName, local_title);
//      (*threadEnv)->ReleaseStringUTFChars(threadEnv, outputFile, output_title);
      //释放当前线程
      (*gJavaVM)->DetachCurrentThread(gJavaVM);

}

/*
 * Class:     cn_dennishucd_FFmpegAudioNative
 * Method:    audioPlayerPauseOrPlay
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cn_dennishucd_FFmpegAudioNative_audioPlayerPauseOrPlay(JNIEnv *env, jclass clz){
 	if(videoFlag == 1)
    {
        videoFlag = 0;
    }else if(videoFlag == 0){
        videoFlag = 1;
    }
    return videoFlag;


}

/*
 * Class:     cn_dennishucd_FFmpegAudioNative
 * Method:    audioPlayerStop
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cn_dennishucd_FFmpegAudioNative_audioPlayerStop(JNIEnv *env, jclass clz){

	videoFlag = -1;
}

/*
 * Class:     cn_dennishucd_FFmpegAudioNative
 * Method:    setPlayingAudioPlayer
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_cn_dennishucd_FFmpegAudioNative_setPlayingAudioPlayer
  (JNIEnv *env, jobject obj, jboolean isplaying){
  }

/*
 * Class:     cn_dennishucd_FFmpegAudioNative
 * Method:    setVolumeAudioPlayer
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_cn_dennishucd_FFmpegAudioNative_setVolumeAudioPlayer
  (JNIEnv *env, jobject obj, jint index){
  }

/*
 * Class:     cn_dennishucd_FFmpegAudioNative
 * Method:    setMutAudioPlayer
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_cn_dennishucd_FFmpegAudioNative_setMutAudioPlayer
  (JNIEnv *env, jobject obj, jboolean ispalying){
  }

/*
 * Class:     cn_dennishucd_FFmpegAudioNative
 * Method:    shutdown
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cn_dennishucd_FFmpegAudioNative_shutdown
  (JNIEnv *env, jobject obj){

  //destory player object
  	if (bqPlayerObject != NULL) {
  		(*bqPlayerObject)->Destroy(bqPlayerObject);
  		bqPlayerPlay = NULL;
  		bqPlayerBufferQueue = NULL;
  		bqPlayerEffectSend = NULL;
  		bqPlayerVolume = NULL;
  	}

  	// destroy output mix object, and invalidate all associated interfaces
  	if (outputMixObject != NULL) {
  		(*outputMixObject)->Destroy(outputMixObject);
  		outputMixObject = NULL;
  	}

  	// destroy engine object, and invalidate all associated interfaces
  	if (engineObject != NULL) {
  		(*engineObject)->Destroy(engineObject);
  		engineObject = NULL;
  		engineEngine = NULL;
  	}

  	 (*env)->DeleteGlobalRef(env, glocal_title);
  	 (*env)->DeleteGlobalRef(env, goutput_title);


  }


#ifdef __cplusplus
}
#endif
