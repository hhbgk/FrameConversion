/**
 * Description:
 * Author:created by hhbgk on 17-5-27.
 */

#include <jni.h>
#include <stddef.h>
#include <assert.h>
#include "jl_frm_codec_jni.h"
#include "jl_rts_muxer_jni.h"

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env = NULL;
    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK)
    {
        //loge("GetEnv failed!");
        return JNI_ERR;
    }

    //logi("Retrieve the env success!");

    register_frame_codec(env);
    register_rts_muxer(env);
    return JNI_VERSION_1_6;
}
