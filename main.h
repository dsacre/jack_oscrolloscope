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

#ifndef _MAIN_H
#define _MAIN_H

#include <stdbool.h>
#include <SDL.h>

#define VERSION                     0.7

#define DEFAULT_WIDTH               480
#define DEFAULT_HEIGHT_PER_TRACK    120
#define DEFAULT_HEIGHT_MAX          480
#define DEFAULT_FPS                 50
#define DEFAULT_DURATION            5


extern bool     g_run;
extern int      g_nports;

extern int      g_ticks_per_frame;
extern bool     g_scrolling;
extern int      g_width;
extern int      g_height;
extern int      g_total_height;
extern bool     g_use_gl;

extern float    g_duration;
extern bool     g_show_clipping;

extern Uint32  *g_colors;
extern float   *g_scales;
extern int     *g_heights;


#endif // _MAIN_H
