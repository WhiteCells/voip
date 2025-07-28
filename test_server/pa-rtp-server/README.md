### PA RTP Server

构建 portaudio 和 jrtplib 的双向通道

portaudio 职责：
    将接收音频进行播放，
    将麦克风的音频传输到 jrtplib

jrtplib 职责：
    负责推送和接收音频流

portaudio <---> que <---> rtp
    |            |         |
    |            |         |
    |            |         |
    |            |         |
    |            |         |
    |            |         |
    |            |         |
    |            |         |