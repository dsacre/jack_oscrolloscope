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

#ifndef _VIDEO_H
#define _VIDEO_H


#define VIDEO_BPP 0
#define VIDEO_FLAGS_SDL (SDL_RESIZABLE | SDL_SWSURFACE)
#define VIDEO_FLAGS_GL (SDL_RESIZABLE | SDL_OPENGL)
#define SURFACE_FLAGS SDL_SWSURFACE

#define DEFAULT_WIDTH 480
#define DEFAULT_HEIGHT_PER_TRACK 120
#define DEFAULT_HEIGHT_MAX 480
#define DEFAULT_FPS 50


typedef struct {
    bool use;
    SDL_Rect rect;
} update_rect;


void video_init();
void video_set_mode(int w, int h);
void video_resize(int w, int h);
void video_update_line(int);
void video_flip();

void (*video_update)(int, int);


extern int  video_ticks_per_frame;
extern bool video_scrolling;

extern bool video_use_gl;
extern int  video_width, video_height;

extern SDL_Surface *video_screen;
extern SDL_Surface *video_draw_surface;

extern SDL_PixelFormat *video_pix_fmt;
extern update_rect video_updates[2];


#endif // _VIDEO_H
