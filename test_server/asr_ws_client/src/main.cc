#include "asr_ws_client.h"
#include "audiodev.h"
#include <thread>

int main()
{
    try {
        asio::io_context ioc;

        std::string host = "127.0.0.1";
        std::string port = "10095";
        std::string path = "/";

        auto client = std::make_shared<ASRWsClient>(ioc, host, port, path, true);

        std::thread ioth([&ioc]() {
            ioc.run();
        });

        for (int i = 0; i < 10; ++i) {
            client->start();
            client->stop();
        }

        client->start();

        Json::Value config;
        config["mode"] = "2pass";
        config["wav_name"] = "record";
        config["wav_format"] = "pcm";
        config["audio_fs"] = 16000.0;
        config["is_speaking"] = true;
        config["itn"] = true;
        config["svs_itn"] = true;

        Json::Value chunk_size(Json::arrayValue);
        chunk_size.append(5);
        chunk_size.append(10);
        chunk_size.append(5);
        config["chunk_size"] = chunk_size;

        Json::StreamWriterBuilder builder;
        std::string config_str = Json::writeString(builder, config);
        client->send(config_str);

        AudioDev audio_dev = AudioDev(16000, 320);
        audio_dev.start([client](const std::vector<int16_t> &data, size_t samples) {
            std::string pcm((char *)data.data(), samples * sizeof(int16_t));
            // std::cout << pcm << std::endl;
            client->send(pcm, false); // binary
        });

        if (ioth.joinable()) {
            ioth.join();
        }

        std::cout << "client exited\n";
    }
    catch (const std::exception &ex) {
        std::cerr << "Exception: " << ex.what() << "\n";
    }

    return 0;
}
