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
#include "game_type_select.h"
#include "ui_mgr.h"
#include "ui_theme.h"
#include "button.h"
#include "loop.h"
#include "render_util.h"
#include "audio.h"
#include "gl_util.h"
#include "keyboard.h"
#include "multiplayer.h"
#include "ui_snow.h"
#include "joystick.h"

static button_t *enter_event_btn = NULL;
static button_t *practice_btn = NULL;
static button_t *credits_btn = NULL;
static button_t *quit_btn = NULL;

void enter_event_click_cb(button_t* button, void *userdata)
{
    check_assertion(userdata == NULL, "userdata is not null");
    g_game.current_event = NULL;
    g_game.current_cup = NULL;
    g_game.current_race = -1;
    g_game.practicing = False;
    set_game_mode(EVENT_SELECT);
    ui_set_dirty();
}

void practice_click_cb(button_t *button, void *userdata)
{
    check_assertion(userdata == NULL, "userdata is not null");
    g_game.current_event = NULL;
    g_game.current_cup = NULL;
    g_game.current_race = -1;
    g_game.practicing = True;
    set_game_mode(RACE_SELECT);
    ui_set_dirty();
}

void credits_click_cb(button_t *button, void *userdata)
{
    check_assertion(userdata == NULL, "userdata is not null");
    set_game_mode(CREDITS);
    ui_set_dirty();
}

void quit_click_cb(button_t *button, void *userdata)
{
    check_assertion(userdata == NULL, "userdata is not null");
    winsys_exit(0);
}

static void set_widget_positions()
{
    button_t** button_list[] = { &enter_event_btn, &practice_btn, &credits_btn, &quit_btn };
    int w = getparam_x_resolution();
    int h = getparam_y_resolution();
    int box_height;
    int box_max_y;
    int top_y;
    int bottom_y;
    int num_buttons = sizeof(button_list) / sizeof(button_list[0]);
    int i;
    int tot_button_height = 0;
    int button_sep =0;
    int cur_y_pos;

    box_height = 210;
    box_max_y = h - 128;
    bottom_y = 0.4 * h - box_height / 2;
    if (bottom_y + box_height > box_max_y) {
        bottom_y = box_max_y - box_height;
    }
    top_y = bottom_y + box_height;

    for (i = 0; i < num_buttons; i++) {
        tot_button_height += button_get_height(*button_list[i]);
    }
    if (num_buttons > 1) {
        button_sep = (top_y - bottom_y - tot_button_height) / (num_buttons - 1);
        button_sep = max(0, button_sep);
    }

    cur_y_pos = top_y;
    for (i = 0; i < num_buttons; i++) {
        cur_y_pos -= button_get_height(*button_list[i]);
        button_set_position(*button_list[i], make_point2d(w / 2.0 - button_get_width(*button_list[i]) / 2.0, cur_y_pos));
        cur_y_pos -= button_sep;
    }
}

static void game_type_select_init(void)
{
    point2d_t dummy_pos = {0, 0};

    winsys_set_display_func(main_loop);
    winsys_set_idle_func(main_loop);
    winsys_set_reshape_func(reshape);
    winsys_set_mouse_func(ui_event_mouse_func);
    winsys_set_motion_func(ui_event_motion_func);
    winsys_set_passive_motion_func(ui_event_motion_func);

    enter_event_btn = button_create(dummy_pos, 300, 40, "button_label", "Enter an event");
    button_set_hilit_font_binding(enter_event_btn, "button_label_hilit");
    button_set_visible(enter_event_btn, True);
    button_set_click_event_cb(enter_event_btn, enter_event_click_cb, NULL);

    practice_btn = button_create(dummy_pos, 300, 40, "button_label", "Practice");
    button_set_hilit_font_binding(practice_btn, "button_label_hilit");
    button_set_visible(practice_btn, True);
    button_set_click_event_cb(practice_btn, practice_click_cb, NULL);

    credits_btn = button_create(dummy_pos, 300, 40, "button_label", "Credits");
    button_set_hilit_font_binding(credits_btn, "button_label_hilit");
    button_set_visible(credits_btn, True);
    button_set_click_event_cb(credits_btn, credits_click_cb, NULL);

    quit_btn = button_create(dummy_pos, 300, 40, "button_label", "Quit");
    button_set_hilit_font_binding(quit_btn, "button_label_hilit");
    button_set_visible(quit_btn, True);
    button_set_click_event_cb(quit_btn, quit_click_cb, NULL);
    play_music("start_screen");
}

static void game_type_select_loop(scalar_t time_step)
{
    check_gl_error();
    update_audio();
    set_gl_options(GUI);
    clear_rendering_context();
    set_widget_positions();
    ui_setup_display();

    if (getparam_ui_snow()) {
        update_ui_snow(time_step, False);
        draw_ui_snow();
    }
    ui_draw_menu_decorations();
    ui_draw();
    reshape(getparam_x_resolution(), getparam_y_resolution());
    winsys_swap_buffers();
}

static void game_type_select_term(void)
{
    button_delete(enter_event_btn);
    enter_event_btn = NULL;

    button_delete(practice_btn);
    practice_btn = NULL;

    button_delete(credits_btn);
    credits_btn = NULL;

    button_delete(quit_btn);
    quit_btn = NULL;
}

START_KEYBOARD_CB(game_type_select_cb)
{
    if (release) return;

    if (!special) {
        key = (int)tolower((char)key);

        switch(key) {
        case 'q':
        case 27: /* Esc */
            winsys_exit(0);
            break;
        case 'e':
        case 13: /* Enter */
            if (enter_event_btn) {
                button_simulate_mouse_click(enter_event_btn);
            }
            break;
        case 'p':
            if (practice_btn) {
                button_simulate_mouse_click(practice_btn);
            }
            break;
        case 'c':
            if (credits_btn) {
                button_simulate_mouse_click(credits_btn);
            }
            break;
        }
    }
    winsys_post_redisplay();
}
END_KEYBOARD_CB

void game_type_select_register()
{
    int status = 0;
    status |= add_keymap_entry(GAME_TYPE_SELECT, DEFAULT_CALLBACK, NULL, NULL, game_type_select_cb);
    register_loop_funcs(GAME_TYPE_SELECT, game_type_select_init, game_type_select_loop, game_type_select_term);
}

/* EOF */