/*
 * jack_oscrolloscope
 *
 * Copyright (C) 2006  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "audio.h"
#include "waves.h"
#include "util.h"

#define SAMPLES_PER_PIXEL_MULTI     2
#define SAMPLES_PER_FRAME_MULTI     8
#define MIN_BUFFER_FRAMES           2048


static jack_client_t *client = NULL;
static jack_port_t **input_ports = NULL;

int audio_buffer_frames = 0;
jack_ringbuffer_t **audio_buffers = NULL;

jack_nframes_t audio_samplerate;

static void audio_exit();
static int audio_process(jack_nframes_t, void *);


void audio_init(char *name, char **connect_ports)
{
    if ((client = jack_client_new(name)) == 0) {
        fprintf(stderr, "can't connect to jack server\n");
        exit(EXIT_FAILURE);
    }
    jack_set_process_callback(client, &audio_process, NULL);

    atexit(audio_exit);

    if (jack_activate(client)) {
        fprintf(stderr, "can't activate client\n");
        exit(EXIT_FAILURE);
    }

    input_ports = (jack_port_t**)calloc(main_nports, sizeof(jack_port_t*));
    audio_buffers = (jack_ringbuffer_t**)calloc(main_nports, sizeof(jack_ringbuffer_t*));

    for (int n = 0; n < main_nports; n++)
    {
        char port_name[8];
        snprintf(port_name, 8, "in_%d", n + 1);
        if ((input_ports[n] = jack_port_register(client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == NULL) {
            fprintf(stderr, "can't register input port\n");
            exit(EXIT_FAILURE);
        }
        if (*connect_ports != NULL) {
            if (jack_connect(client, *connect_ports, jack_port_name(input_ports[n]))) {
                fprintf(stderr, "can't connect '%s' to '%s'\n", *connect_ports, jack_port_name(input_ports[n]));
            }
            connect_ports++;
        }
    }

    audio_samplerate = jack_get_sample_rate(client);

    audio_adjust();
}


void audio_adjust()
{
    int n = next_power_of_two(max3(
                waves_samples_per_pixel() * SAMPLES_PER_PIXEL_MULTI,
                waves_samples_per_frame() * SAMPLES_PER_FRAME_MULTI,
                MIN_BUFFER_FRAMES
            ));

    if (audio_buffer_frames != n)
    {
        audio_buffer_frames = n;

        for (int n = 0; n < main_nports; n++) {
            if (audio_buffers[n]) {
                jack_ringbuffer_free(audio_buffers[n]);
            }
            audio_buffers[n] = jack_ringbuffer_create(audio_buffer_frames * sizeof(sample_t));
        }
    }
}


static void audio_exit()
{
    if (client) {
        jack_deactivate(client);
        jack_client_close(client);
    }
    free(input_ports);
    if (audio_buffers) {
        for (int n = 0; n < main_nports; n++) jack_ringbuffer_free(audio_buffers[n]);
    }
    free(audio_buffers);
}


static int audio_process(jack_nframes_t nframes, void *p)
{
    static bool buffer_full = false;

    if (!main_run) return 0;

    for (int n = 0; n < main_nports; n++)
    {
        void *in = jack_port_get_buffer(input_ports[n], nframes);
        if (jack_ringbuffer_write_space(audio_buffers[n]) >= (nframes * sizeof(sample_t))) {
            jack_ringbuffer_write(audio_buffers[n], (const char*)in, (nframes * sizeof(sample_t)));
            buffer_full = false;
        } else {
            // this shouldn't be here...
            if (!buffer_full) fprintf(stderr, "insufficient buffer space\n");
            buffer_full = true;
            break;
        }
    }
    return 0;
}


jack_nframes_t audio_buffer_get_available()
{
    size_t minimum = SIZE_MAX;
    for (int n = 0; n < main_nports; n++) {
        size_t av = jack_ringbuffer_read_space(audio_buffers[n]);
        if (av < minimum) minimum = av;
    }
    return minimum / sizeof(sample_t);
}


void audio_buffer_read(int nport, sample_t *frames, jack_nframes_t nframes)
{
    jack_ringbuffer_read(audio_buffers[nport], (char*)frames, nframes * sizeof(sample_t));
}
