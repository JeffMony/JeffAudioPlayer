package com.jeffmony.audioplayer;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import com.jeffmony.audiolibrary.AudioPlayer;
import com.jeffmony.audiolibrary.IAudioPlayer;
import com.jeffmony.audiolibrary.utils.LogUtils;
import com.jeffmony.audiolibrary.utils.TimeUtils;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    private static final String TAG = "MainActivity";

    private static final int MSG_UPDATE_TIME = 1;
    private static final int MSG_UPDATE_TIME_VIEW = 2;
    private static final int MSG_UPDATE_VOLUME_VIEW = 3;
    private static final int MSG_RESET_TIME_VIEW = 4;

    private EditText mUrlTxt;
    private SeekBar mAudioSeekBar;
    private TextView mTimeTxt;
    private Button mStartBtn;
    private Button mPauseBtn;
    private Button mResumeBtn;
    private Button mStopBtn;
    private SeekBar mVolumeSeekbar;
    private TextView mVolumeText;
    private CheckBox mLoopBox;
    private Button mSpeed1Btn;
    private Button mSpeed2Btn;
    private Button mSpeed3Btn;
    private Button mSpeed4Btn;
    private Button mSpeed5Btn;

    private IAudioPlayer mPlayer;

    private long mTotalDuration = 0;
    private long mCurrentPosition = 0;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(@NonNull Message msg) {
            super.handleMessage(msg);
            int what = msg.what;
            if (what == MSG_UPDATE_TIME) {
                updateTimeInfoInternal();
            } else if (what == MSG_UPDATE_TIME_VIEW) {
                updateTimeViewInternal();
            } else if (what == MSG_UPDATE_VOLUME_VIEW) {
                updateVolumeViewInternal(msg.arg1);
            } else if (what == MSG_RESET_TIME_VIEW) {
                resetTimeViewInternal();
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initViews();

        mPlayer = new AudioPlayer();
    }

    private void initViews() {
        mUrlTxt = findViewById(R.id.url_txt);
        mAudioSeekBar = findViewById(R.id.audio_seekbar);
        mTimeTxt = findViewById(R.id.time_txt);
        mStartBtn = findViewById(R.id.play_btn);
        mPauseBtn = findViewById(R.id.pause_btn);
        mResumeBtn = findViewById(R.id.resume_btn);
        mStopBtn = findViewById(R.id.stop_btn);
        mVolumeSeekbar = findViewById(R.id.volume_seekbar);
        mVolumeText = findViewById(R.id.volume_txt);
        mLoopBox = findViewById(R.id.loop_box);
        mSpeed1Btn = findViewById(R.id.speed_1_btn);
        mSpeed2Btn = findViewById(R.id.speed_2_btn);
        mSpeed3Btn = findViewById(R.id.speed_3_btn);
        mSpeed4Btn = findViewById(R.id.speed_4_btn);
        mSpeed5Btn = findViewById(R.id.speed_5_btn);

        mStartBtn.setOnClickListener(this);
        mPauseBtn.setOnClickListener(this);
        mResumeBtn.setOnClickListener(this);
        mStopBtn.setOnClickListener(this);
        mSpeed1Btn.setOnClickListener(this);
        mSpeed2Btn.setOnClickListener(this);
        mSpeed3Btn.setOnClickListener(this);
        mSpeed4Btn.setOnClickListener(this);
        mSpeed5Btn.setOnClickListener(this);

        /// 拖动进度条的监听
        mAudioSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                LogUtils.i(TAG, "onStartTrackingTouch");
                mHandler.removeMessages(MSG_UPDATE_TIME);
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                LogUtils.i(TAG, "onStopTrackingTouch progress="+seekBar.getProgress());
                if (mTotalDuration != 0) {
                    long time = (long)(seekBar.getProgress() * 1.0f / 1000 * mTotalDuration);
                    seek(time);
                }
                mHandler.sendEmptyMessageDelayed(MSG_UPDATE_TIME, 500);
            }
        });

        /// 音量调整的seekbar
        mVolumeSeekbar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                Message message = Message.obtain();
                message.what = MSG_UPDATE_VOLUME_VIEW;
                message.arg1 = seekBar.getProgress();
                mHandler.sendMessage(message);
            }
        });

        mLoopBox.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (mPlayer != null) {
                mPlayer.setLoop(isChecked);
            }
        });
    }

    private void play() {
        String url = mUrlTxt.getText().toString();
        if (TextUtils.isEmpty(url)) {
            Toast.makeText(MainActivity.this, "请输入播放url", Toast.LENGTH_SHORT).show();
            return;
        }

        mPlayer.prepare(url);
        mPlayer.setLoop(mLoopBox.isChecked());
        mPlayer.setOnPreparedListener(() -> {
            mPlayer.start();
            updateTimeInfo();
        });

        mPlayer.setOnErrorListener((code, msg) -> {
            LogUtils.e(TAG, "code="+code+", msg="+msg);
        });

        mPlayer.setOnPlayerStateListener(() -> {
            mHandler.removeMessages(MSG_UPDATE_TIME);
            mHandler.sendEmptyMessage(MSG_UPDATE_TIME_VIEW);
        });
    }

    private void pause() {
        if (mPlayer != null) {
            mPlayer.pause();
            mHandler.removeMessages(MSG_UPDATE_TIME);
        }
    }

    private void resume() {
        if (mPlayer != null) {
            mPlayer.resume();
            updateTimeInfo();
        }
    }

    private void stop() {
        if (mPlayer != null) {
            mPlayer.stop();
            mHandler.removeMessages(MSG_UPDATE_TIME);
            mHandler.sendEmptyMessage(MSG_RESET_TIME_VIEW);
        }
    }

    private void seek(long time) {
        if (mPlayer != null) {
            mPlayer.seek(time);
        }
    }

    private void updateTimeInfo() {
        mHandler.sendEmptyMessage(MSG_UPDATE_TIME);
    }

    private void updateTimeInfoInternal() {
        mTotalDuration = mPlayer.getDuration();
        mCurrentPosition = mPlayer.getCurrentPosition();
        if (mTotalDuration != 0) {
            float progress = 1.0f * mCurrentPosition / mTotalDuration;
            mAudioSeekBar.setProgress((int) (progress * 1000));
        }
        String timeStr = TimeUtils.getVideoTimeString(mCurrentPosition) + " : " +
                TimeUtils.getVideoTimeString(mTotalDuration);
        mTimeTxt.setText(timeStr);
        mHandler.sendEmptyMessageDelayed(MSG_UPDATE_TIME, 300);
    }

    private void updateTimeViewInternal() {
        mAudioSeekBar.setProgress(1000);
        String timeStr = TimeUtils.getVideoTimeString(mTotalDuration) + " : " +
                TimeUtils.getVideoTimeString(mTotalDuration);
        mTimeTxt.setText(timeStr);
    }

    private void updateVolumeViewInternal(int volumeLevel) {
        if (mPlayer != null) {
            mPlayer.setVolumeLevel(volumeLevel);
        }
        mVolumeText.setText("音量 : " + volumeLevel);
    }

    private void resetTimeViewInternal() {
        mAudioSeekBar.setProgress(0);
        mTotalDuration = mPlayer.getDuration();
        mCurrentPosition = mPlayer.getCurrentPosition();
        String timeStr = TimeUtils.getVideoTimeString(mCurrentPosition) + " : " +
                TimeUtils.getVideoTimeString(mTotalDuration);
        mTimeTxt.setText(timeStr);

    }

    @SuppressLint("ResourceAsColor")
    private void changeSpeed(View view, float speed) {
        mSpeed1Btn.setBackgroundColor(R.color.mtrl_btn_bg_color_selector);
        mSpeed2Btn.setBackgroundColor(R.color.mtrl_btn_bg_color_selector);
        mSpeed3Btn.setBackgroundColor(R.color.mtrl_btn_bg_color_selector);
        mSpeed4Btn.setBackgroundColor(R.color.mtrl_btn_bg_color_selector);
        mSpeed5Btn.setBackgroundColor(R.color.mtrl_btn_bg_color_selector);

        if (mPlayer != null) {
            mPlayer.setSpeed(speed);
        }
        view.setBackgroundColor(R.color.design_bottom_navigation_shadow_color);
    }

    @Override
    public void onClick(View v) {
        if (v == mStartBtn) {
            play();
        } else if (v == mPauseBtn) {
            pause();
        } else if (v == mResumeBtn) {
            resume();
        } else if (v == mStopBtn) {
            stop();
        } else if (v == mSpeed1Btn) {
            changeSpeed(v, 0.5f);
        } else if (v == mSpeed2Btn) {
            changeSpeed(v, 0.75f);
        } else if (v == mSpeed3Btn) {
            changeSpeed(v, 1.0f);
        } else if (v == mSpeed4Btn) {
            changeSpeed(v, 1.5f);
        } else if (v == mSpeed5Btn) {
            changeSpeed(v, 2.0f);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mPlayer != null) {
            mPlayer.stop();
            mPlayer.destroy();
            mHandler.removeMessages(MSG_UPDATE_TIME);
        }
    }
}