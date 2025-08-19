#include "rtp_server.h"
#include "audio_dev.h"
#include "audio_queue.h"
#include <thread>
#include <csignal>
#include <cstring>

std::atomic<bool> running {true};

AudioQueue input_que;
AudioQueue output_que;

static void signal_handler(int)
{
    running = false;
}

// 推送给队列，在 Rtp Server 中发送
static int inputCallback(const void *inputBuffer, void *,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *,
                         PaStreamCallbackFlags,
                         void *)
{
    // std::cout << __func__ << std::endl;
    const int16_t *input = static_cast<const int16_t *>(inputBuffer);
    if (!input) {
        return paContinue;
    }

    int16_t *copy = new int16_t[framesPerBuffer];
    std::memcpy(copy, input, framesPerBuffer * sizeof(int16_t));
    input_que.push(copy, framesPerBuffer * sizeof(int16_t));

    // std::cout << "frames pre buffer: " << framesPerBuffer << std::endl;
    // std::cout << "input: " << reinterpret_cast<const char *>(input) << std::endl;

    return paContinue;
}

float sine_phase = 0.0;

// 队列中拉取，播放声音
static int outputCallback(const void *, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *,
                          PaStreamCallbackFlags,
                          void *)
{
    // std::cout << __func__ << std::endl;
    int16_t *output = static_cast<int16_t *>(outputBuffer);

    if (!output_que.empty()) {
        AudioQueue::FrameType p = output_que.pop();
        size_t copy_len =( std::min)(p.second, framesPerBuffer * sizeof(int16_t));
        std::memcpy(output, p.first, copy_len);
        // delete[] static_cast<int8_t *>(p.first);
        delete[] p.first;
    }
    else {
        std::memset(output, 0, framesPerBuffer * sizeof(int16_t));
    }

    // for (unsigned long i = 0; i < framesPerBuffer; ++i) {
    //     float sample = std::sin(sine_phase) * 0.3f;
    //     output[i] = static_cast<int16_t>(sample * 32767);
    //     sine_phase += 2.0f * M_PIf * 440.0f / 8000.0f;
    //     if (sine_phase >= 2.0f * M_PIf) {
    //         sine_phase -= 2.0f * M_PIf;
    //     }
    // }

    return paContinue;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, signal_handler);

    AudioDev dev(8000, 160, 1, paInt16);
    dev.openStream(outputCallback, inputCallback);
    dev.startStream();

    RtpServer server(output_que, input_que, "127.0.0.1",
                     8002,
                     8000,
                     1.0 / 8000.0,
                     160);

    std::thread t([&]() {
        server.poll(running);
    });

    std::thread t2([&]() {
        server.send(running);
    });

    // for (;;) {
    //     std::vector<uint8_t> fake(160, 0x55);
    //     server.sendFrame(fake.data(), fake.size());
    // }

    t.join();
    t2.join();

    return 0;
}