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

#ifndef _VIDEO_H
#define _VIDEO_H

void video_init();
void video_set_mode(int w, int h);
void video_resize(int w, int h);
void video_update_line(int);
void video_flip();

extern void (*video_update)(int, int);

SDL_Surface *video_get_screen();
SDL_Surface *video_get_draw_surface();
SDL_PixelFormat *video_get_pix_fmt();

#endif // _VIDEO_H
