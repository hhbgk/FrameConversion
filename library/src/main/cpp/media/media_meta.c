/**
 * Description:
 * Author:created by hhbgk on 17-6-7.
 */
//

#include "media_meta.h"

void set_duration(AVFormatContext *ic) {
	char value[30] = "0";
	int duration = 0;

	if (ic) {
		if (ic->duration != AV_NOPTS_VALUE) {
			duration = ((ic->duration / AV_TIME_BASE) * 1000);
		}
	}

	sprintf(value, "%d", duration);
	av_dict_set(&ic->metadata, DURATION, value, 0);
}

void set_frame_rate(AVFormatContext *ic, AVStream *video_st) {
	char value[30] = "0";

	if (video_st && video_st->avg_frame_rate.den && video_st->avg_frame_rate.num) {
		double d = av_q2d(video_st->avg_frame_rate);
		uint64_t v = lrintf(d * 100);
		if (v % 100) {
			sprintf(value, "%3.2f", d);
		} else if (v % (100 * 1000)) {
			sprintf(value,  "%1.0f", d);
		} else {
			sprintf(value, "%1.0fk", d / 1000);
		}

	    av_dict_set(&ic->metadata, FRAME_RATE, value, 0);
	}
}

const char* get_media_meta(AVFormatContext *ic, AVStream *audio_st, AVStream *video_st, const char* key) {
    char* value = NULL;

	if (!ic) {
		return value;
	}

	if (key) {
		if (av_dict_get(ic->metadata, key, NULL, AV_DICT_MATCH_CASE)) {
			value = av_dict_get(ic->metadata, key, NULL, AV_DICT_MATCH_CASE)->value;
		} else if (audio_st && av_dict_get(audio_st->metadata, key, NULL, AV_DICT_MATCH_CASE)) {
			value = av_dict_get(audio_st->metadata, key, NULL, AV_DICT_MATCH_CASE)->value;
		} else if (video_st && av_dict_get(video_st->metadata, key, NULL, AV_DICT_MATCH_CASE)) {
			value = av_dict_get(video_st->metadata, key, NULL, AV_DICT_MATCH_CASE)->value;
		}
	}

	return value;
}
