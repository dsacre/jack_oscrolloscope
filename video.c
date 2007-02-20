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

#include <SDL.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "main.h"
#include "video.h"

static const int TEXTURE_WIDTH = 64;


static void video_exit();

void (*video_update)(int, int);
static void video_update_gl(int, int);
static void video_update_sdl(int, int);


int video_ticks_per_frame = 1000 / DEFAULT_FPS;
static int ticks = 0;

bool video_scrolling = true;

bool video_use_gl = false;
int  video_width = DEFAULT_WIDTH;
int  video_height = 0;

SDL_Surface *video_screen = NULL;
SDL_Surface *video_buffer = NULL;
SDL_Surface *video_draw_surface = NULL;

SDL_PixelFormat *video_pix_fmt = NULL;
update_rect video_updates[2] = { { false } };

static GLuint *textures = NULL;
static int    num_textures = 0;
static float  tex_coord_h;
static int    max_texture_size = 0;

static inline int next_power_of_two(int i) {
    int r = 1;
    while (r < i) r *= 2;
    return r;
}


void video_init()
{
    if (video_use_gl)
    {
        if (video_ticks_per_frame == 0) {
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

    video_set_mode(video_width, video_height);

    atexit(video_exit);
}


static void video_exit()
{
    if (video_use_gl)
    {
        glDeleteTextures(num_textures, textures);
        SDL_FreeSurface(video_buffer);
        free(textures);
    }
    else
    {
        if (video_scrolling) SDL_FreeSurface(video_buffer);
    }
}


void video_set_mode(int w, int h)
{
    if (video_use_gl && max_texture_size != 0 && h > max_texture_size) {
        fprintf(stderr, "maximum OpenGL texture size exceeded\n");
        h = max_texture_size;
    }

    video_width = w;
    video_height = h;

    if ((video_screen = SDL_SetVideoMode(video_width, video_height, VIDEO_BPP,
                                (video_use_gl ? VIDEO_FLAGS_GL : VIDEO_FLAGS_SDL))) == NULL)
    {
        fprintf(stderr, "can't set video mode: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    if (video_use_gl)
    {
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

        if (textures) glDeleteTextures(num_textures, textures);

        num_textures = (int)ceilf((float)video_width / (float)TEXTURE_WIDTH);
        textures = realloc(textures, num_textures * sizeof(GLuint));
        glGenTextures(num_textures, textures);

        int tex_w = TEXTURE_WIDTH;
        int tex_h = next_power_of_two(video_height);
        tex_coord_h = (float)video_height / (float)tex_h;

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

        glViewport(0, 0, video_width, video_height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0f, (GLdouble)video_width, (GLdouble)video_height, 0.0f, -1.0f, 1.0f);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        if (video_buffer) SDL_FreeSurface(video_buffer);

        // give OpenGL the pixel format it expects
        video_buffer = SDL_CreateRGBSurface(SDL_SWSURFACE, 1, video_height, 32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                            0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
#else
                                            0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
#endif
        video_pix_fmt = video_buffer->format;

        video_draw_surface = video_buffer;
        video_update = video_update_gl;
    }
    else // SDL
    {
        video_pix_fmt = video_screen->format;

        if (video_scrolling) {
            if (video_buffer) SDL_FreeSurface(video_buffer);
            video_buffer = SDL_CreateRGBSurface(SURFACE_FLAGS, video_width, video_height, video_pix_fmt->BitsPerPixel,
                                video_pix_fmt->Rmask, video_pix_fmt->Gmask, video_pix_fmt->Bmask, video_pix_fmt->Amask);
            video_draw_surface = video_buffer;
        } else {
            video_draw_surface = video_screen;
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
    if (video_use_gl) {
        SDL_LockSurface(video_buffer);
        glBindTexture(GL_TEXTURE_2D, textures[pos / TEXTURE_WIDTH]);
        // in the texture, the column is represented as one row!
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, pos % TEXTURE_WIDTH, video_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, video_buffer->pixels);
        SDL_UnlockSurface(video_buffer);
    }
}


static void video_update_gl(int pos, int prev_pos)
{
    void inline draw_quad(int x, GLuint tex) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glBegin(GL_QUADS);
        // x and y texture coordinates are swapped
        glTexCoord2f(0.0f,        0.0f); glVertex2i(x,                 0);
        glTexCoord2f(0.0f,        1.0f); glVertex2i(x + TEXTURE_WIDTH, 0);
        glTexCoord2f(tex_coord_h, 1.0f); glVertex2i(x + TEXTURE_WIDTH, video_height);
        glTexCoord2f(tex_coord_h, 0.0f); glVertex2i(x,                 video_height);
        glEnd();
    }

    glEnable(GL_TEXTURE_2D);

    glColor3f(1.0f, 1.0f, 1.0f);

    if (video_scrolling)
    {
        // this texture needs to be drawn twice
        int ntex = pos / TEXTURE_WIDTH;
        draw_quad((ntex * TEXTURE_WIDTH) - pos, textures[ntex]);
        // now start with last quad, this way the last one overlapping the first is not an issue
        for (int n = num_textures - 1; n >= 0; n--) {
            draw_quad((video_width - pos + n * TEXTURE_WIDTH) % video_width, textures[n]);
        }
    }
    else
    {
        for (int n = 0; n < num_textures; n++) {
            draw_quad(n * TEXTURE_WIDTH, textures[n]);
        }
    }

    glDisable(GL_TEXTURE_2D);
}


static void video_update_sdl(int pos, int prev_pos)
{
    if (video_scrolling)
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
        SDL_Rect r_src1 = { 0, 0, pos, video_height };
        SDL_Rect r_dst1 = { video_width - pos, 0, 0, 0 };
        SDL_BlitSurface(video_buffer, &r_src1, video_screen, &r_dst1);
        // now the second part
        SDL_Rect r_src2 = { pos, 0, video_width - pos, video_height };
        SDL_Rect r_dst2 = { 0, 0, 0, 0 };
        SDL_BlitSurface(video_buffer, &r_src2, video_screen, &r_dst2);

        // need to update whole window
        video_updates[0].rect.x = video_updates[0].rect.y = video_updates[0].rect.w = video_updates[0].rect.h = 0;
        video_updates[0].use = true;
    }
    else
    {
        // no need to blit, since we've already drawn directly to the screen surface

        // find out which portions of the screen to update
        if (pos >= prev_pos)
        {
            video_updates[0].rect.x = prev_pos;
            video_updates[0].rect.w = min(pos + 4 - prev_pos, video_width - prev_pos);
            if (pos > video_width - 2) {
                video_updates[1].rect.x = 0;
                video_updates[1].rect.w = 2;
                video_updates[1].use = true;
            }
        }
        else
        {
            video_updates[0].rect.x = 0;
            video_updates[0].rect.w = pos + 4;
            video_updates[0].use = true;
            video_updates[1].rect.x = prev_pos;
            video_updates[1].rect.w = video_width - prev_pos;
            video_updates[1].use = true;
        }
        video_updates[0].use = true;
        video_updates[0].rect.y = video_updates[1].rect.y = 0;
        video_updates[0].rect.h = video_updates[1].rect.h = video_height;
    }
}


void video_flip()
{
    while (SDL_GetTicks() < ticks + video_ticks_per_frame) {
        SDL_Delay(1);
    }
    ticks = SDL_GetTicks();

    if (video_use_gl)
    {
        SDL_GL_SwapBuffers();
    }
    else
    {
        for (int n = 0; n < 2; n++) {
            if (video_updates[n].use) {
                SDL_UpdateRect(video_screen, video_updates[n].rect.x, video_updates[n].rect.y,
                                             video_updates[n].rect.w, video_updates[n].rect.h);
                video_updates[n].use = false;
            }
        }
    }
}
