/*
 * jack_oscrolloscope
 *
 * Copyright (C) 2006-2010  Dominic Sacré  <dominic.sacre@gmx.de>
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
static int waves_height;
static int waves_draw_height;
static jack_nframes_t frames_per_line;
static int draw_pos = 0;

static Uint32 waves_color,
              waves_color_clipping,
              waves_color_position;


void waves_init()
{
    waves_color = SDL_MapRGB(video_get_pix_fmt(), 0, 255, 0);
    waves_color_clipping = g_show_clipping ? SDL_MapRGB(video_get_pix_fmt(), 255, 0, 0) : waves_color;
    waves_color_position = SDL_MapRGB(video_get_pix_fmt(), 255, 255, 255);

    if (g_use_gl)
    {
        waves_draw_play_head = waves_draw_play_head_gl;
    }
    else
    {
        waves_draw_play_head = waves_draw_play_head_sdl;
    }

    waves_adjust();
    atexit(waves_exit);
}


static void waves_exit()
{
    free(frames);
}


void waves_adjust()
{
    waves_height = g_height / g_nports;
    // actual height of one waveform is always an odd number
    waves_draw_height = waves_height - (int)(waves_height % 2 == 0);

    frames_per_line = (audio_get_samplerate() * g_duration) / g_width;
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


static inline void waves_analyze_frames(waves_line *line)
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

    float upper = (waves_draw_height * (1.0f - maxi)) / 2;
    float lower = (waves_draw_height * (1.0f - mini)) / 2;
    line->upper = max((int)floorf(upper), 0);
    line->lower = min((int)ceilf(lower), waves_draw_height);
}


static inline void waves_clear_line_all(int pos)
{
    SDL_Rect r = { pos, 0, 1, g_height };
    SDL_FillRect(video_get_draw_surface(), &r, 0);
}


static inline void waves_draw_line(int pos, int ntrack, waves_line *line)
{
    SDL_Rect r = { pos, ntrack * waves_height + line->upper, 1, line->lower - line->upper };
    SDL_FillRect(video_get_draw_surface(), &r, (line->clipping ? waves_color_clipping : waves_color));
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
    SDL_FillRect(video_get_screen(), &r_pos, waves_color_position);
    SDL_Rect r_pos_black = { (pos + 2) % g_width, 0, 2, g_height };
    SDL_FillRect(video_get_screen(), &r_pos_black, 0);
}


void waves_draw()
{
    int prev_pos = draw_pos;

    while (audio_buffer_get_available() >= frames_per_line)
    {
        waves_clear_line_all(g_use_gl ? 0 : draw_pos);

        for (int n = 0; n < g_nports; n++)
        {
            waves_line line;
            audio_buffer_read(n, frames, frames_per_line);
            waves_analyze_frames(&line);
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
