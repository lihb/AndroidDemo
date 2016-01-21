#include <jni.h>

#include "cn_dennishucd_RTMPLibTest.h"

#include <stdio.h>
#include <librtmp/rtmp_sys.h>
#include <librtmp/log.h>

/*for android logs*/
#include <android/log.h>

#define LOG_TAG "android-ffmpeg-lihb-test"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__);
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     cn_dennishucd_RTMPLibTest
 * Method:    NativeMain
 * Signature: ()V
 */
JNIEXPORT jint JNICALL Java_cn_dennishucd_RTMPLibTest_naTest
  (JNIEnv *env, jobject obj, jstring outputFile){
		InitSockets();
		LOGI("enter naTest method()");

		char *outFileName = (char *)(*env)->GetStringUTFChars(env, outputFile, NULL);
 		LOGI("out fileName is %s", outFileName);
        double duration=-1;
        int nRead;
        //is live stream ?
        int bLiveStream=1;


        int bufsize=1024*1024*10;

        char *buf=(char*)malloc(bufsize);
        memset(buf,0,bufsize);
        long countbufsize=0;

        FILE *fp=fopen(outFileName,"wb");
        if (!fp){
            RTMP_LogPrintf("Open File Error.\n");
            LOGI("Open File Error.\n");
            CleanupSockets();
            return -1;
        }

        /* set log level */
        //RTMP_LogLevel loglvl=RTMP_LOGDEBUG;
        //RTMP_LogSetLevel(loglvl);

        RTMP *rtmp=RTMP_Alloc();
        RTMP_Init(rtmp);
        //set connection timeout,default 30s
        rtmp->Link.timeout=10;
        // HKS's live URL
        if(!RTMP_SetupURL(rtmp,"rtmp://183.61.143.98/flvplayback/mp4:panvideo1.mp4"))
        {
            RTMP_Log(RTMP_LOGERROR,"SetupURL Err\n");
            RTMP_Free(rtmp);
            CleanupSockets();
            LOGI("SetupURL Err\n");
            return -1;
        }
        if (bLiveStream){
            rtmp->Link.lFlags|=RTMP_LF_LIVE;
        }

        //1hour
        RTMP_SetBufferMS(rtmp, 3600*1000);

        if(!RTMP_Connect(rtmp,NULL)){
            RTMP_Log(RTMP_LOGERROR,"Connect Err\n");
            RTMP_Free(rtmp);
            CleanupSockets();
            LOGI("Connect Err\n");
            return -1;
        }

        if(!RTMP_ConnectStream(rtmp,0)){
            RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");
            RTMP_Close(rtmp);
            RTMP_Free(rtmp);
            CleanupSockets();
            LOGI("ConnectStream Err\n");
            return -1;
        }

        while(nRead=RTMP_Read(rtmp,buf,bufsize)){
            fwrite(buf,1,nRead,fp);

            countbufsize+=nRead;
            RTMP_LogPrintf("Receive: %5dByte, Total: %5.2fkB\n",nRead,countbufsize*1.0/1024);
            LOGI("Receive: %5dByte, Total: %5.2fkB\n",nRead,countbufsize*1.0/1024);
        }

        if(fp)
            fclose(fp);

        if(buf){
            free(buf);
        }

        if(rtmp){
            RTMP_Close(rtmp);
            RTMP_Free(rtmp);
            CleanupSockets();
            rtmp=NULL;
        }
        return 0;

  }

void InitSockets(){
	#ifdef WIN32
        WORD version;
        WSADATA wsaData;
        version = MAKEWORD(1, 1);
        return (WSAStartup(version, &wsaData) == 0);
    #endif
  }

void CleanupSockets(){
	#ifdef WIN32
        WSACleanup();
    #endif
  }

#ifdef __cplusplus
}
#endif
