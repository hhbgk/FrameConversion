/**
 * Description:
 * Author:created by hhbgk on 17-5-30.
 */
//

#ifndef IJKPLAYER_JL_CONVERTER_JNI_H
#define IJKPLAYER_JL_CONVERTER_JNI_H
#include <android/log.h>
#include <jni.h>
#include <stdint.h>
#define TAG "MediaCodec"
#define logi(...)  ((void)__android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__))
#define logw(...)  ((void)__android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__))
#define loge(...)  ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))
typedef void (*on_codec_cb)(void *, ...);
typedef void (*on_error_cb)(const char *, ...);
typedef struct {
    int duration;
    int width;
    int height;
    uint8_t path[0];
}av_meta_t;
typedef struct {
    /*int width;
    int height;*/
    int size;
    on_codec_cb codec_cb;
    on_error_cb error_cb;
    av_meta_t *meta;
    uint8_t* buf;
}context_t;
int register_frame_codec(JNIEnv* env);
#endif //IJKPLAYER_JL_CONVERTER_JNI_H
