/*
 * JACK Output Plugin for Audacious
 * Copyright 2014 John Lindgren
 *
 * Based on the SDL Output Plugin and the previous JACK Output Plugin.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/plugin.h>
#include <libaudcore/preferences.h>
#include <libaudcore/ringbuf.h>
#include <libaudcore/runtime.h>

#include <algorithm>

#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include <jack/jack.h>

static_assert(std::is_same<jack_default_audio_sample_t, float>::value,
 "JACK must be compiled to use float samples");

class JACKOutput : public OutputPlugin
{
public:
    static const char * const defaults[];
    static const PreferencesWidget widgets[];
    static const PluginPreferences prefs;

    static constexpr PluginInfo info = {
        N_("JACK Output"),
        PACKAGE,
        nullptr,
        & prefs
    };

    constexpr JACKOutput () : OutputPlugin (info, 0) {}

    bool init ();

    StereoVolume get_volume ();
    void set_volume (StereoVolume v);

    bool open_audio (int format, int rate, int channels);
    void close_audio ();

    int buffer_free ();
    void period_wait ();
    void write_audio (const void * data, int size);
    void drain ();

    int output_time ();

    void pause (bool pause);
    void flush (int time);

private:
    bool connect_ports (int channels);
    void generate (jack_nframes_t frames);

    static void error_cb (const char * error)
        { AUDWARN ("%s\n", error); }
    static int generate_cb (jack_nframes_t frames, void * obj)
        { ((JACKOutput *) obj)->generate (frames); return 0; }

    int m_rate = 0, m_channels = 0;
    bool m_paused = false, m_prebuffer = false;
    int64_t m_frames_written = 0;

    int m_last_write_frames = 0;
    timeval m_last_write_time = timeval ();
    bool m_rate_mismatch = false;

    RingBuf<float> m_buffer;

    jack_client_t * m_client = nullptr;
    jack_port_t * m_ports[AUD_MAX_CHANNELS] = {};

    pthread_mutex_t m_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t m_cond = PTHREAD_COND_INITIALIZER;
};

EXPORT JACKOutput aud_plugin_instance;

const char * const JACKOutput::defaults[] = {
    "auto_connect", "TRUE",
    "volume_left", "100",
    "volume_right", "100",
    nullptr
};

const PreferencesWidget JACKOutput::widgets[] = {
    WidgetCheck (N_("Automatically connect to output ports"),
        WidgetBool ("jack", "auto_connect"))
};

const PluginPreferences JACKOutput::prefs = {{JACKOutput::widgets}};

bool JACKOutput::init ()
{
    aud_config_set_defaults ("jack", defaults);
    return true;
}

void JACKOutput::set_volume (StereoVolume v)
{
    aud_set_int ("jack", "volume_left", v.left);
    aud_set_int ("jack", "volume_right", v.right);
}

StereoVolume JACKOutput::get_volume ()
{
    return {aud_get_int ("jack", "volume_left"), aud_get_int ("jack", "volume_right")};
}

bool JACKOutput::connect_ports (int channels)
{
    bool success = false;
    const char * * ports = nullptr;
    int count = 0;

    if (! (ports = jack_get_ports (m_client, nullptr, nullptr,
     JackPortIsPhysical | JackPortIsInput)))
    {
        AUDERR ("jack_get_ports() failed\n");
        goto fail;
    }

    while (ports[count])
        count ++;

    if (count < channels)
    {
        aud_ui_show_error (str_printf (_("Only %d JACK output ports were "
         "found but %d are required."), count, channels));
        goto fail;
    }

    for (int i = 0; i < channels; i ++)
    {
        if (jack_connect (m_client, jack_port_name (m_ports[i]), ports[i]) != 0)
        {
            aud_ui_show_error (str_printf (_("Failed to connect to JACK port %s."), ports[i]));
            goto fail;
        }
    }

    success = true;

fail:
    if (ports)
        jack_free (ports);

    return success;
}

bool JACKOutput::open_audio (int format, int rate, int channels)
{
    int buffer_time;

    if (format != FMT_FLOAT)
    {
        aud_ui_show_error (_("JACK supports only floating-point audio.  You "
         "must change the output bit depth to floating-point in Audacious "
         "settings."));
        return false;
    }

    assert (rate > 0 && channels > 0 && channels < AUD_MAX_CHANNELS);
    assert (! m_client);

    jack_set_error_function (error_cb);

    if (! (m_client = jack_client_open ("audacious", JackNoStartServer, nullptr)))
    {
        aud_ui_show_error (_("Failed to connect to the JACK server; is it running?"));
        goto fail;
    }

    for (int i = 0; i < channels; i ++)
    {
        StringBuf name = str_printf ("out_%d", i);
        if (! (m_ports[i] = jack_port_register (m_client, name,
         JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)))
        {
            AUDERR ("jack_port_register() failed\n");
            goto fail;
        }
    }

    buffer_time = aud_get_int (nullptr, "output_buffer_size");
    m_buffer.alloc (aud::rescale (buffer_time, 1000, rate) * channels);

    m_rate = rate;
    m_channels = channels;
    m_paused = false;
    m_prebuffer = true;
    m_frames_written = 0;

    m_last_write_frames = 0;
    m_last_write_time = timeval ();
    m_rate_mismatch = false;

    jack_set_process_callback (m_client, generate_cb, this);

    if (jack_activate (m_client) != 0)
    {
        AUDERR ("jack_activate() failed\n");
        goto fail;
    }

    if (aud_get_bool ("jack", "auto_connect"))
    {
        if (! connect_ports (channels))
            goto fail;
    }

    return true;

fail:
    close_audio ();
    return false;
}

void JACKOutput::close_audio ()
{
    if (m_client)
        jack_client_close (m_client);

    m_buffer.destroy ();

    std::fill (m_ports, std::end (m_ports), nullptr);
    m_client = nullptr;
}

void JACKOutput::generate (jack_nframes_t frames)
{
    pthread_mutex_lock (& m_mutex);

    m_last_write_frames = 0;
    gettimeofday (& m_last_write_time, nullptr);

    float * out[AUD_MAX_CHANNELS];
    for (int i = 0; i < m_channels; i ++)
        out[i] = (float *) jack_port_get_buffer (m_ports[i], frames);

    int jack_rate = jack_get_sample_rate (m_client);

    if (jack_rate != m_rate)
    {
        if (! m_rate_mismatch)
        {
            aud_ui_show_error (str_printf (_("The JACK server requires a "
             "sample rate of %d Hz, but Audacious is playing at %d Hz.  Please "
             "use the Sample Rate Converter effect to correct the mismatch."),
             jack_rate, m_rate));
            m_rate_mismatch = true;
        }

        goto silence;
    }

    m_rate_mismatch = false;

    if (m_paused || m_prebuffer)
        goto silence;

    while (frames && m_buffer.len ())
    {
        int linear_samples = m_buffer.linear ();
        assert (linear_samples % m_channels == 0);

        int frames_to_copy = aud::min (frames, (jack_nframes_t) linear_samples / m_channels);

        audio_amplify (& m_buffer[0], m_channels, frames_to_copy, get_volume ());
        audio_deinterlace (& m_buffer[0], FMT_FLOAT, m_channels,
         (void * const *) out, frames_to_copy);

        m_last_write_frames += frames_to_copy;
        m_buffer.discard (frames_to_copy * m_channels);

        for (int i = 0; i < m_channels; i ++)
            out[i] += frames_to_copy;

        frames -= frames_to_copy;
    }

silence:
    for (int i = 0; i < m_channels; i ++)
        std::fill (out[i], out[i] + frames, 0.0);

    pthread_cond_broadcast (& m_cond);
    pthread_mutex_unlock (& m_mutex);
}

int JACKOutput::buffer_free ()
{
    pthread_mutex_lock (& m_mutex);
    int samples = m_buffer.space ();
    pthread_mutex_unlock (& m_mutex);
    return samples * sizeof (float);
}

void JACKOutput::period_wait ()
{
    pthread_mutex_lock (& m_mutex);

    while (! m_buffer.space ())
    {
        m_prebuffer = false;
        pthread_cond_wait (& m_cond, & m_mutex);
    }

    pthread_mutex_unlock (& m_mutex);
}

void JACKOutput::write_audio (const void * data, int size)
{
    pthread_mutex_lock (& m_mutex);

    int samples = size / sizeof (float);
    assert (samples % m_channels == 0);

    m_buffer.copy_in ((const float *) data, samples);
    m_frames_written += samples / m_channels;

    if (m_buffer.len () >= m_buffer.size () / 4)
        m_prebuffer = false;

    pthread_mutex_unlock (& m_mutex);
}

void JACKOutput::drain ()
{
    pthread_mutex_lock (& m_mutex);

    m_prebuffer = false;

    while (m_buffer.len () || m_last_write_frames)
        pthread_cond_wait (& m_cond, & m_mutex);

    pthread_mutex_unlock (& m_mutex);
}

int JACKOutput::output_time ()
{
    auto timediff = [] (const timeval & a, const timeval & b) -> int64_t
        { return 1000 * (int64_t) (b.tv_sec - a.tv_sec) + (b.tv_usec - a.tv_usec) / 1000; };

    pthread_mutex_lock (& m_mutex);

    int64_t frames = m_frames_written - m_buffer.len () / m_channels;
    int time = aud::rescale<int64_t> (frames, m_rate, 1000);

    if (m_last_write_frames)
    {
        timeval now;
        gettimeofday (& now, nullptr);

        int written = aud::rescale (m_last_write_frames, m_rate, 1000);
        int64_t elapsed = timediff (m_last_write_time, now);

        if (elapsed < written)
            time -= written - (int) elapsed;
    }

    pthread_mutex_unlock (& m_mutex);
    return time;
}

void JACKOutput::pause (bool pause)
{
    pthread_mutex_lock (& m_mutex);
    m_paused = pause;
    pthread_mutex_unlock (& m_mutex);
}

void JACKOutput::flush (int time)
{
    pthread_mutex_lock (& m_mutex);

    m_buffer.discard ();

    m_prebuffer = true;
    m_frames_written = aud::rescale<int64_t> (time, 1000, m_rate);

    m_last_write_frames = 0;
    m_last_write_time = timeval ();

    pthread_cond_broadcast (& m_cond);
    pthread_mutex_unlock (& m_mutex);
}
