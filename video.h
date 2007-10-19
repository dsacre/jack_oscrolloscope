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

void video_init();
void video_set_mode(int w, int h);
void video_resize(int w, int h);
void video_update_line(int);
void video_flip();

extern void (*video_update)(int, int);


extern int  video_ticks_per_frame;
extern bool video_scrolling;

extern bool video_use_gl;
extern int  video_width, video_height;

extern SDL_Surface *video_screen;
extern SDL_Surface *video_draw_surface;

extern SDL_PixelFormat *video_pix_fmt;


#endif // _VIDEO_H
