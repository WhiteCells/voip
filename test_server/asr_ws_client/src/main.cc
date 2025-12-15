#include "audio_dev.h"
#include "logger.h"
#include "asr_ws_client.h"
#include <boost/asio/io_context.hpp>
#include <memory>
#include <cstdint>
#include <cstring>

static std::shared_ptr<ASRWsClient> asr_ws_client;

static int inputCallback(const void *inputBuffer, void *,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *,
                         PaStreamCallbackFlags,
                         void *)
{
    // LOG_INFO("inputCallback");
    const int16_t *input = static_cast<const int16_t *>(inputBuffer);
    if (!input) {
        return paContinue;
    }

    int16_t *copy = new int16_t[framesPerBuffer];
    std::memcpy(copy, input, framesPerBuffer * sizeof(int16_t));

    asr_ws_client->send(std::string(reinterpret_cast<const char *>(copy), framesPerBuffer * sizeof(int16_t)), true);

    // input_que.push(copy, framesPerBuffer * sizeof(int16_t));

    // std::cout << "frames pre buffer: " << framesPerBuffer << std::endl;
    // std::cout << "input: " << reinterpret_cast<const char *>(input) << std::endl;

    return paContinue;
}

static int outputCallback(const void *, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *,
                          PaStreamCallbackFlags,
                          void *)
{
    // LOG_INFO("outputCallback");
    // int16_t *output = static_cast<int16_t *>(outputBuffer);

    // if (!output_que.empty()) {
    //     AudioQueue::FrameType p = output_que.pop();
    //     size_t copy_len = (std::min)(p.second, framesPerBuffer * sizeof(int16_t));
    //     std::memcpy(output, p.first, copy_len);
    //     // delete[] static_cast<int8_t *>(p.first);
    //     delete[] p.first;
    // }
    // else {
    //     std::memset(output, 0, framesPerBuffer * sizeof(int16_t));
    // }

    return paContinue;
}

int main(int argc, char *argv[])
{
    Logger::init();

    AudioDev dev(16000, 320, 1, paInt16);
    dev.openStream(outputCallback, inputCallback);
    dev.startStream();

    net::io_context ioc;
    asr_ws_client = std::make_shared<ASRWsClient>(ioc,
                                                  "192.168.10.5",
                                                  "8080",
                                                  "/",
                                                  true,
                                                  "asr-server.crt");
    asr_ws_client->start();

    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });
    ioc_thread.join();

    // while (1) {
    // }
    return 0;
}