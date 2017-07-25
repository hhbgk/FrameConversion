/**
 * Description:
 * Author:created by hhbgk on 17-5-30.
 */
//

#include "frame_codec.h"
#include <memory.h>
#include <stdint.h>

#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"

#include "media_meta.h"
//#define DB_SAVE_JPG

static int convert_yuv_to_jpg(context_t *app_ctx, AVFrame* pFrame){
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
	int width = app_ctx->meta->width;
	int height = app_ctx->meta->height;
	//loge("width=%d, h=%d", width, height);
#ifdef DB_SAVE_JPG
	const char *out_file = "/mnt/sdcard/iframe.jpg";
#endif

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

#ifdef DB_SAVE_JPG
logw("111");
    //Output URL
    if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0){
        loge("Couldn't open output file.");
        return -1;
    }
#endif
logw("222");
	video_st = avformat_new_stream(pFormatCtx, 0);
	if (video_st==NULL){
		loge("avformat_new_stream fail");
		return -1;
	}
logw("333");

	pCodecCtx = video_st->codec;
	pCodecCtx->codec_id = fmt->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
	pCodecCtx->width = width;
	pCodecCtx->height = height;

	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 30;
logw("444");
	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);

	if (!pCodec){
		loge("Codec not found.");
		return -1;
	}
	logw("555");
	if (avcodec_open2(pCodecCtx, pCodec,NULL) < 0){
		loge("Could not open codec.");
		return -1;
	}
#ifdef DB_SAVE_JPG
	//Write Header
	avformat_write_header(pFormatCtx, NULL);
#endif
logw("666");
	y_size = pCodecCtx->width * pCodecCtx->height;

	av_new_packet(&pkt, y_size * 3);
logw("777 pkt =%p, pFrame=%p", pkt, pFrame);
	//Encode
	ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
	logw("8888");
	if(ret < 0){
		loge("Encode Error.");
		return -1;
	}
	logw("9999");
	if (got_picture){
		//logi("pkt data size=%d", pkt.size);
		pkt.stream_index = video_st->index;
		//logi("00 pkt data size=%d", pkt.size);
#ifdef DB_SAVE_JPG
		ret = av_write_frame(pFormatCtx, &pkt);
#endif
		//logi("11 pkt data size=%d", pkt.size);
		app_ctx->buf = pkt.data;
		app_ctx->size = pkt.size;
		if(app_ctx->codec_cb) {
		    app_ctx->codec_cb(app_ctx);
		} else {
		    logw("Callback is not initialized.");
		}
	}
	av_free_packet(&pkt);
#ifdef DB_SAVE_JPG
	//Write Trailer
	av_write_trailer(pFormatCtx);
	avio_close(pFormatCtx->pb);
#endif

	loge("Encode Successful.");

	if (video_st){
		avcodec_close(video_st->codec);
	}
	avformat_free_context(pFormatCtx);
loge("Encode Successful. 1");
	return 0;
}

int decode_h264_frame_to_yuv(context_t *app_ctx){
    logi("%s", __func__);
    AVCodec *codec = NULL;
    AVCodecContext *ctx= NULL;
    AVFrame *fr = NULL;
    uint8_t *byte_buffer = NULL;
    AVPacket pkt;
    //AVFormatContext *fmt_ctx = NULL;
    int got_frame = 0;
    int byte_buffer_size;
    int result;
logi("1111111111");
    av_register_all();

    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        loge("Can't find decoder");
        return -1;
    }
logi("222");
    ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        loge("Can't allocate decoder context");
        return AVERROR(ENOMEM);
    }
 logi("333");
	ctx->time_base.num = 1;
	ctx->frame_number = 1; //每包一个视频帧
	ctx->codec_type = AVMEDIA_TYPE_VIDEO;
	ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
	ctx->bit_rate = 0;
	ctx->time_base.den = 30;//帧率
	ctx->width = app_ctx->meta->width;//视频宽
	ctx->height = app_ctx->meta->height;//视频高
	logi("444 a");
    result = avcodec_open2(ctx, codec, NULL);
    logi("444 b");
    if (result < 0) {
        loge("Can't open decoder");
        return result;
    }
logi("444 c");
    fr = av_frame_alloc();
    logi("444 d");
    if (!fr) {
        loge("Can't allocate frame");
        return AVERROR(ENOMEM);
    }
logi("666");
    byte_buffer_size = av_image_get_buffer_size(ctx->pix_fmt, ctx->width, ctx->height, 16);
    if(byte_buffer_size<=0)
    {
	    loge("Can't get buffer size");
    	return AVERROR(ENOMEM);
    }
    logi("777");
    byte_buffer = av_malloc(byte_buffer_size);
    if (!byte_buffer) {
        loge("Can't allocate buffer");
        return AVERROR(ENOMEM);
    }

    av_init_packet(&pkt);
    logi("app_ctx->buf=%p", app_ctx->buf);
    pkt.data = app_ctx->buf;//这里填入一个指向完整H264数据帧的buffer
logi("pkt.data=%p", pkt.data);
	pkt.size = app_ctx->size;//这个填入H264数据帧的大小
	logi("pkt.size=%d",pkt.size );
	result = avcodec_decode_video2(ctx, fr, &got_frame, &pkt);
logi("result=%d", result);
	if (result < 0) {
        loge("Error decoding frame");
        return result;
    }
logi("got_frame =%d", got_frame);
    if (got_frame) {

    	convert_yuv_to_jpg(app_ctx, fr);
    }

    av_packet_unref(&pkt);
    av_frame_free(&fr);
    avcodec_close(ctx);
    //avformat_close_input(&fmt_ctx);
    avcodec_free_context(&ctx);
    av_freep(&byte_buffer);
    logi("end====");
    return 0;
}
/****************************************************************************
 *
 * @param app_ctx 上下文
 * @param input_filename 输入路径
 * @return 0为成功，否则失败
 */
int decode_video(context_t *app_ctx, const char *input_filename) {
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
    int duration = 0;

    av_register_all();

    //logi("%s:input file=%s", __func__, input_filename);
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

    if (fmt_ctx) {
		if (fmt_ctx->duration != AV_NOPTS_VALUE) {
			duration = ((fmt_ctx->duration / AV_TIME_BASE) * 1000);
		}
	}
	if (app_ctx->meta)
	    app_ctx->meta->duration = duration;
    //loge("#tb %d: %d/%d, duration=%d", video_stream, fmt_ctx->streams[video_stream]->time_base.num,
    //fmt_ctx->streams[video_stream]->time_base.den, duration);
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
                if (app_ctx->meta) {
                    app_ctx->meta->width = ctx->width;
                    app_ctx->meta->height = ctx->height;
                }

            	//logi("w=%d, h=%d,app_ctx->width=%d, app_ctx->height=%d", ctx->width, ctx->height, app_ctx->width, app_ctx->height);
            	result = convert_yuv_to_jpg(app_ctx, fr);
            	//result = convert_yuv_to_jpg1(fr, app_ctx->width, app_ctx->height, "/mnt/sdcard/my.jpg");
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
