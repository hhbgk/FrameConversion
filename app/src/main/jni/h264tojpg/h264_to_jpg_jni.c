//
// Created by bob on 16-9-5.
//

#include "h264_to_jpg_jni.h"

#define JNI_CLASS_IJKPLAYER     "com/hhbgk/h264tojpg/api/H264ToJpg"
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))

static JavaVM *gJVM = NULL;
static jobject gObj = NULL;
static jmethodID on_data_prepared_method_id;

static void data_handler(uint8_t *data, int size){
    logd("%s", __func__);
    jboolean isAttached = JNI_FALSE;
    JNIEnv *env = NULL;
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

    jbyteArray jArray = (*env)->NewByteArray(env, size);
    (*env)->SetByteArrayRegion(env, jArray, 0, size, (jbyte*)data);
    (*env)->CallVoidMethod(env, gObj, on_data_prepared_method_id, jArray);
    (*env)->DeleteLocalRef(env, jArray);
    if (isAttached)
        (*gJVM)->DetachCurrentThread(gJVM);
    logi("%s: over....", __func__);
}

static int convert_yuv_to_jpg(AVFrame* pFrame, int width, int height, const char *out_file){
    logi("%s", __func__);
	AVFormatContext* pFormatCtx;
	AVOutputFormat* fmt;
	AVStream* video_st;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;
	AVPacket pkt;
	int y_size;
	int got_picture=0;
	int ret=0;

    //Method 1
    pFormatCtx = avformat_alloc_context();
	if(!pFormatCtx){
	    loge("pFormatCtx is null");
	    return -1;
	}
    //Guess format
    fmt = av_guess_format("mjpeg", NULL, NULL);
    if(!fmt){
        loge("fmt is null");
        return -1;
    }
    pFormatCtx->oformat = fmt;
    //Output URL
    if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0){
        printf("Couldn't open output file.");
        return -1;
    }

	video_st = avformat_new_stream(pFormatCtx, 0);
	if (video_st==NULL){
		loge("avformat_new_stream fail");
		return -1;
	}

	pCodecCtx = video_st->codec;
	pCodecCtx->codec_id = fmt->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
	pCodecCtx->width = width;
	pCodecCtx->height = height;

	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;
	//Output some information
	av_dump_format(pFormatCtx, 0, out_file, 1);

	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec){
		loge("Codec not found.");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec,NULL) < 0){
		loge("Could not open codec.");
		return -1;
	}

	//Write Header
	avformat_write_header(pFormatCtx, NULL);

	y_size = pCodecCtx->width * pCodecCtx->height;
	av_new_packet(&pkt, y_size * 3);

	//Encode
	ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
	if(ret < 0){
		loge("Encode Error.");
		return -1;
	}
	if (got_picture){
		loge("pkt data size=%d, ret=%d", pkt.size, ret);
		pkt.stream_index = video_st->index;
		ret = av_write_frame(pFormatCtx, &pkt);
		data_handler(pkt.data, pkt.size);
	}
	av_free_packet(&pkt);
	//Write Trailer
	av_write_trailer(pFormatCtx);

	loge("Encode Successful.");

	if (video_st){
		avcodec_close(video_st->codec);
	}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);

	return 0;
}

static int decode_video(const char *input_filename, const char *output_filename){
    AVCodec *codec = NULL;
    AVCodecContext *origin_ctx = NULL, *ctx= NULL;
    AVFrame *fr = NULL;
    uint8_t *byte_buffer = NULL;
    AVPacket pkt;
    AVFormatContext *fmt_ctx = NULL;
    int video_stream;
    int got_frame = 0;
    int byte_buffer_size;
    int i = 0;
    int result;
    int end_of_stream = 0;

    av_register_all();

    logi("%s:file=%s, out_file=%s", __func__, input_filename, output_filename);
    result = avformat_open_input(&fmt_ctx, input_filename, NULL, NULL);
    if (result < 0) {
        loge("Can't open file");
        return result;
    }

    result = avformat_find_stream_info(fmt_ctx, NULL);
    if (result < 0) {
        loge("Can't get stream info");
        return result;
    }

    video_stream = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_stream < 0) {
      loge("Can't find video stream in input file");
      return -1;
    }

    origin_ctx = fmt_ctx->streams[video_stream]->codec;

    codec = avcodec_find_decoder(origin_ctx->codec_id);
    if (!codec) {
        loge("Can't find decoder");
        return -1;
    }

    ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        loge("Can't allocate decoder context");
        return AVERROR(ENOMEM);
    }

    result = avcodec_copy_context(ctx, origin_ctx);
    if (result) {
        loge("Can't copy decoder context");
        return result;
    }

    result = avcodec_open2(ctx, codec, NULL);
    if (result < 0) {
        loge("Can't open decoder");
        return result;
    }

    fr = av_frame_alloc();
    if (!fr) {
        loge("Can't allocate frame");
        return AVERROR(ENOMEM);
    }

    byte_buffer_size = av_image_get_buffer_size(ctx->pix_fmt, ctx->width, ctx->height, 16);
    byte_buffer = av_malloc(byte_buffer_size);
    if (!byte_buffer) {
        loge("Can't allocate buffer");
        return AVERROR(ENOMEM);
    }

    //loge("#tb %d: %d/%d\n", video_stream, fmt_ctx->streams[video_stream]->time_base.num, fmt_ctx->streams[video_stream]->time_base.den);
    i = 0;
    av_init_packet(&pkt);
    do {
        if (!end_of_stream)
            if (av_read_frame(fmt_ctx, &pkt) < 0)
                end_of_stream = 1;
        if (end_of_stream) {
            pkt.data = NULL;
            pkt.size = 0;
        }
        if (pkt.stream_index == video_stream || end_of_stream) {
            got_frame = 0;
            if (pkt.pts == AV_NOPTS_VALUE)
                pkt.pts = pkt.dts = i;
            result = avcodec_decode_video2(ctx, fr, &got_frame, &pkt);
            if (result < 0) {
                loge("Error decoding frame");
                break;
            }
            if (got_frame) {
            	result = convert_yuv_to_jpg(fr, ctx->width, ctx->height, output_filename);
            	break;
            }
            av_packet_unref(&pkt);
            av_init_packet(&pkt);
        }
        i++;
    } while (!end_of_stream);

    av_packet_unref(&pkt);
    av_frame_free(&fr);
    avcodec_close(ctx);
    avformat_close_input(&fmt_ctx);
    avcodec_free_context(&ctx);
    av_freep(&byte_buffer);
    if (result >=0)
        return 0;
    else
        return result;
}

static jboolean jni_request_h264_jpg(JNIEnv *env, jobject thiz, jstring jfile, jstring joutput_file){
    const char *file_name = (*env)->GetStringUTFChars(env, jfile, NULL);
    const char *output_file = (*env)->GetStringUTFChars(env, joutput_file, NULL);
    int ret = decode_video(file_name, output_file);
    (*env)->ReleaseStringUTFChars(env, jfile, file_name);
    (*env)->ReleaseStringUTFChars(env, joutput_file, output_file);
    return ret == 0 ? JNI_TRUE : JNI_FALSE;
}

static void jni_native_init(JNIEnv *env, jobject thiz){
    logd("%s", __func__);
    //保存全局JVM以便在子线程中使用
    (*env)->GetJavaVM(env,&gJVM);
    //不能直接赋值(g_obj = thiz)
    gObj = (*env)->NewGlobalRef(env, thiz);

    jclass clazz = (*env)->GetObjectClass(env, thiz);
    if(clazz == NULL){
    	(*env)->ThrowNew(env, "java/lang/NullPointerException", "Unable to find exception class");
    }
    on_data_prepared_method_id = (*env)->GetMethodID(env, clazz, "onDataPrepared", "([B)V");
    if(!on_data_prepared_method_id){
    	loge("The calling class does not implement all necessary interface methods");
    }

}

static JNINativeMethod g_methods[] = {
    { "nativeInit",         "()V",                                               (void *) jni_native_init },
    { "_request_jpg",         "(Ljava/lang/String;Ljava/lang/String;)Z",         (void *) jni_request_h264_jpg },
//    { "_destroy",         "()Z",                                               (void *) jni_tftp_destroy},
 };

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv* env = NULL;

    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    assert(env != NULL);

    // FindClass returns LocalReference
    jclass klass = (*env)->FindClass (env, JNI_CLASS_IJKPLAYER);
    if (klass == NULL) {
      //LOGE ("Native registration unable to find class '%s'", JNI_CLASS_IJKPLAYER);
      return JNI_ERR;
    }
    (*env)->RegisterNatives(env, klass, g_methods, NELEM(g_methods) );

    return JNI_VERSION_1_4;
}
