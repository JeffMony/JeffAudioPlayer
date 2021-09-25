package com.jeffmony.audiolibrary;

import com.jeffmony.audiolibrary.listener.OnErrorListener;
import com.jeffmony.audiolibrary.listener.OnPlayerStateListener;
import com.jeffmony.audiolibrary.listener.OnPreparedListener;

/**
 * @author jeffli
 * @Date   2021-09-06
 */
public class AudioPlayer implements IAudioPlayer {

    private static boolean sLibraryLoaded = false;

    private long mId;

    private OnPreparedListener mOnPreparedListener;
    private OnErrorListener mOnErrorListener;
    private OnPlayerStateListener mOnPlayerStateListener;

    public AudioPlayer() {
        init();
        mId = create();
    }

    private static void init() {
        if (!sLibraryLoaded) {
            /// 最终生成的库是 libltpaudio.so
            System.loadLibrary("ltpaudio");
            sLibraryLoaded = true;
        }
    }

    @Override
    public void prepare(String url) {
        if (mId <= 0) {
            return;
        }
        prepare(mId, url);
    }

    @Override
    public void start() {
        if (mId <= 0) {
            return;
        }
        start(mId);
    }

    @Override
    public void pause() {
        if (mId <= 0) {
            return;
        }
        pause(mId);
    }

    @Override
    public void resume() {
        if (mId <= 0) {
            return;
        }
        resume(mId);
    }

    @Override
    public void stop() {
        if (mId <= 0) {
            return;
        }
        stop(mId);
    }

    @Override
    public void destroy() {
        if (mId <= 0) {
            return;
        }
        destroy(mId);
        mId = 0;
    }

    @Override
    public void seek(long time) {
        if (mId <= 0) {
            return;
        }
        seek(mId, time);
    }

    @Override
    public long getDuration() {
        if (mId <= 0) {
            return 0;
        }
        return getDuration(mId);
    }

    @Override
    public long getCurrentPosition() {
        if (mId <= 0) {
            return 0;
        }
        return getCurrentPosition(mId);
    }

    @Override
    public void setLoop(boolean loop) {
        if (mId <= 0) {
            return;
        }
        setLoop(mId, loop);
    }

    @Override
    public void setVolumeLevel(int volumeLevel) {
        if (mId <= 0) {
            return;
        }
        setVolumeLevel(mId, volumeLevel);
    }

    @Override
    public void setSpeed(float speed) {
        if (mId <= 0) {
            return;
        }
        setSpeed(mId, speed);
    }

    @Override
    public void setOnPreparedListener(OnPreparedListener listener) {
        mOnPreparedListener = listener;
    }

    @Override
    public void setOnErrorListener(OnErrorListener listener) {
        mOnErrorListener = listener;
    }

    @Override
    public void setOnPlayerStateListener(OnPlayerStateListener listener) {
        mOnPlayerStateListener = listener;
    }

    /**
     * prepare已经完成
     */
    private void onPrepared() {
        if (mOnPreparedListener != null) {
            mOnPreparedListener.onPrepared();
        }
    }

    /**
     * 播放出现错误
     */
    private void onError(int errCode, String errMsg) {
        if (mOnErrorListener != null) {
            mOnErrorListener.onError(errCode, errMsg);
        }
    }

    /**
     * 播放到结尾
     */
    private void onCompleted() {
        if (mOnPlayerStateListener != null) {
            mOnPlayerStateListener.onCompleted();
        }
    }

    /// native 方法，见audio_jni.cc文件
    private native long create();
    private native void prepare(long id, String url);
    private native void start(long id);
    private native void pause(long id);
    private native void resume(long id);
    private native void stop(long id);
    private native void destroy(long id);
    private native void seek(long id, long time);
    private native long getDuration(long id);
    private native long getCurrentPosition(long id);
    private native void setLoop(long id, boolean loop);
    private native void setVolumeLevel(long id, int volumeLevel);
    private native void setSpeed(long id, float speed);
}
