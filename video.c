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
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "main.h"
#include "video.h"
#include "util.h"

#define VIDEO_BPP       0
#define VIDEO_FLAGS_SDL (SDL_RESIZABLE | SDL_SWSURFACE)
#define VIDEO_FLAGS_GL  (SDL_RESIZABLE | SDL_OPENGL)
#define SURFACE_FLAGS   SDL_SWSURFACE
#define TEXTURE_WIDTH   64


static void video_exit();

void (*video_update)(int, int);
static void video_update_gl(int, int);
static void video_update_sdl(int, int);


static SDL_Surface *screen = NULL;
static SDL_Surface *buffer = NULL;
static SDL_Surface *draw_surface = NULL;

static SDL_PixelFormat *pix_fmt = NULL;

static unsigned int ticks = 0;

typedef struct {
    bool use;
    SDL_Rect rect;
} update_rect;

static update_rect update_rects[2];

static GLuint *textures = NULL;
static int    num_textures = 0;
static float  tex_coord_h;
static int    max_texture_size = 0;


void video_init()
{
    for (int n = 0; n < 2; n++) {
        update_rects[n].use = false;
    }

    if (g_use_gl)
    {
        if (g_ticks_per_frame == 0) {
            // attempt to enable vsync, which makes scrolling more smooth.
            // this probably has no effect on anything but nVidia cards.
            setenv("__GL_SYNC_TO_VBLANK", "1", 0);
#if SDL_VERSION_ATLEAST(1, 2, 10)
            // this might help, too... but has no effect for me
            SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
#endif
        }
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    }

    if (!g_height) {
        g_height = min(DEFAULT_HEIGHT_PER_TRACK * g_nports, DEFAULT_HEIGHT_MAX);
    }

    video_set_mode(g_width, g_height);

    atexit(video_exit);
}


static void video_exit()
{
    if (g_use_gl)
    {
        glDeleteTextures(num_textures, textures);
        SDL_FreeSurface(buffer);
        free(textures);
    }
    else
    {
        if (g_scrolling) SDL_FreeSurface(buffer);
    }
}


void video_set_mode(int w, int h)
{
    if (g_use_gl && max_texture_size != 0 && h > max_texture_size) {
        fprintf(stderr, "maximum OpenGL texture size exceeded\n");
        h = max_texture_size;
    }

    g_width = w;
    g_height = h;

    if ((screen = SDL_SetVideoMode(g_width, g_height, VIDEO_BPP, (g_use_gl ? VIDEO_FLAGS_GL : VIDEO_FLAGS_SDL))) == NULL)
    {
        fprintf(stderr, "can't set video mode: %s\n", SDL_GetError());
        if (g_use_gl) {
            fprintf(stderr, "you may want to try disabling OpenGL support using the -G command line option\n");
        }
        exit(EXIT_FAILURE);
    }

    if (g_use_gl)
    {
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

        if (textures) glDeleteTextures(num_textures, textures);

        num_textures = (int)ceilf((float)g_width / (float)TEXTURE_WIDTH);
        textures = (GLuint*)realloc(textures, num_textures * sizeof(GLuint));
        glGenTextures(num_textures, textures);

        int tex_w = TEXTURE_WIDTH;
        int tex_h = next_power_of_two(g_height);
        tex_coord_h = (float)g_height / (float)tex_h;

        // used to initially fill the textures
        void *black_pixels = calloc(tex_w * tex_h, 4);

        for (int n = 0; n < num_textures; n++)
        {
            glBindTexture(GL_TEXTURE_2D, textures[n]);

            // empty error flags
            while (glGetError()) { }
            // width and height swapped, so that we're able to change the texture one row at a time
            // (more efficient than one column!)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_h, tex_w, 0, GL_RGBA, GL_UNSIGNED_BYTE, black_pixels);
            if (glGetError()) {
                fprintf(stderr, "failed to create texture of size %dx%d, sorry\n", tex_h, tex_w);
                exit(EXIT_FAILURE);
            }

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        free(black_pixels);

        glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0, 0, g_width, g_height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, (GLdouble)g_width, (GLdouble)g_height, 0.0, -1.0, 1.0);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        if (buffer) SDL_FreeSurface(buffer);

        // give OpenGL the pixel format it expects
        buffer = SDL_CreateRGBSurface(SDL_SWSURFACE, 1, g_height, 32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#else
            0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#endif
        );
        pix_fmt = buffer->format;

        draw_surface = buffer;
        video_update = video_update_gl;
    }
    else // SDL
    {
        pix_fmt = screen->format;

        if (g_scrolling) {
            if (buffer) SDL_FreeSurface(buffer);
            buffer = SDL_CreateRGBSurface(SURFACE_FLAGS, g_width, g_height, pix_fmt->BitsPerPixel,
                                          pix_fmt->Rmask, pix_fmt->Gmask, pix_fmt->Bmask, pix_fmt->Amask);
            draw_surface = buffer;
        } else {
            draw_surface = screen;
        }
        video_update = video_update_sdl;
    }
}


void video_resize(int w, int h)
{
    video_set_mode(w, h);
}


void video_update_line(int pos)
{
    if (g_use_gl) {
        SDL_LockSurface(buffer);
        glBindTexture(GL_TEXTURE_2D, textures[pos / TEXTURE_WIDTH]);
        // in the texture, the column is represented as one row!
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, pos % TEXTURE_WIDTH, g_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, buffer->pixels);
        SDL_UnlockSurface(buffer);
    }
}


static inline void video_draw_quad(int x, GLuint tex) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glBegin(GL_QUADS);
    // x and y texture coordinates are swapped
    glTexCoord2f(0.0f,        0.0f); glVertex2i(x,                 0);
    glTexCoord2f(0.0f,        1.0f); glVertex2i(x + TEXTURE_WIDTH, 0);
    glTexCoord2f(tex_coord_h, 1.0f); glVertex2i(x + TEXTURE_WIDTH, g_height);
    glTexCoord2f(tex_coord_h, 0.0f); glVertex2i(x,                 g_height);
    glEnd();
}


static void video_update_gl(int pos, int prev_pos)
{
    (void)prev_pos;

    glEnable(GL_TEXTURE_2D);

    glColor3f(1.0f, 1.0f, 1.0f);

    if (g_scrolling)
    {
        // this texture needs to be drawn twice
        int ntex = pos / TEXTURE_WIDTH;
        video_draw_quad((ntex * TEXTURE_WIDTH) - pos, textures[ntex]);
        // now start with last quad, this way the last one overlapping the first is not an issue
        for (int n = num_textures - 1; n >= 0; n--) {
            video_draw_quad((g_width - pos + n * TEXTURE_WIDTH) % g_width, textures[n]);
        }
    }
    else
    {
        for (int n = 0; n < num_textures; n++) {
            video_draw_quad(n * TEXTURE_WIDTH, textures[n]);
        }
    }

    glDisable(GL_TEXTURE_2D);
}


static void video_update_sdl(int pos, int prev_pos)
{
    if (g_scrolling)
    {
        /*          pos
        *  +--------+--------------------+
        *  | r_src1 |       r_src2       |  buffer
        *  +--------+--------------------+
        *
        *  +--------------------+--------+
        *  |      r_dst2        | r_dst1 |  screen
        *  +--------------------+--------+
        */

        // left part of source surface first
        SDL_Rect r_src1 = { 0, 0, pos, g_height };
        SDL_Rect r_dst1 = { g_width - pos, 0, 0, 0 };
        SDL_BlitSurface(buffer, &r_src1, screen, &r_dst1);
        // now the second part
        SDL_Rect r_src2 = { pos, 0, g_width - pos, g_height };
        SDL_Rect r_dst2 = { 0, 0, 0, 0 };
        SDL_BlitSurface(buffer, &r_src2, screen, &r_dst2);

        // need to update whole window
        update_rects[0].rect.x = update_rects[0].rect.y = update_rects[0].rect.w = update_rects[0].rect.h = 0;
        update_rects[0].use = true;
    }
    else
    {
        // no need to blit, since we've already drawn directly to the screen surface

        // find out which portions of the screen to update
        if (pos >= prev_pos)
        {
            update_rects[0].rect.x = prev_pos;
            update_rects[0].rect.w = min(pos + 4 - prev_pos, g_width - prev_pos);
            if (pos > g_width - 2) {
                update_rects[1].rect.x = 0;
                update_rects[1].rect.w = 2;
                update_rects[1].use = true;
            }
        }
        else
        {
            update_rects[0].rect.x = 0;
            update_rects[0].rect.w = pos + 4;
            update_rects[0].use = true;
            update_rects[1].rect.x = prev_pos;
            update_rects[1].rect.w = g_width - prev_pos;
            update_rects[1].use = true;
        }
        update_rects[0].use = true;
        update_rects[0].rect.y = update_rects[1].rect.y = 0;
        update_rects[0].rect.h = update_rects[1].rect.h = g_height;
    }
}


void video_flip()
{
    while (SDL_GetTicks() < ticks + g_ticks_per_frame) {
        SDL_Delay(1);
    }
    ticks = SDL_GetTicks();

    if (g_use_gl)
    {
        SDL_GL_SwapBuffers();
    }
    else
    {
        for (int n = 0; n < 2; n++) {
            if (update_rects[n].use) {
                SDL_UpdateRect(screen, update_rects[n].rect.x, update_rects[n].rect.y,
                                       update_rects[n].rect.w, update_rects[n].rect.h);
                update_rects[n].use = false;
            }
        }
    }
}


SDL_Surface *video_get_screen()
{
    return screen;
}


SDL_Surface *video_get_draw_surface()
{
    return draw_surface;
}


SDL_PixelFormat *video_get_pix_fmt()
{
    return pix_fmt;
}
