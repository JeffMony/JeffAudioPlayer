## 音频播放器
- 1.正常播放功能
- 2.拖动进度条
- 3.倍速
- 4.控制音量
- 5.控制音调
- 6.循环播放

### 1.编译库
### 2.OpenSL ES功能介绍
### 3.拖动进度条
### 4.倍速
### 5.控制音量
### 6.控制音调

### 疑难问题
- 读取文件结束，最后还有几十帧音频数据无法取出来，怎么办？<br>
保证解码线程轮转，直到队列为空为止

- 拖动进度条之后，codec_context_还有AVPacket未解码怎么办？<br>
调用 avcodec_send_packet(codec_context_, nullptr); 然后自驱avcodec_receive_frame自动取出AVFrame会清空缓存中的AVPacket数据

- sonic使用的时候有什么注意点？<br>
sonicWriteShortToStream比较容易用错

- 控制声音没使用OpenSL ES原生的接口，使用的是sonic.<br>
使用sonic不仅可以减小音量，也可以增大音量，可以做到比100还大的音量

- sonic和soundtouch有什么优劣? <br>
简而言之，sonic对正常的说话处理是优于soundtouch的，但是soundtouch对音乐处理效果要更好，<br>
看你应用的是什么场景，根据不同的场景选择不同的库。

- sonic的倍速方案? <br>
变速不变调是倍速方法中的核心，sonic可以改变pitch/speed/volume等参数，<br>
满足你变速不变调，不变速变调，调整声音等不同的需求。

- short 和 uint8_t 的区别? <br>
因为ffmpeg中音频数据用uint8_t*表示，正常环境下用short*表示，两种变量的转化要注意。<br>
sizeof(short) = 2, sizeof(uint8_t) = 1


