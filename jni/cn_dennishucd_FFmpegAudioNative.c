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
#include <assert.h>

#include <stdio.h>
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


void createEngine(JNIEnv* env, jobject obj)
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
}

void createBufferQueueAudioPlayer(JNIEnv* env, jobject obj, int rate, int channel,int bitsPerSample)
{
    SLresult result;
    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
//    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_16,
//        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
//        SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN};
    SLDataFormat_PCM format_pcm;
    format_pcm.formatType = SL_DATAFORMAT_PCM; //数据格式定义
	format_pcm.numChannels = channel;
	format_pcm.samplesPerSec = rate * 1000; //采样率(hz)
	format_pcm.bitsPerSample = bitsPerSample;
	format_pcm.containerSize = 16;
	if(channel == 2)
		format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
	else
		format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
	format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
	SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    // create audio player
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
            3, ids, req);
    assert(SL_RESULT_SUCCESS == result);
// realize the player
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // get the play interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    assert(SL_RESULT_SUCCESS == result);

    // get the buffer queue interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
            &bqPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == result);

    // register callback on the buffer queue
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
    assert(SL_RESULT_SUCCESS == result);

    // get the effect send interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
            &bqPlayerEffectSend);
    assert(SL_RESULT_SUCCESS == result);

	#if 0   // mute/solo is not supported for sources that are known to be mono, as this is
		// get the mute/solo interface
		result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_MUTESOLO, &bqPlayerMuteSolo);
		assert(SL_RESULT_SUCCESS == result);
	#endif

    // get the volume interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    assert(SL_RESULT_SUCCESS == result);

	// set the player's state to playing
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    assert(SL_RESULT_SUCCESS == result);

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
JNIEXPORT jint JNICALL Java_cn_dennishucd_FFmpegAudioNative_audioPlayer(JNIEnv *env, jobject obj, jstring fileName){

	  const char* local_title = (*env)->GetStringUTFChars(env, fileName, NULL);
        av_register_all();//注册所有支持的文件格式以及编解码器
        /*
         *只读取文件头，并不会填充流信息
         */
        if(avformat_open_input(&pFormatCtx, local_title, NULL, NULL) != 0)
            return -1;
        /*
         *获取文件中的流信息，此函数会读取packet，并确定文件中所有流信息，
         *设置pFormatCtx->streams指向文件中的流，但此函数并不会改变文件指针，
         *读取的packet会给后面的解码进行处理。
         */
        if(avformat_find_stream_info(pFormatCtx, NULL) < 0)
            return -1;
        /*
         *输出文件的信息，也就是我们在使用ffmpeg时能够看到的文件详细信息，
         *第二个参数指定输出哪条流的信息，-1代表ffmpeg自己选择。最后一个参数用于
         *指定dump的是不是输出文件，我们的dump是输入文件，因此一定要为0
         */
        av_dump_format(pFormatCtx, -1, local_title, 0);
        int i = 0;
        for(i=0; i< pFormatCtx->nb_streams; i++)
        {
            if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
                audioStream = i;
                break;
            }
        }

        if(audioStream < 0)return -1;
        aCodecCtx = pFormatCtx->streams[audioStream]->codec;
        aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
        if(avcodec_open2(aCodecCtx, aCodec, NULL) < 0)return -1;
        aFrame = avcodec_alloc_frame();
        if(aFrame == NULL)return -1;
        int ret;
        createEngine(env, obj);
        int flag_start = 0;
        while(videoFlag != -1)
        {
            if(av_read_frame(pFormatCtx, &packet) < 0)break;
            if(packet.stream_index == audioStream)
            {
                ret = avcodec_decode_audio4(aCodecCtx, aFrame, &frameFinished, &packet);
                if(ret > 0 && frameFinished)
                {
                    if(flag_start == 0)
                    {
                        flag_start = 1;
                        createBufferQueueAudioPlayer(env, obj, aCodecCtx->sample_rate, aCodecCtx->channels,
                        SL_PCMSAMPLEFORMAT_FIXED_16);
                    }
                    int data_size = av_samples_get_buffer_size(
                            aFrame->linesize,aCodecCtx->channels,
                            aFrame->nb_samples,aCodecCtx->sample_fmt, 1);
                    LOGI("audioDecodec  :%d : %d, :%d    :%d",data_size,aCodecCtx->channels,aFrame->nb_samples,aCodecCtx->sample_rate);
                    (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, aFrame->data[0], data_size);
                }

            }
            usleep(5000);
            while(videoFlag != 0)
            {
                if(videoFlag == 1)//暂停
                {
                    sleep(1);
                }else if(videoFlag == -1) //停止
                {
                    break;
                }
            }
            av_free_packet(&packet);
        }
        av_free(aFrame);
        avcodec_close(aCodecCtx);
        avformat_close_input(&pFormatCtx);
        (*env)->ReleaseStringUTFChars(env, fileName, local_title);
}

/*
 * Class:     cn_dennishucd_FFmpegAudioNative
 * Method:    audioPlayerPauseOrPlay
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cn_dennishucd_FFmpegAudioNative_audioPlayerPauseOrPlay(JNIEnv *env, jobject obj){
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
JNIEXPORT jint JNICALL Java_cn_dennishucd_FFmpegAudioNative_audioPlayerStop(JNIEnv *env, jobject obj){

	videoFlag = -1;
}
#ifdef __cplusplus
}
#endif
