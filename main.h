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

#define min(a, b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a < _b ? _a : _b; })
#define max(a, b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a > _b ? _a : _b; })

#include <stdbool.h>

#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x

#define VERSION 0.3

extern bool main_run;
extern int  main_nports;

#endif // _MAIN_H
