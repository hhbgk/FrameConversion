/**
 * Description:
 * Author:created by hhbgk on 17-5-30.
 */
//
#include <jni.h>
#include <assert.h>
#include <stdio.h>
#include "jl_frm_codec_jni.h"
#include "frame_codec.h"

static JavaVM *gJVM = NULL;
static jobject gObj = NULL;
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
//#define CLASS_PATH "tv/danmaku/ijk/media/player/FrameCodec"
#define CLASS_PATH "com/jieli/media/codec/FrameCodec"
static context_t gFrame;
static jmethodID on_codec_completed_cb;
static jmethodID on_codec_error_cb;
static jboolean jni_init(JNIEnv *env, jobject thiz)
{
    //logi("%s:", __func__);
    gObj = (*env)->NewGlobalRef(env, thiz);
    (*env)->GetJavaVM(env, &gJVM);
    jclass clazz = (*env)->GetObjectClass(env, thiz);
    if (clazz == NULL)
    {
        (*env)->ThrowNew(env, "java/lang/NullPointerException", "Unable to find exception class");
    }

    on_codec_completed_cb = (*env)->GetMethodID(env, clazz, "onCodecSuccess", "([BIIILjava/lang/String;)V");
    on_codec_error_cb = (*env)->GetMethodID(env, clazz, "onCodecError", "(Ljava/lang/String;)V");
    if (!on_codec_completed_cb || !on_codec_error_cb)
    {
		loge("The calling class does not implement all necessary interface methods");
		return JNI_FALSE;
	}

    return JNI_TRUE;
}

static void on_frame_codec (void *data)
{
    //logi ("%s", __func__);
    const context_t *pd = data;
    assert(pd != NULL);
//logi ("on_video_frame .......1...");
    assert(gJVM != NULL);
    JNIEnv *env = NULL;
    jboolean isAttached = JNI_FALSE;

    if ((*gJVM)->GetEnv(gJVM, (void**) &env, JNI_VERSION_1_6) < 0)
    {
        if ((*gJVM)->AttachCurrentThread(gJVM, &env, NULL) < 0)
        {
            loge("AttachCurrentThread failed");
            isAttached = JNI_FALSE;
        }
        else
        {
            isAttached = JNI_TRUE;
        }
    }
    assert(env != NULL);
    //logi ("on_video_frame ....2......");
    jbyteArray jArray = NULL;
    jstring jpath = NULL;

    //logi ("on_video_frame ....5.....pd=%p", pd);
    if (pd->buf)
    {
        //logi ("on_video_frame ....5.....offset=%d", pd->v_offset);
        jArray = (*env)->NewByteArray(env, pd->size);
        (*env)->SetByteArrayRegion (env, jArray, 0, pd->size, (jbyte*)(pd->buf));
    }
    else
    {
        logw ("No data");
    }
    if (pd->meta && strlen(pd->meta->path))
        jpath = (*env)->NewStringUTF(env, (const char *) pd->meta->path);
    //logi ("on_video_frame ....6.....size=%d", size);
    (*env)->CallVoidMethod (env, gObj, on_codec_completed_cb, jArray,
                           (jint)pd->meta->width, (jint)pd->meta->height, (jint)pd->meta->duration, jpath);
loge ("on_video_frame ....a.....size=%d");
    if (jArray != NULL)
        (*env)->DeleteLocalRef(env, jArray);
loge ("on_video_frame ....b.....size=%d");
     if (jpath != NULL)
        (*env)->DeleteLocalRef(env, jpath);
loge ("on_video_frame ....c.....size");
    if (isAttached)
        (*gJVM)->DetachCurrentThread(gJVM);
loge ("on_video_frame ....d.....size=");
    if (pd->meta)
       free(pd->meta);
loge ("on_video_frame ....e.....size=");
}
static void on_error (const char *msg)
{
    //logi ("on_video_frame ..........");
    const char *pd = msg;
    assert(pd != NULL);
//logi ("on_video_frame .......1...");
    assert(gJVM != NULL);
    JNIEnv *env = NULL;
    jboolean isAttached = JNI_FALSE;

    if ((*gJVM)->GetEnv(gJVM, (void**) &env, JNI_VERSION_1_6) < 0)
    {
        if ((*gJVM)->AttachCurrentThread(gJVM, &env, NULL) < 0)
        {
            loge("AttachCurrentThread failed");
            return;
        }
        isAttached = JNI_TRUE;
    }
    assert(env != NULL);
//logi ("on_video_frame ....2......");
    jstring jmessage = (*env)->NewStringUTF(env, pd);

    //logi ("on_video_frame ....6.....packetHdr->sequence=%d", packetHdr->sequence);
    (*env)->CallVoidMethod (env, gObj, on_codec_error_cb, jmessage);
    if (jmessage != NULL)
        (*env)->DeleteLocalRef(env, jmessage);
    if (isAttached)
        (*gJVM)->DetachCurrentThread(gJVM);
}
/*********************************************************************************************
解码一帧数据为JPG, 并初始化媒体信息
***********/
static jboolean jni_convert(JNIEnv *env, jobject thiz, jbyteArray jdata, jint len, jint jw, jint jh, jstring jpath)
{
    unsigned char *buf = (unsigned char *) (*env)->GetByteArrayElements(env, jdata, NULL);
    if (buf == NULL)
        return JNI_FALSE;
    context_t *pFrame = &gFrame;
    unsigned char *path = NULL;
    jint jlen = 0;
    if (jpath != NULL)
    {
        jlen = (jint)(*env)->GetStringLength(env,jpath);
        path = (unsigned char *) (*env)->GetStringUTFChars(env, jpath, NULL);
        //logw("===============path=%s, len=%d", path, jlen);
        pFrame->meta = calloc(1, sizeof(av_meta_t) + sizeof(uint8_t) * jlen + 1);
    }
    else
    {
        pFrame->meta = calloc(1, sizeof(av_meta_t));
    }
    if(!pFrame->meta)
    {
        loge("malloc meta failed");
        goto err_output;
    }
    if (jpath != NULL)
    {
        sprintf(pFrame->meta->path, "%s", path);
        //logw("============dd===path=%s", pFrame->meta->path);
    }

    pFrame->size = len;
    pFrame->meta->width = jw;
    pFrame->meta->height = jh;
    pFrame->buf = buf;
    pFrame->codec_cb = (on_codec_cb)on_frame_codec;
    pFrame->error_cb = (on_error_cb)on_error;

    int ret = decode_h264_frame_to_yuv(pFrame);
err_output:
   if(buf)
        (*env)->ReleaseByteArrayElements(env, jdata, buf, 0);

    if (path)
        (*env)->ReleaseStringUTFChars(env, jpath, path);
    return (ret >= 0) ? JNI_TRUE : JNI_FALSE;
}
/**********************************************************************
从视频文件中提取一帧数据，并解码一帧数据为JPG
*****************************************/
static jboolean jni_convert_video(JNIEnv *env, jobject thiz, jstring jpath)
{
    if (jpath == NULL)
        return JNI_FALSE;

    int ret = -1;
    context_t *pFrame = &gFrame;
    unsigned char *path = NULL;
    jint jlen = 0;
    memset(pFrame, 0, sizeof(context_t));

    jlen = (jint)(*env)->GetStringLength(env,jpath);
    path = (unsigned char *) (*env)->GetStringUTFChars(env, jpath, NULL);
    //logw("===============path=%s, len=%d", path, jlen);
    pFrame->meta = calloc(1, sizeof(av_meta_t) + sizeof(uint8_t) * jlen + 1);

    if(!pFrame->meta)
    {
        loge("malloc meta failed");
        goto err_output;
    }
    if (path)
    {
        sprintf(pFrame->meta->path, "%s", path);
        //logw("============dd===path=%s", pFrame->meta->path);
    }

    pFrame->codec_cb = (on_codec_cb)on_frame_codec;
    pFrame->error_cb = (on_error_cb)on_error;

    ret = decode_video(pFrame, path);
err_output:
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    return (ret >= 0) ? JNI_TRUE : JNI_FALSE;
}
static jint jni_get_duration(JNIEnv *env, jobject thiz)
{
    //logi ("%s", __func__);
    context_t *ctx = &gFrame;
    return (jint)ctx->meta->duration;
}
static jboolean jni_release(JNIEnv *env, jobject thiz)
{
    context_t *frm = &gFrame;
    memset(frm, 0, sizeof(context_t));
    if (frm->buf)
        free(frm->buf);
    if (frm->meta)
        free(frm->meta);
    return JNI_TRUE;
}
static JNINativeMethod g_methods[] =
{
    {"nativeInit",         "()Z",                     (void *) jni_init},
    {"nativeConvert", "([BIIILjava/lang/String;)Z",   (void *) jni_convert},
    {"nativeConvertVideo", "(Ljava/lang/String;)Z",   (void *) jni_convert_video},
    {"nativeGetDuration", "()I",                      (void *) jni_get_duration},
    {"nativeRelease",         "()Z",                     (void *) jni_release},
};
int register_frame_codec(JNIEnv* env)
{
    jclass klass = (*env)->FindClass(env, CLASS_PATH);
    if (klass == NULL) {
        return JNI_ERR;
    }
    (*env)->RegisterNatives(env, klass, g_methods, NELEM(g_methods));

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved)
{
}
