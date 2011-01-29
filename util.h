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

#ifndef _UTIL_H
#define _UTIL_H

#define min(a, b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a < _b ? _a : _b; })
#define max(a, b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a > _b ? _a : _b; })

#define max3(a, b, c) max(a, max(b, c))

#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x

static inline int next_power_of_two(int i) {
    int r = 1;
    while (r < i) r *= 2;
    return r;
}

#endif // _UTIL_H
