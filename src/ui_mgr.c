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
#include "ui_mgr.h"
#include "hash.h"
#include "textures.h"
#include "ui_snow.h"
#include "loop.h"

typedef struct {
    void *widget;
    void *cb;
}
ui_callback_data_t;

static hash_table_t mouse_motion_cbs;
static hash_table_t mouse_down_cbs;
static hash_table_t mouse_up_cbs;
static hash_table_t widget_draw_cbs;
static bool_t initialized = False;
static bool_t needs_redraw = True;
static char key_buffer[12]; /* enough to hold a pointer in hex */
static point2d_t cursor_pos = { 0, 0 };
static bool_t left_mouse_button_down = False;
static bool_t middle_mouse_button_down = False;
static bool_t right_mouse_button_down = False;

#define OFFSET_AMT 0.0
#define CURSOR_TEX_SIZE 32

void init_ui_manager()
{
    if (!initialized) {
        mouse_motion_cbs = create_hash_table();
        mouse_down_cbs = create_hash_table();
        mouse_up_cbs = create_hash_table();
        widget_draw_cbs = create_hash_table();
    }
    initialized = True;
}

void ui_setup_display()
{
    scalar_t offset = OFFSET_AMT;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, getparam_x_resolution(), 0, getparam_y_resolution(), -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(offset, offset, -1.0);
    glColor4f(1.0, 1.0, 1.0, 1.0);
}

static char* generate_key_from_pointer(void *ptr)
{
    sprintf(key_buffer, "%p", ptr);
    return key_buffer;
}

static ui_callback_data_t* generate_cb_data(void *widget, void *cb)
{
    ui_callback_data_t *cb_data;

    cb_data = (ui_callback_data_t*)malloc(sizeof(ui_callback_data_t));
    if (cb_data != NULL) {
        cb_data->widget = widget;
        cb_data->cb = cb;
    }
    return cb_data;
}

void ui_add_mouse_motion_callback(void* widget, mouse_motion_event_cb_t cb) 
{
    char *key;
    hash_table_t table;
    ui_callback_data_t *cb_data;

    key = generate_key_from_pointer(widget);
    table = mouse_motion_cbs;

    if (get_hash_entry(table, key, NULL)) {
        del_hash_entry(table, key, (hash_entry_t*)&cb_data);
        free(cb_data);
    }
    if (cb != NULL) {
        add_hash_entry(table, key, generate_cb_data( widget, (void*)cb));
    }
}

void ui_delete_mouse_motion_callback(void* widget) 
{
    ui_add_mouse_motion_callback(widget, NULL);
}

void ui_add_mouse_down_callback(void* widget, mouse_button_event_cb_t cb) 
{
    char *key;
    hash_table_t table;
    ui_callback_data_t *cb_data;

    key = generate_key_from_pointer(widget);
    table = mouse_down_cbs;

    if (get_hash_entry(table, key, NULL)) {
        del_hash_entry(table, key, (hash_entry_t*)&cb_data);
        free(cb_data);
    }
    if (cb != NULL) {
        add_hash_entry(table, key, generate_cb_data(widget, (void*)cb));
    }
}

void ui_delete_mouse_down_callback(void* widget) 
{
    ui_add_mouse_down_callback(widget, NULL);
}

void ui_add_mouse_up_callback(void* widget, mouse_button_event_cb_t cb) 
{
    char *key;
    hash_table_t table;
    ui_callback_data_t *cb_data;

    key = generate_key_from_pointer(widget);
    table = mouse_up_cbs;

    if (get_hash_entry(table, key, NULL)) {
        del_hash_entry(table, key, (hash_entry_t*)&cb_data);
        free(cb_data);
    }
    if (cb != NULL) {
        add_hash_entry(table, key, generate_cb_data(widget, (void*)cb));
    }
}

void ui_delete_mouse_up_callback(void* widget) 
{
    ui_add_mouse_up_callback(widget, NULL);
}

void ui_add_widget_draw_callback(void* widget, widget_draw_cb_t cb)
{
    char *key;
    hash_table_t table;
    ui_callback_data_t *cb_data;

    key = generate_key_from_pointer(widget);
    table = widget_draw_cbs;

    if (get_hash_entry(table, key, NULL)) {
        del_hash_entry(table, key, (hash_entry_t*)&cb_data);
        free(cb_data);
    }
    if (cb != NULL) {
        add_hash_entry(table, key, generate_cb_data(widget, (void*)cb));
    }
}

void ui_delete_widget_draw_callback(void* widget)
{
    ui_add_widget_draw_callback(widget, NULL);
}

void ui_set_dirty()
{
    needs_redraw = True;
}

void ui_check_dirty()
{
    if (needs_redraw) {
        winsys_post_redisplay();
        needs_redraw = False;
    }
}

static void trigger_mouse_button_cbs(hash_table_t table, winsys_mouse_button_t button, int x, int y)
{
    hash_search_t iter;
    ui_callback_data_t *cb_data;

    begin_hash_scan(table, &iter);
    while (next_hash_entry(iter, NULL, (hash_entry_t*)&cb_data)) {
        ((mouse_button_event_cb_t)cb_data->cb)(cb_data->widget, (winsys_mouse_button_t)button, x, y);
        if (is_mode_change_pending()) {
            /* Callback just changed the mode; stop handling events for this mode. */
            break;
        }
    }
    end_hash_scan(iter);
}

static void trigger_mouse_motion_cbs(hash_table_t table, int x, int y)
{
    hash_search_t iter;
    ui_callback_data_t *cb_data;

    begin_hash_scan(table, &iter);
    while (next_hash_entry(iter, NULL, (hash_entry_t*)&cb_data)) {
        ((mouse_motion_event_cb_t)cb_data->cb)(cb_data->widget, x, y);
        if (is_mode_change_pending()) {
            /* Callback just changed the mode; stop handling events for this mode. */
            break;
        }
    }
    end_hash_scan(iter);
}

void ui_event_mouse_func(int button, int state, int x, int y)
{
    if (is_mode_change_pending()) {
        /* Don't process events until mode change occurs */
        return;
    }
    /* Reverse y coordinate */
    y = getparam_y_resolution() - y;

    if (state == WS_MOUSE_DOWN) {
        trigger_mouse_button_cbs(mouse_down_cbs, button, x, y);
    } else {
        trigger_mouse_button_cbs(mouse_up_cbs, button, x, y);
    }
    if (button == WS_LEFT_BUTTON) {
        left_mouse_button_down = (bool_t)(state == WS_MOUSE_DOWN);
    }
    if (button == WS_MIDDLE_BUTTON) {
        middle_mouse_button_down = (bool_t)( state == WS_MOUSE_DOWN);
    }
    if (button == WS_RIGHT_BUTTON) {
        right_mouse_button_down = (bool_t)( state == WS_MOUSE_DOWN);
    }
    ui_check_dirty();
}

void ui_event_motion_func(int x, int y)
{
    point2d_t old_pos;

    if (is_mode_change_pending()) {
        /* don't process events until mode change occurs */
        return;
    }
    /* reverse y coordinate */
    y = getparam_y_resolution() - y;

    trigger_mouse_motion_cbs(mouse_motion_cbs, x, y);
    old_pos = cursor_pos;
    cursor_pos = make_point2d(x, y);

    if (old_pos.x != x || old_pos.y != y) {
        /* update UI snow */
        if (getparam_ui_snow()) {
            if (right_mouse_button_down) {
                make_ui_snow(cursor_pos);
                reset_ui_snow_cursor_pos(cursor_pos);
            }
            else if (middle_mouse_button_down) {
                make_ui_snow(cursor_pos);
                push_ui_snow(cursor_pos);
            }
            else {
                push_ui_snow(cursor_pos);
            }
        }
        /* need to redraw cursor */
        ui_set_dirty();
        ui_check_dirty();
    }
}

static void ui_draw_cursor(void)
{
    GLuint texobj;
    char *binding;

    binding = "mouse_cursor";
    if (!get_texture_binding(binding, &texobj)) {
        texobj = 0;
    }
    ui_setup_display();
    glBindTexture(GL_TEXTURE_2D, texobj);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(cursor_pos.x, cursor_pos.y);
    glTexCoord2f(0, 0); glVertex2f(cursor_pos.x, cursor_pos.y - CURSOR_TEX_SIZE);
    glTexCoord2f(1, 0); glVertex2f(cursor_pos.x + CURSOR_TEX_SIZE, cursor_pos.y - CURSOR_TEX_SIZE);
    glTexCoord2f(1, 1); glVertex2f(cursor_pos.x + CURSOR_TEX_SIZE, cursor_pos.y);
    glEnd();
}

void ui_draw(void)
{
    hash_search_t iter;
    ui_callback_data_t *cb_data;

    ui_setup_display();
    begin_hash_scan(widget_draw_cbs, &iter);

    while (next_hash_entry(iter, NULL, (hash_entry_t*)&cb_data)) {
        ((widget_draw_cb_t)cb_data->cb)(cb_data->widget);
        check_assertion(!is_mode_change_pending(), "widget draw callback changed the mode");
    }
    end_hash_scan(iter);
    ui_draw_cursor();
}

point2d_t ui_get_mouse_position(void)
{
    return cursor_pos;
}

/* EOF */