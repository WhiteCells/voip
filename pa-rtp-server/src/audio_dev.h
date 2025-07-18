#ifndef _AUDIODEV_H_
#define _AUDIODEV_H_

#include <portaudio.h>

class AudioDev
{
public:
    AudioDev(int sample_rate,
             int frame_per_buffer,
             int num_channels,
             PaSampleFormat sample_format);
    AudioDev(const AudioDev &) = delete;
    AudioDev &operator=(const AudioDev &) = delete;
    virtual ~AudioDev();

    void initParams();

    void openStream(PaStreamCallback output_callback,
                    PaStreamCallback input_callback);

    void startStream();

    void stop();

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