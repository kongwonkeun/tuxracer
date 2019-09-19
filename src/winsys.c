/* 
 * Tux Racer 
 * Copyright (C) 1999-2001 Jasmin F. Patry
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "tuxracer.h"
#include "winsys.h"
#include "audio.h"

/* Windowing System Abstraction Layer */
/* abstracts creation of windows, handling of events, etc. */
#if defined(HAVE_SDL)
#if defined(HAVE_SDL_MIXER)
#   include "SDL_mixer.h"
#endif

static SDL_Surface *screen = NULL;

static winsys_display_func_t  display_func = NULL;
static winsys_idle_func_t     idle_func = NULL;
static winsys_reshape_func_t  reshape_func = NULL;
static winsys_keyboard_func_t keyboard_func = NULL;
static winsys_mouse_func_t    mouse_func = NULL;
static winsys_motion_func_t   motion_func = NULL;
static winsys_motion_func_t   passive_motion_func = NULL;
static winsys_atexit_func_t   atexit_func = NULL;

static bool_t redisplay = False;

void winsys_post_redisplay() { redisplay = True; }

void winsys_set_display_func(winsys_display_func_t func)       { display_func = func; }
void winsys_set_idle_func(winsys_idle_func_t func)             { idle_func = func; }
void winsys_set_reshape_func(winsys_reshape_func_t func)       { reshape_func = func; }
void winsys_set_keyboard_func(winsys_keyboard_func_t func)     { keyboard_func = func; }
void winsys_set_mouse_func(winsys_mouse_func_t func)           { mouse_func = func; }
void winsys_set_motion_func(winsys_motion_func_t func)         { motion_func = func; }
void winsys_set_passive_motion_func(winsys_motion_func_t func) { passive_motion_func = func; }

void winsys_swap_buffers()              { SDL_GL_SwapBuffers(); }
void winsys_warp_pointer(int x, int y)  { SDL_WarpMouse(x, y); }
void winsys_show_cursor(bool_t visible) { SDL_ShowCursor(visible); }
void winsys_shutdown()                  { SDL_Quit(); }

static void setup_sdl_video_mode()
{
    Uint32 video_flags = SDL_OPENGL; 
    int bpp = 0;
    int width, height;

    if (getparam_fullscreen()) {
        video_flags |= SDL_FULLSCREEN;
    } else {
        video_flags |= SDL_RESIZABLE;
    }

    switch (getparam_bpp_mode()) {
    case 0: /* use current bpp */
        bpp = 0;
        break;
    case 1: /* 16 bpp */
        bpp = 16;
        break;
    case 2: /* 32 bpp */
        bpp = 32;
        break;
    default:
        setparam_bpp_mode(0);
        bpp = getparam_bpp_mode();
    }

    width  = getparam_x_resolution();
    height = getparam_y_resolution();

    if ((screen = SDL_SetVideoMode(width, height, bpp, video_flags)) ==  NULL) {
        handle_system_error(1, "couldn't initialize video: %s", SDL_GetError());
    }
}

void winsys_init(int* argc, char** argv, char* window_title, char* icon_title)
{
    Uint32 sdl_flags = SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE;

    /* init SDL */
    if (SDL_Init(sdl_flags) < 0) {
        handle_error(1, "couldn't initialize SDL: %s", SDL_GetError());
    }

    /* init video */
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    setup_sdl_video_mode();
    SDL_WM_SetCaption(window_title, icon_title);
}

void winsys_enable_key_repeat(bool_t enabled)
{
    if (enabled) {
        SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    } else {
        SDL_EnableKeyRepeat(0, 0);
    }
}

void winsys_process_events()
{
    SDL_Event event; 
    unsigned int key;
    int x, y;

    while (True) {
        SDL_LockAudio();
        SDL_UnlockAudio();

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                if (keyboard_func) {
                    SDL_GetMouseState(&x, &y);
                    key = event.key.keysym.sym; 
                    (*keyboard_func)(key, key >= 256, False, x, y);
                }
                break;
            case SDL_KEYUP:
                if (keyboard_func) {
                    SDL_GetMouseState(&x, &y);
                    key = event.key.keysym.sym; 
                    (*keyboard_func)( key, key >= 256, True, x, y);
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if (mouse_func) {
                    (*mouse_func)(event.button.button, event.button.state, event.button.x, event.button.y);
                }
                break;
            case SDL_MOUSEMOTION:
                if (event.motion.state) { /* buttons are down */
                    if (motion_func) {
                        (*motion_func)(event.motion.x, event.motion.y);
                    }
                } else { /* no buttons are down */
                    if (passive_motion_func) {
                        (*passive_motion_func)(event.motion.x, event.motion.y);
                    }
                }
                break;
            case SDL_VIDEORESIZE:
                setup_sdl_video_mode();
                if (reshape_func) {
                    (*reshape_func)(event.resize.w, event.resize.h);
                }
                break;
            }
            SDL_LockAudio();
            SDL_UnlockAudio();
        }

        if (redisplay && display_func) {
            redisplay = False;
            (*display_func)();
        }
        else if (idle_func) {
            (*idle_func)();
        }
        /* delay for 1 ms. this allows the other threads to do some work (otherwise the audio thread gets starved) */
        SDL_Delay(1);
    }
    /* never exits */
    code_not_reached();
}

void winsys_atexit(winsys_atexit_func_t func)
{
    static bool_t called = False;
    check_assertion(called == False, "winsys_atexit called twice");
    called = True;
    atexit_func = func;
}

void winsys_exit(int code)
{
    if (atexit_func) {
        (*atexit_func)();
    }
    exit(code);
}

#endif /* defined(HAVE_SDL) */

/* EOF */