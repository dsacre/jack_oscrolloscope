/*
 * jack_oscrolloscope
 *
 * Copyright (C) 2006-2010  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <SDL.h>
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


bool g_run = false;

char const * g_client_name = "jack_oscrolloscope";
int  g_nports = 0;

int  g_ticks_per_frame = 1000 / DEFAULT_FPS;

bool g_scrolling = true;
int  g_width = DEFAULT_WIDTH;
int  g_height = 0;
bool g_use_gl = false;

int  g_duration = DEFAULT_DURATION;
bool g_show_clipping = false;


static void print_usage()
{
    fprintf(stderr, "jack_oscrolloscope " STRINGIFY(VERSION) "\n"
            "\n"
            "Usage:\n"
            "  jack_oscrolloscope [ options ] [ port1 port2 ... ]\n"
            "\n"
            "Options:\n"
            "  -N <name>        JACK client name\n"
            "  -n <number>      number of input ports\n"
            "  -d <seconds>     duration of audio being displayed (default " STRINGIFY(DEFAULT_DURATION) "s)\n"
            "  -c               indicate clipping\n"
            "  -s               disable scrolling\n"
            "  -x <pixels>      set window width\n"
            "  -y <pixels>      set window height\n"
            "  -g               use OpenGL for drawing\n"
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


static void process_options(int argc, char *argv[])
{
    int c;
    const char *optstring = "N:n:d:c::s::x:y:g::f:h";

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
                g_duration = atoi(optarg);
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
            case 'g':
                g_use_gl = optional_bool(optarg);
                break;
            case 'f':
              { int fps = atoi(optarg);
                if (fps) g_ticks_per_frame = 1000 / fps;
                    else g_ticks_per_frame = 0;
              } break;
            case 'h':
            default:
                print_usage();
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


int main(int argc, char *argv[])
{
    SDL_Event event;

    process_configfile();
    process_options(argc, argv);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "can't init SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    atexit(SDL_Quit);

    // use opt_nports if specified, otherwise use the number of port arguments.
    // if neither is given, create just one port
    int nportargs = argc - optind;
    g_nports = max(1, g_nports ? : nportargs);

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
