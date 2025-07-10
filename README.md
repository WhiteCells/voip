### voip

#### Dependenice

- C++17
- CMake >= 3.10
- pjsip pjsua2
- boost
  - asio
  - beast
- jsoncpp
- spdlog

```json
git submodule update --init --recursive
```

#### Build

```sh
cmake -B build
cmake --build build -j$(nproc)
./build/voip/voip
```

#### Docker

```sh
docker build -t voip -f dockerfile .
docker run -it voip bash
```


### PCM to WAV

```sh
ffmpeg -f s16le -ar 8000 -ac 1 -i output.pcm output.wav
```

### sdp

```sdp
v=0
o=FreeSWITCH 1751952676 1751952677 IN IP4 192.168.10.51
s=FreeSWITCH
c=IN IP4 192.168.10.51
t=0 0
m=audio 10962 RTP/AVP 0 121
a=rtpmap:0 PCMU/8000
a=rtpmap:121 telephone-event/8000
a=fmtp:121 0-15
a=silenceSupp:off - - - -
a=ptime:20
a=rtcp:10963 IN IP4 192.168.10.51
m=text 10586 RTP/AVP 100 98
a=rtpmap:100 red/1000
a=fmtp:100 98/98/98
a=rtpmap:98 t140/1000
```

### RTP Server

```
pjsua2 client    rtp server
  required  <---  生成模拟音频帧推送，或者通过 portaudio 获取麦克风音频，然后推送
  received  --->  接收音频帧，并通过 portaudio 进行播放
```
