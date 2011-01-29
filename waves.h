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

#ifndef _WAVES_H
#define _WAVES_H

void waves_init();
void waves_adjust();
void waves_draw();

int waves_samples_per_pixel();
int waves_samples_per_frame();

#endif // _WAVES_H
