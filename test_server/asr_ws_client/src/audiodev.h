#pragma once

#include <portaudio.h>
#include <functional>
#include <stdexcept>
#include <vector>
#include <atomic>
#include <vector>
#include <iostream>
#include <thread>

class AudioDev
{
public:
    using PCMCallback = std::function<void(const std::vector<int16_t> &pcm, size_t samples)>;
    AudioDev(int sample_rate = 16000, int frame_per_buf = 320)
        : sample_rate_(sample_rate)
        , frame_per_buf_(frame_per_buf)
    {
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            throw std::runtime_error("PortAudio initialize failed");
        }
    }

    ~AudioDev()
    {
        stop();
        PaError err = Pa_Terminate();
        if (err != paNoError) {
            std::cerr << "PortAudio terminate failed: " << Pa_GetErrorText(err) << "\n";
        }
    }

    bool start(PCMCallback callback)
    {
        if (running_) {
            return true;
        }
        callback_ = callback;

        PaStreamParameters input;
        input.device = Pa_GetDefaultInputDevice();
        if (input.device == paNoDevice) {
            std::cerr << "No input device.\n";
            return false;
        }

        input.channelCount = 1;
        input.sampleFormat = paInt16;
        input.suggestedLatency = Pa_GetDeviceInfo(input.device)->defaultLowInputLatency;
        input.hostApiSpecificStreamInfo = nullptr;

        PaError err = Pa_OpenStream(&stream_,
                                    &input,
                                    NULL,
                                    sample_rate_,
                                    frame_per_buf_,
                                    paClipOff,
                                    NULL,
                                    NULL);

        if (err != paNoError) {
            std::cerr << "Pa_OpenStream error: " << Pa_GetErrorText(err) << "\n";
            return false;
        }

        err = Pa_StartStream(stream_);
        if (err != paNoError) {
            std::cerr << "Pa_StartStream error: " << Pa_GetErrorText(err) << "\n";
            return false;
        }

        running_ = true;

        // 创建录音线程
        std::thread([this]() {
            std::vector<int16_t> buffer(frame_per_buf_);

            while (running_) {
                PaError r = Pa_ReadStream(stream_, buffer.data(), frame_per_buf_);
                if (r && r != paInputOverflowed) {
                    std::cerr << "Pa_ReadStream error: " << Pa_GetErrorText(r) << "\n";
                    break;
                }
                if (callback_) {
                    callback_(buffer, buffer.size());
                }
            }
        }).detach();

        return true;
    }

    void stop()
    {
        if (!running_) {
            return;
        }
        running_ = false;

        if (stream_) {
            Pa_StopStream(stream_);
            Pa_CloseStream(stream_);
            stream_ = nullptr;
        }
    }

    bool isRunning() const
    {
        return running_;
    }

private:
    int sample_rate_;
    int frame_per_buf_;
    std::atomic<bool> running_ {false};
    PCMCallback callback_;
    PaStream *stream_;
};