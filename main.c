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

#include <SDL.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "main.h"
#include "video.h"
#include "audio.h"
#include "waves.h"
#include "util.h"


bool    g_run = false;
int     g_nports = 0;

int     g_ticks_per_frame = 1000 / DEFAULT_FPS;
bool    g_scrolling = true;
int     g_width = DEFAULT_WIDTH;
int     g_height = 0;
int     g_total_height;
bool    g_use_gl = true;

float   g_duration = DEFAULT_DURATION;
bool    g_show_clipping = false;

Uint32  *g_colors = NULL;
float   *g_scales = NULL;
int     *g_heights = NULL;

static int  ncolors = 0;
static int  nscales = 0;
static int  nheights = 0;

static char const * g_client_name = "jack_oscrolloscope";


static void print_usage()
{
    fprintf(stderr, "jack_oscrolloscope " STRINGIFY(VERSION) "\n"
            "\n"
            "Usage:\n"
            "  jack_oscrolloscope [options] [port ...]\n"
            "\n"
            "Options:\n"
            "  -N <name>        JACK client name\n"
            "  -n <number>      number of input ports\n"
            "  -d <seconds>     duration of audio being displayed (default " STRINGIFY(DEFAULT_DURATION) "s)\n"
            "  -c               indicate clipping\n"
            "  -s               disable scrolling\n"
            "  -x <pixels>      set window width\n"
            "  -y <pixels>      set window height\n"
            "  -C <color,...>   set waveform color\n"
            "  -S <scale,...>   set waveform scale\n"
            "  -Y <height,...>  set waveform height (per port)\n"
            "  -G               don't use OpenGL for drawing\n"
            "  -f <fps>         video frames per second (default " STRINGIFY(DEFAULT_FPS) ", 0 = unlimited/vsync)\n"
            "  -h               show this help\n");
}


static inline bool optional_bool(const char* arg)
{
    if (!arg) return true;
    if (strcmp(arg, "1") == 0) return true;
    if (strcmp(arg, "0") == 0) return false;
    return true;
}


static int count_char(const char *s, char c)
{
    int n = 0;
    while (*s != '\0') {
        if (*s++ == c) ++n;
    }
    return n;
}


static void parse_colors(char *s)
{
    int n = ncolors + 1 + count_char(s, ',');

    g_colors = (Uint32*)realloc(g_colors, n * sizeof(Uint32));

    // we'll need an X display for this... yuck!
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "can't open display\n");
        exit(EXIT_FAILURE);
    }
    Colormap cmap = DefaultColormap(dpy, 0);

    // tokenize the string and parse each color
    char *p = strsep(&s, ",");
    while (p) {
        Uint32 c;
        if (strlen(p) || !ncolors) {
            XColor color;
            if (!XParseColor(dpy, cmap, p, &color)) {
                fprintf(stderr, "can't parse color: %s\n", p);
                exit(EXIT_FAILURE);
            }

            c = (color.red / 256) << 16 | (color.green / 256) << 8 | (color.blue / 256);
        } else {
            c = g_colors[ncolors - 1];
        }
        g_colors[ncolors++] = c;

        p = strsep(&s, ",");
    }
}


static void parse_scales(char *s)
{
    int n = nscales + 1 + count_char(s, ',');

    g_scales = (float*)realloc(g_scales, n * sizeof(float));

    char *p = strsep(&s, ",");
    while (p) {
        float f;
        if (strlen(p) || !nscales) {
            f = atof(p);
        } else {
            f = g_scales[nscales - 1];
        }
        g_scales[nscales++] = f;

        p = strsep(&s, ",");
    }
}


static void parse_heights(char *s)
{
    int n = nheights + 1 + count_char(s, ',');

    g_heights = (int*)realloc(g_heights, n * sizeof(int));

    char *p = strsep(&s, ",");
    while(p) {
        int h;
        if (strlen(p) || !nheights) {
            h = atoi(p);
        } else {
            h = g_heights[nheights - 1];
        }
        g_heights[nheights++] = h;

        p = strsep(&s, ",");
    }
}


static void process_options(int argc, char *argv[])
{
    int c;
    const char *optstring = "N:n:d:c::s::x:y:C:S:Y:g::G::f:h";

    optind = 1;
    opterr = 1;

    while ((c = getopt(argc, argv, optstring)) != -1)
    {
        switch (c) {
            case 1:
                break;  // end of options
            case 'N':
                g_client_name = optarg;
                break;
            case 'n':
                g_nports = atoi(optarg);
                break;
            case 'd':
                g_duration = atof(optarg);
                break;
            case 'c':
                g_show_clipping = optional_bool(optarg);
                break;
            case 's':
                g_scrolling = !optional_bool(optarg);
                break;
            case 'x':
                g_width = atoi(optarg);
                break;
            case 'y':
                g_height = atoi(optarg);
                break;
            case 'C':
                parse_colors(optarg);
                break;
            case 'S':
                parse_scales(optarg);
                break;
            case 'Y':
                parse_heights(optarg);
                break;
            case 'g':
                g_use_gl = optional_bool(optarg);
                break;
            case 'G':
                g_use_gl = !optional_bool(optarg);
                break;
            case 'f':
              { int fps = atoi(optarg);
                if (fps) g_ticks_per_frame = 1000 / fps;
                    else g_ticks_per_frame = 0;
              } break;
            case 'h':
                print_usage();
                exit(EXIT_SUCCESS);
                break;
            default:
                exit(EXIT_FAILURE);
                break;
        }
    }
}


static void process_configfile()
{
    char *home = getenv("HOME");
    char cfg_path[256] = { 0 };
    if (home) snprintf(cfg_path, 256, "%s/.jack_oscrolloscoperc", home);

    FILE *cfg_file;

    if ((cfg_file = fopen(cfg_path, "r")))
    {
        char str[256];
        int argc = 1;
        char *argv[16] = { NULL };

        if (fgets(str, 256, cfg_file)) {
            char *token = strtok(str, " \t\n");
            while (token && argc < 16) {
                argv[argc++] = token;
                token = strtok(NULL, " \t\n");
            }
        }
        fclose(cfg_file);
        process_options(argc, argv);
    }
}


static void main_exit()
{
    free(g_colors);
    free(g_scales);
    free(g_heights);
}


int main(int argc, char *argv[])
{
    SDL_Event event;

    atexit(main_exit);

    process_configfile();
    process_options(argc, argv);

    // use g_nports if specified, otherwise use the number of port arguments.
    // if neither is given, create just one port
    int nportargs = argc - optind;
    g_nports = max(1, g_nports ? : nportargs);

    // now that we know the actual number of ports, repeat the last color/scale/height value
    // as often as necessary
    if (g_colors) {
        g_colors = (Uint32*)realloc(g_colors, g_nports * sizeof(Uint32));
        for (int n = ncolors; n < g_nports; ++n) {
            g_colors[n] = g_colors[ncolors - 1];
        }
    }

    if (g_scales) {
        g_scales = (float*)realloc(g_scales, g_nports * sizeof(float));
        for (int n = nscales; n < g_nports; ++n) {
            g_scales[n] = g_scales[nscales - 1];
        }
    }

    if (g_heights) {
        g_heights = (int*)realloc(g_heights, g_nports * sizeof(int));
        for (int n = nheights; n < g_nports; ++n) {
            g_heights[n] = g_heights[nheights - 1];
        }
        // g_heights overrides g_height
        g_height = 0;
        for (int n = 0; n < g_nports; ++n) {
            g_height += g_heights[n];
        }
        g_total_height = g_height;
    }


    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "can't init SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    atexit(SDL_Quit);

    audio_init(g_client_name, (const char * const *)&argv[optind]);

    video_init();
    SDL_WM_SetCaption(audio_get_client_name(), NULL);

    waves_init();

    g_run = true;

    while (g_run)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_VIDEORESIZE:
                    video_resize(event.resize.w, event.resize.h);
                    audio_adjust();
                    waves_adjust();
                    break;
                case SDL_QUIT:
                    g_run = false;
                    break;
            }
        }

        waves_draw();
        video_flip();
    }

    return 0;
}
