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

// aux effect on the output mix, used by the buffer queue player
static const SLEnvironmentalReverbSettings reverbSettings =
    SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

// file descriptor player interfaces
static SLObjectItf fdPlayerObject = NULL;
static SLPlayItf fdPlayerPlay;
static SLSeekItf fdPlayerSeek;
static SLMuteSoloItf fdPlayerMuteSolo;
static SLVolumeItf fdPlayerVolume;

// pointer and size of the next player buffer to enqueue, and number of remaining buffers
static short *nextBuffer;
static unsigned nextSize;
static int nextCount;

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
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


void createEngine(JNIEnv* env, jclass clz)
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
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    assert(SL_RESULT_SUCCESS == result);

    // realize the output mix
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
            &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
    }
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

void createBufferQueueAudioPlayer(JNIEnv* env, jclass clz, int rate, int channel,int bitsPerSample)
{
//   const char* utf8Uri = env->GetStringUTFChars(uri, NULL);

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

void AudioWrite(const void*buffer, int size)
{
    (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, buffer, size);
}

/*
 * Class:     cn_dennishucd_FFmpegAudioNative
 * Method:    audioPlayer
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_cn_dennishucd_FFmpegAudioNative_audioPlayer(JNIEnv *env, jclass clz, jstring fileName,
jstring outputFile){

	  const char* local_title = (*env)->GetStringUTFChars(env, fileName, NULL);
	  const char* output_title = (*env)->GetStringUTFChars(env, outputFile, NULL);

	  LOGI("fileName is %s", local_title);
	  LOGI("out fileName is %s", output_title);
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
              printf("Couldn't open input stream.\n");
              return -1;
          }
          // Retrieve stream information
          if(avformat_find_stream_info(pFormatCtx,NULL)<0){
              printf("Couldn't find stream information.\n");
              return -1;
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
              printf("Didn't find a audio stream.\n");
              return -1;
          }

          // Get a pointer to the codec context for the audio stream
          pCodecCtx=pFormatCtx->streams[audioStream]->codec;

          // Find the decoder for the audio stream
          pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
          if(pCodec==NULL){
              printf("Codec not found.\n");
              return -1;
          }

          // Open codec
          if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
              printf("Could not open codec.\n");
              return -1;
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

          out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
          pFrame=av_frame_alloc();

          //FIX:Some Codec's Context Information is missing
          in_channel_layout=av_get_default_channel_layout(pCodecCtx->channels);
          //Swr
          au_convert_ctx = swr_alloc();
          au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,out_channel_layout, AV_SAMPLE_FMT_S16, out_sample_rate,
              in_channel_layout,pCodecCtx->sample_fmt , pCodecCtx->sample_rate,0, NULL);
          swr_init(au_convert_ctx);
		    createEngine(env, clz);
		    int flag_start = 0;
          while(av_read_frame(pFormatCtx, packet)>=0){
              if(packet->stream_index==audioStream){

                  ret = avcodec_decode_audio4( pCodecCtx, pFrame, &got_picture, packet);
                  if ( ret < 0 ) {
                      printf("Error in decoding audio frame.\n");
                      return -1;
                  }
                  if ( got_picture > 0 ){
                      swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)pFrame->data , pFrame->nb_samples);

                      printf("index:%5d\t pts:%lld\t packet size:%d\n",index,packet->pts,packet->size);
                      //Write PCM
//                      fwrite(out_buffer, 1, out_buffer_size, pFile);
                      index++;
                      if(flag_start == 0)
						{
							flag_start = 1;
							createBufferQueueAudioPlayer(env, clz, pCodecCtx->sample_rate, pCodecCtx->channels, SL_PCMSAMPLEFORMAT_FIXED_16);
						}
//						int data_size = av_samples_get_buffer_size(
//						pFrame->linesize,pCodecCtx->channels,
//						pFrame->nb_samples,pCodecCtx->sample_fmt, 1);
//						LOGI("audioDecodec  :%d : %d, :%d    :%d",data_size,pCodecCtx->channels,pFrame->nb_samples,pCodecCtx->sample_rate);
//						(*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, pFrame->data[0], data_size);

						LOGI("audioDecodec  :%d : %d, :%d    :%d", out_buffer_size,pCodecCtx->channels,
						pFrame->nb_samples,pCodecCtx->sample_rate);

						(*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, out_buffer, out_buffer_size);
                  }
              }
              usleep(10000);
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

//      FILE* pcm;
//      pcm = fopen(output_title,"wb");
//
//       av_register_all();//注册所有支持的文件格式以及编解码器
//          /*
//           *只读取文件头，并不会填充流信息
//           */
//          if(avformat_open_input(&pFormatCtx, local_title, NULL, NULL) != 0)
//              return -1;
//          /*
//           *获取文件中的流信息，此函数会读取packet，并确定文件中所有流信息，
//           *设置pFormatCtx->streams指向文件中的流，但此函数并不会改变文件指针，
//           *读取的packet会给后面的解码进行处理。
//           */
//          if(avformat_find_stream_info(pFormatCtx, NULL) < 0)
//              return -1;
//          /*
//           *输出文件的信息，也就是我们在使用ffmpeg时能够看到的文件详细信息，
//           *第二个参数指定输出哪条流的信息，-1代表ffmpeg自己选择。最后一个参数用于
//           *指定dump的是不是输出文件，我们的dump是输入文件，因此一定要为0
//           */
//          av_dump_format(pFormatCtx, -1, local_title, 0);
//          int i = 0;
//          for(i=0; i< pFormatCtx->nb_streams; i++)
//          {
//              if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
//                  audioStream = i;
//                  break;
//              }
//          }
//
//          if(audioStream < 0)return -1;
//          aCodecCtx = pFormatCtx->streams[audioStream]->codec;
//          aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
//
//          if(avcodec_open2(aCodecCtx, aCodec, NULL) < 0)return -1;
//          aFrame = av_frame_alloc();
//          if(aFrame == NULL)return -1;
//          int ret;
//           SwrContext* swrContext = NULL;
//
//           swrContext = swr_alloc_set_opts(swrContext,
//               aCodecCtx->channel_layout, // out channel layout
//               aCodecCtx->sample_fmt, // out sample format
//               aCodecCtx->sample_rate, // out sample rate
//               aCodecCtx->channel_layout, // in channel layout
//               aCodecCtx->sample_fmt, // in sample format
//               aCodecCtx->sample_rate, // in sample rate
//               0, // log offset
//               NULL); // log context
//           swr_init(swrContext);
//
//           if(aCodecCtx->sample_fmt==AV_SAMPLE_FMT_S16)
//		   {
//			   swrContext = swr_alloc_set_opts(swrContext,
//				   aCodecCtx->channel_layout,
//				   AV_SAMPLE_FMT_S16P,
//				   aCodecCtx->sample_rate,
//				   aCodecCtx->channel_layout,
//				   AV_SAMPLE_FMT_S16,
//				   aCodecCtx->sample_rate,
//				   0,
//				   0);
//			   if(swr_init(swrContext)<0)
//			   {
//				   printf("swr_init() for AV_SAMPLE_FMT_S16P fail");
//				   return 0;
//			   }
//		   }
//		   else if(aCodecCtx->sample_fmt==AV_SAMPLE_FMT_FLT)
//		   {
//			   swrContext = swr_alloc_set_opts(swrContext,
//				   aCodecCtx->channel_layout,
//				   AV_SAMPLE_FMT_S16P,
//				   aCodecCtx->sample_rate,
//				   aCodecCtx->channel_layout,
//				   AV_SAMPLE_FMT_FLT,
//				   aCodecCtx->sample_rate,
//				   0,
//				   0);
//			   if(swr_init(swrContext)<0)
//			   {
//				   printf("swr_init() for AV_SAMPLE_FMT_S16P fail");
//				   return 0;
//			   }
//		   }
//
//		   int bufSize = av_samples_get_buffer_size(NULL, aCodecCtx->channels, aCodecCtx->sample_rate,
//		   aCodecCtx->sample_fmt, 0);
//		   uint8_t* buf = (uint8_t *)malloc(bufSize);
//
//		   av_init_packet(&packet);
//
//		   int packetCount = 0;
//		   int decodedFrameCount = 0;
//
//		   static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
//		   uint8_t *out[]={audio_buf};
//
//   		   uint8_t *pktdata;
//
//		   int pktsize,flush_complete=0;
//
//
////          createEngine(env, clz);
//          int flag_start = 0;
//          while(videoFlag != -1)
//          {
//
//              if(av_read_frame(pFormatCtx, &packet) < 0)break;
//               ++packetCount;
//                if (packet.stream_index == audioStream)
//			   {
//					pktdata = packet.data;
//					pktsize = packet.size;
//				   int frameFinished = 0;
//				   int len = avcodec_decode_audio4(aCodecCtx, aFrame, &frameFinished, &packet);
//
//				   if (frameFinished)
//				   {
//					   pktsize -= len;
//					   pktdata += len;
//					   int data_size = av_samples_get_buffer_size(NULL,aCodecCtx->channels,aFrame->nb_samples,
//					   aCodecCtx->sample_fmt, 1);
//
//					   /*****************************************************
//					   以下代码使用swr_convert函数进行转换，但是转换后的文件连mp3到pcm文件都不能播放了，所以注释了
//					   const uint8_t *in[] = {aFrame->data[0]};
//
//					   int len=swr_convert(swrContext,out,sizeof(audio_buf)/aCodecCtx->channels/av_get_bytes_per_sample(AV_SAMPLE_FMT_S16P),
//						   in,aFrame->linesize[0]/aCodecCtx->channels/av_get_bytes_per_sample(aCodecCtx->sample_fmt));
//
//					   len=len*aCodecCtx->channels*av_get_bytes_per_sample(AV_SAMPLE_FMT_S16P);
//
//					   if (len < 0) {
//						   fprintf(stderr, "audio_resample() failed\n");
//						   break;
//					   }
//					   if (len == sizeof(audio_buf) / aCodecCtx->channels / av_get_bytes_per_sample(AV_SAMPLE_FMT_S16P)) {
//						   fprintf(stderr, "warning: audio buffer is probably too small\n");
//						   swr_init(swrContext);
//					   }
//					   *****************************************************/
////						short *data = (char *)malloc(aFrame->nb_samples);
////						short *sample_buffer = (short *)aFrame->data[0];
////						short *sample_buffer1=(short*)aFrame->data[1];
////						int i;
////						for (i = 0; i < aFrame->nb_samples; i++)
////						{
////							data[i*2] = sample_buffer[i];
////							data[i*2+1] = sample_buffer1[i];
////						}
//
//					   char *data = (char *)malloc(data_size);
//					   short *sample_buffer = (short *)aFrame->data[0];
//					   int i;
//					   for (i = 0; i < data_size/2; i++)
//					   {
//						   data[i*2] = (char)(sample_buffer[i/2] & 0xFF);
//						   data[i*2+1] = (char)((sample_buffer[i/2] >>8) & 0xFF);
//
//					   }
//					   fwrite(data, data_size, 1, pcm);
//					   fflush(pcm);
//
//
//				   }
//			   }
//              if(packet.stream_index == audioStream)
//              {
//                  ret = avcodec_decode_audio4(aCodecCtx, aFrame, &frameFinished, &packet);
//                  if(ret > 0 && frameFinished)
//                  {
//                      if(flag_start == 0)
//                      {
//                          flag_start = 1;
////                          createBufferQueueAudioPlayer(env, clz, aCodecCtx->sample_rate, aCodecCtx->channels, SL_PCMSAMPLEFORMAT_FIXED_16);
//                      }
//                      int data_size = av_samples_get_buffer_size(
//                              aFrame->linesize,aCodecCtx->channels,
//                              aFrame->nb_samples,aCodecCtx->sample_fmt, 1);
//                      LOGI("audioDecodec  :%d : %d, :%d    :%d",data_size,aCodecCtx->channels,aFrame->nb_samples,aCodecCtx->sample_rate);
//                      fwrite(aFrame->data[0], 1, data_size, pcm);
//                      fflush(pcm);
////                      (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, aFrame->data[0], data_size);
//                  }
//
//              }
////              usleep(5000);
//              while(videoFlag != 0)
//              {
//                  if(videoFlag == 1)//暂停
//                  {
//                      sleep(1);
//                  }else if(videoFlag == -1) //停止
//                  {
//                      break;
//                  }
//              }
//              av_free_packet(&packet);
//          }
//          free(buf);
//          swr_free(&swrContext);
//          av_free(aFrame);
//          avcodec_close(aCodecCtx);
//          avformat_close_input(&pFormatCtx);
		 LOGI("audioDecodec--decode finshed..........");


      (*env)->ReleaseStringUTFChars(env, fileName, local_title);
      (*env)->ReleaseStringUTFChars(env, fileName, output_title);

//            return 0;
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


  }


#ifdef __cplusplus
}
#endif
