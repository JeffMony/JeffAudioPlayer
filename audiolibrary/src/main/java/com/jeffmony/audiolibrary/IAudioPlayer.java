package com.jeffmony.audiolibrary;

import com.jeffmony.audiolibrary.listener.OnErrorListener;
import com.jeffmony.audiolibrary.listener.OnPlayerStateListener;
import com.jeffmony.audiolibrary.listener.OnPreparedListener;

/**
 * @author jeffli
 * @Date   2021-09-09
 *
 * 音频播放器的接口
 */

public interface IAudioPlayer {

    void prepare(String url);

    /**
     * 开始播放音频
     */
    void start();

    /**
     * 暂停
     */
    void pause();

    /**
     * 继续播放
     */
    void resume();

    /**
     * 停止
     */
    void stop();

    /**
     * 彻底销毁
     */
    void destroy();

    /**
     * 拖动进度条
     * @param time 单位ms
     */
    void seek(long time);

    /**
     * 获取音频时长
     * @return 单位ms
     */
    long getDuration();

    /**
     * 获取播放的时间点
     * @return 单位ms
     */
    long getCurrentPosition();

    /**
     * 设置是否循环播放
     * @param loop
     */
    void setLoop(boolean loop);

    /**
     * 设置音量等级
     * @param volumeLevel 0 ~ 100范围内
     */
    void setVolumeLevel(int volumeLevel);

    /**
     * 设置播放速度
     * @param speed 1.0是正常播放速度
     */
    void setSpeed(float speed);

    void setOnPreparedListener(OnPreparedListener listener);

    void setOnErrorListener(OnErrorListener listener);

    void setOnPlayerStateListener(OnPlayerStateListener listener);
}
