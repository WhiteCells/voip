#ifndef _AUDIODEV_H_
#define _AUDIODEV_H_

#include <portaudio.h>

class AudioDev
{
public:
    AudioDev(int sample_rate,
             int frame_per_buffer,
             int num_channels,
             PaSampleFormat sample_format) :
        m_sample_rate(sample_rate),
        m_frame_per_buffer(frame_per_buffer),
        m_num_channels(num_channels),
        m_sample_format(sample_format)
    {
        Pa_Initialize();
        initParams();
    }

    AudioDev(const AudioDev &) = delete;
    AudioDev &operator=(const AudioDev &) = delete;
    virtual ~AudioDev()
    {
        stop();
    }

    void initParams()
    {
        // output
        m_output_params.device = Pa_GetDefaultOutputDevice();
        m_output_params.channelCount = m_num_channels;
        m_output_params.sampleFormat = m_sample_format;
        m_output_params.suggestedLatency = Pa_GetDeviceInfo(m_output_params.device)->defaultLowOutputLatency;
        m_output_params.hostApiSpecificStreamInfo = nullptr;

        // input
        m_input_params.device = Pa_GetDefaultInputDevice();
        m_input_params.channelCount = m_num_channels;
        m_input_params.sampleFormat = m_sample_format;
        m_input_params.suggestedLatency = Pa_GetDeviceInfo(m_input_params.device)->defaultLowInputLatency;
        m_input_params.hostApiSpecificStreamInfo = nullptr;
    }

    void openStream(PaStreamCallback output_callback,
                    PaStreamCallback input_callback)
    {
        // output
        Pa_OpenStream(&m_output_stream, nullptr, &m_output_params,
                      m_sample_rate, m_frame_per_buffer, paClipOff,
                      output_callback, nullptr);

        // input
        Pa_OpenStream(&m_input_stream, &m_input_params, nullptr,
                      m_sample_rate, m_frame_per_buffer, paClipOff,
                      input_callback, nullptr);
    }

    void startStream()
    {
        // output
        Pa_StartStream(m_output_stream);

        // input
        Pa_StartStream(m_input_stream);
    }

    void stop()
    {
        // output
        Pa_StopStream(m_output_stream);
        Pa_CloseStream(m_output_stream);

        // input
        Pa_StopStream(m_input_stream);
        Pa_CloseStream(m_input_stream);
    }

private:
    int m_sample_rate;
    int m_frame_per_buffer;
    int m_num_channels;
    PaSampleFormat m_sample_format;

    PaStreamParameters m_input_params;
    PaStreamParameters m_output_params;

    // stream
    PaStream *m_input_stream;
    PaStream *m_output_stream;
};

#endif // _AUDIODEV_H_