/*
 * jack_oscrolloscope
 *
 * Copyright (C) 2006-2011  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _AUDIO_H
#define _AUDIO_H

#include <jack/types.h>
#include <jack/ringbuffer.h>

typedef jack_default_audio_sample_t sample_t;

void audio_init(const char *name, const char * const * connect_ports);
void audio_adjust();

const char * audio_get_client_name();
jack_nframes_t audio_get_samplerate();

jack_nframes_t audio_buffer_get_available();
void audio_buffer_read(int nport, sample_t *frames, jack_nframes_t nframes);

#endif // _AUDIO_H
