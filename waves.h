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

#ifndef _WAVES_H
#define _WAVES_H

#define DEFAULT_DURATION 5

void waves_init();
void waves_adjust();
void waves_draw();

extern int waves_duration;
extern bool waves_show_clipping;

#endif // _WAVES_H
