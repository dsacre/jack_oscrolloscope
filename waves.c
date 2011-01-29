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

#include <SDL.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "main.h"
#include "video.h"
#include "audio.h"
#include "waves.h"
#include "util.h"


typedef struct {
    int upper;
    int lower;
    bool clipping;
} waves_line;


static void waves_exit();

static void waves_clear_line_all(int);
static void waves_draw_line(int, int, waves_line*);

static void (*waves_draw_play_head)(int);

static void waves_draw_play_head_gl(int);
static void waves_draw_play_head_sdl(int);


static sample_t *frames = NULL;
static int *track_heights = NULL;
static int *track_yoffsets = NULL;
static int *draw_heights = NULL;
static jack_nframes_t frames_per_line;
static int draw_pos = 0;

static Uint32 *colors = NULL;
static Uint32 *colors_clipping = NULL;
static Uint32 color_position;


void waves_init()
{
    colors = (Uint32*)calloc(g_nports, sizeof(Uint32));
    colors_clipping = (Uint32*)calloc(g_nports, sizeof(Uint32));

    for (int n = 0; n < g_nports; ++n) {
        Uint32 c;
        if (g_colors) {
            c = g_colors[n];
        } else {
            // use green as default color
            c = 0x00ff00;
        }

        Uint8 r = c >> 16 & 0xff,
              g = c >>  8 & 0xff,
              b = c & 0xff;
        colors[n] = SDL_MapRGB(video_get_pix_fmt(), r, g, b);
        colors_clipping[n] = SDL_MapRGB(video_get_pix_fmt(), 255 - r, 255 - g, 255 - b);
    }

    color_position = SDL_MapRGB(video_get_pix_fmt(), 255, 255, 255);

    if (g_use_gl) {
        waves_draw_play_head = waves_draw_play_head_gl;
    } else {
        waves_draw_play_head = waves_draw_play_head_sdl;
    }

    waves_adjust();
    atexit(waves_exit);
}


static void waves_exit()
{
    free(colors);
    free(colors_clipping);
    free(frames);
    free(track_heights);
    free(track_yoffsets);
    free(draw_heights);
}


void waves_adjust()
{
    track_heights = (int*)realloc(track_heights, g_nports * sizeof(int));
    track_yoffsets = (int*)realloc(track_yoffsets, g_nports * sizeof(int));
    draw_heights = (int*)realloc(draw_heights, g_nports * sizeof(int));

    int yoffset = 0;

    for (int n = 0; n < g_nports; ++n) {
        if (g_heights) {
            track_heights[n] = ((float)g_heights[n] / g_total_height) * g_height;
        } else {
            track_heights[n] = g_height / g_nports;
        }

        track_yoffsets[n] = yoffset;
        yoffset += track_heights[n];

        // actual height of one waveform is always an odd number
        draw_heights[n] = track_heights[n] - (int)(track_heights[n] % 2 == 0);
    }

    // don't allow frames_per_line to be zero
    frames_per_line = max((audio_get_samplerate() * g_duration) / g_width, 1);
    draw_pos = 0;

    frames = (sample_t*)realloc(frames, frames_per_line * sizeof(sample_t));
}


int waves_samples_per_pixel()
{
    return audio_get_samplerate() * g_duration / g_width;
}


int waves_samples_per_frame()
{
    return audio_get_samplerate() * g_ticks_per_frame / 1000;
}


static inline void waves_analyze_frames(int ntrack, waves_line *line)
{
    sample_t maxi = frames[0];
    sample_t mini = frames[0];
    line->clipping = false;

    // find maximum and minimum sample value
    for (unsigned int i = 1; i < frames_per_line; i++) {
        if (frames[i] > maxi) maxi = frames[i];
        if (frames[i] < mini) mini = frames[i];
        if (frames[i] >= 1.0 || frames[i] <= -1.0) line->clipping = true;
    }

    // scale signal
    if (g_scales) {
        maxi *= g_scales[ntrack];
        mini *= g_scales[ntrack];
    }

    float upper = (draw_heights[ntrack] * (1.0f - maxi)) / 2;
    float lower = (draw_heights[ntrack] * (1.0f - mini)) / 2;
    line->upper = max((int)floorf(upper), 0);
    line->lower = min((int)ceilf(lower), draw_heights[ntrack]);
}


static inline void waves_clear_line_all(int pos)
{
    SDL_Rect r = { pos, 0, 1, g_height };
    SDL_FillRect(video_get_draw_surface(), &r, 0);
}


static inline void waves_draw_line(int pos, int ntrack, waves_line *line)
{
    SDL_Rect r = {
        pos,
        track_yoffsets[ntrack] + line->upper,
        1,
        line->lower - line->upper
    };
    Uint32 c = (line->clipping && g_show_clipping) ? colors_clipping[ntrack] : colors[ntrack];

    SDL_FillRect(video_get_draw_surface(), &r, c);
}


static void waves_draw_play_head_gl(int pos)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2i(pos, 0);
    glVertex2i(pos, g_height);
    glVertex2i(pos + 1, 0);
    glVertex2i(pos + 1, g_height);
    for (int x = 2; x < 20; x++) {
        glColor4f(0.0f, 0.0f, 0.0f, 0.8f - (0.04f * x));
        glVertex2i((pos + x) % g_width, 0);
        glVertex2i((pos + x) % g_width, g_height);
    }
    glEnd();

    glDisable(GL_BLEND);
}


static void waves_draw_play_head_sdl(int pos)
{
    SDL_Rect r_pos = { pos, 0, 2, g_height };
    SDL_FillRect(video_get_screen(), &r_pos, color_position);
    SDL_Rect r_pos_black = { (pos + 2) % g_width, 0, 2, g_height };
    SDL_FillRect(video_get_screen(), &r_pos_black, 0);
}


void waves_draw()
{
    int prev_pos = draw_pos;
    int count = 0;

    while (audio_buffer_get_available() >= frames_per_line)
    {
        // this is just a simplistic safeguard in case we can't keep up with incoming audio samples.
        // the waveform might be garbled, but at least this way the program won't lock up completely.
        if (++count > 4096) {
            break;
        }

        waves_clear_line_all(g_use_gl ? 0 : draw_pos);

        for (int n = 0; n < g_nports; n++)
        {
            waves_line line;
            audio_buffer_read(n, frames, frames_per_line);
            waves_analyze_frames(n, &line);
            waves_draw_line(g_use_gl ? 0 : draw_pos, n, &line);
        }

        video_update_line(draw_pos);

        draw_pos = (draw_pos + 1) % g_width;
    }

    video_update(draw_pos, prev_pos);

    if (!g_scrolling) {
        waves_draw_play_head(draw_pos);
    }
}
