package com.hhbgk.h264tojpg.api;

import android.util.Log;

/**
 * Author: bob
 * Date: 16-9-5 14:25
 * Version: V1
 * Description:
 */
public class H264ToJpg {
    private final String tag = getClass().getSimpleName();
    static {
        System.loadLibrary("ijkffmpeg");
        System.loadLibrary("jpg");
    }
    private native void nativeInit();
    private native boolean _request_jpg(String h264, String jpg);
    public H264ToJpg(){
        nativeInit();
    }

    public boolean requestJpg(String h264, String jpg){
        return _request_jpg(h264, jpg);
    }

    private OnDataPreparedListener mOnDataPreparedListener;
    public interface OnDataPreparedListener{
        void onPrepared(byte[] data);
    }

    public void setOnDataPreparedListener(OnDataPreparedListener listener){
        mOnDataPreparedListener = listener;
    }
    private void onDataPrepared(byte[] data){
        Log.i(tag, "data size=" + data.length);
        if (mOnDataPreparedListener != null){
            mOnDataPreparedListener.onPrepared(data);
        }
    }
}
