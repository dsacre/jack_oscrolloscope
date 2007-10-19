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

#ifndef _MAIN_H
#define _MAIN_H

#include <stdbool.h>

#define VERSION                     0.4

#define DEFAULT_WIDTH               480
#define DEFAULT_HEIGHT_PER_TRACK    120
#define DEFAULT_HEIGHT_MAX          480
#define DEFAULT_FPS                 50

#define DEFAULT_DURATION            5


extern bool main_run;
extern int  main_nports;


#endif // _MAIN_H
