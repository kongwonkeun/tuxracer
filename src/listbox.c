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
#include "listbox.h"
#include "fonts.h"
#include "ui_mgr.h"
#include "ui_theme.h"
#include "render_util.h"
#include "button.h"

#define DEFAULT_ARROW_BUTTON_HEIGHT 15
#define DEFAULT_ARROW_BUTTON_WIDTH  27
#define DEFAULT_ARROW_REGION_WIDTH  36
#define DEFAULT_ARROW_VERT_SEPARATION  4
#define DEFAULT_BORDER_WIDTH 4
#define DEFAULT_TEXT_PAD 8

struct listbox_ {
    point2d_t pos;
    scalar_t w;
    scalar_t h;
    scalar_t arrow_width;
    scalar_t border_width;
    scalar_t text_pad;
    scalar_t arrow_vert_separation;
    char *font_binding;
    colour_t border_colour;
    colour_t background_colour;
    button_t *up_button;
    button_t *down_button;
    listbox_item_change_event_cb_t item_change_cb;
    void *item_change_cb_userdata;
    list_t item_list;
    list_elem_t cur_item;
    bool_t visible;
    bool_t active;

    listbox_list_elem_to_string_fptr_t label_gen_func;
};

char * listbox_default_list_elem_to_string_func( list_elem_data_t data )
{
    return (char*) data;
}

static void update_button_enabled_states( listbox_t *listbox )
{
    check_assertion( listbox != NULL, "listbox is NULL" );

    if ( listbox->cur_item == NULL ) {
        /* No items in list */
        button_set_enabled( listbox->up_button, False );
        button_set_enabled( listbox->down_button, False );
    } else {
        if ( listbox->cur_item == get_list_head( listbox->item_list ) ) {
            button_set_enabled( listbox->up_button, False );
        } else {
            button_set_enabled( listbox->up_button, True );
        }

        if ( listbox->cur_item == get_list_tail( listbox->item_list ) ) {
            button_set_enabled( listbox->down_button, False );
        } else {
            button_set_enabled( listbox->down_button, True );
        }
    }
}

static void update_button_positions( listbox_t *listbox )
{
    check_assertion( listbox != NULL, "listbox is NULL" );

    button_set_position( listbox->up_button, 
        make_point2d( listbox->pos.x + listbox->w - button_get_width( listbox->up_button ),
            listbox->pos.y + listbox->h / 2.0 + listbox->arrow_vert_separation / 2.0 
        ) 
    );

    button_set_position( 
        listbox->down_button,
        make_point2d( listbox->pos.x + listbox->w -  button_get_width( listbox->down_button ),
            listbox->pos.y + listbox->h / 2.0 - listbox->arrow_vert_separation / 2.0 - button_get_height( listbox->up_button ) 
        ) 
    );
}

static void listbox_arrow_click_cb( button_t *button, void* userdata )
{
    listbox_t *listbox = (listbox_t*)userdata;

    check_assertion( listbox != NULL, "listbox is NULL" );

    if ( button == listbox->up_button ) {
        listbox->cur_item = get_prev_list_elem( listbox->item_list, listbox->cur_item );
    } else if ( button == listbox->down_button ) {
        listbox->cur_item = get_next_list_elem( listbox->item_list, listbox->cur_item );
    } else {
        check_assertion( 0, "listbox arrow click callback called with unknown button" );
    }

    if ( listbox->item_change_cb != NULL ) {
        listbox->item_change_cb( listbox, listbox->item_change_cb_userdata );
    }
    update_button_enabled_states( listbox );
    ui_set_dirty();
}

bool_t listbox_goto_next_item( listbox_t *listbox )
{
    check_assertion( listbox != NULL, "listbox is NULL" );

    if ( listbox->cur_item == get_list_tail( listbox->item_list ) ) {
        return False;
    }
    button_simulate_mouse_click( listbox->down_button );
    return True;
}

bool_t listbox_goto_prev_item( listbox_t *listbox )
{
    check_assertion( listbox != NULL, "listbox is NULL" );

    if ( listbox->cur_item == get_list_head( listbox->item_list ) ) {
        return False;
    }
    button_simulate_mouse_click( listbox->up_button );
    return True;
}

void listbox_draw( listbox_t *listbox )
{
    font_t *font;

    check_assertion( listbox != NULL, "listbox is NULL" );
    glDisable( GL_TEXTURE_2D );
    glColor3dv( (scalar_t*)&listbox->border_colour );
    glRectf( listbox->pos.x, listbox->pos.y, listbox->pos.x + listbox->w - listbox->arrow_width, listbox->pos.y + listbox->h );
    glColor3dv( (scalar_t*)&listbox->background_colour );
    glRectf( listbox->pos.x + listbox->border_width, 
        listbox->pos.y + listbox->border_width,
        listbox->pos.x + listbox->w - listbox->border_width -
        listbox->arrow_width,
        listbox->pos.y + listbox->h - listbox->border_width 
    );

    glEnable( GL_TEXTURE_2D );

    if ( !get_font_binding( listbox->font_binding, &font ) ) {
        print_warning( IMPORTANT_WARNING, "Couldn't get font object for binding %s", listbox->font_binding );
    } else {
        int w, asc, desc;
        char *string;

        string = listbox->label_gen_func( get_list_elem_data( listbox->cur_item ) );
        get_font_metrics( font, string, &w, &asc, &desc );
        bind_font_texture( font );
        glColor3f( 1.0, 1.0, 1.0 );

        glPushMatrix();
        {
            glTranslatef( listbox->pos.x + listbox->border_width + listbox->text_pad, listbox->pos.y + listbox->h/2.0 - asc/2.0 + desc/2.0, 0 );
            draw_string( font, string );
        }
        glPopMatrix();
    }

    button_draw( listbox->up_button );
    button_draw( listbox->down_button );
}

static void listbox_draw_cb( void *widget )
{
    check_assertion( widget != NULL, "widget is NULL" );
    listbox_draw( (listbox_t*) widget );
}

listbox_t* listbox_create( point2d_t pos, scalar_t w, scalar_t h, char *font_binding, list_t item_list, listbox_list_elem_to_string_fptr_t func )
{
    listbox_t *listbox;
    char *binding;
    point2d_t ll;
    point2d_t ur;

    listbox = (listbox_t*)malloc( sizeof(listbox_t) );

    check_assertion( listbox != NULL, "out of memory" );

    listbox->pos = pos;
    listbox->w = w;
    listbox->h = h;
    listbox->arrow_width = DEFAULT_ARROW_REGION_WIDTH;
    listbox->border_width = DEFAULT_BORDER_WIDTH;
    listbox->text_pad = DEFAULT_TEXT_PAD;
    listbox->arrow_vert_separation = DEFAULT_ARROW_VERT_SEPARATION;
    listbox->font_binding = font_binding;
    listbox->border_colour = ui_foreground_colour;
    listbox->background_colour = ui_background_colour; 

    /* Create up arrow button */
    listbox->up_button = button_create( 
        make_point2d( 0, 0 ), /* position will be set later */
        DEFAULT_ARROW_BUTTON_WIDTH,
        DEFAULT_ARROW_BUTTON_HEIGHT,
        NULL,
        NULL 
    );

    binding = "listbox_arrows";

    ll = make_point2d( 2.0/64.0, 17.0/64.0 );
    ur = make_point2d( 29.0/64.0, 32.0/64.0 );
    button_set_image( listbox->up_button, binding, ll, ur, white );

    ll = make_point2d( 34.0/64.0, 17.0/64.0 );
    ur = make_point2d( 61.0/64.0, 32.0/64.0 );
    button_set_disabled_image( listbox->up_button, binding, ll, ur, white );

    ll = make_point2d( 34.0/64.0, 49.0/64.0 );
    ur = make_point2d( 61.0/64.0, 64.0/64.0 );
    button_set_hilit_image( listbox->up_button, binding, ll, ur, white );

    ll = make_point2d( 2.0/64.0, 49.0/64.0 );
    ur = make_point2d( 29.0/64.0, 64.0/64.0 );
    button_set_clicked_image( listbox->up_button, binding, ll, ur, white );


    button_set_click_event_cb( listbox->up_button, listbox_arrow_click_cb, listbox );

    /* Create down arrow button */
    listbox->down_button = button_create( 
        make_point2d( 0, 0 ), /* position will be set later */
        DEFAULT_ARROW_BUTTON_WIDTH,
        DEFAULT_ARROW_BUTTON_HEIGHT,
        NULL,
        NULL 
    );

    binding = "listbox_arrows";

    ll = make_point2d( 2.0/64.0, 1.0/64.0 );
    ur = make_point2d( 29.0/64.0, 16.0/64.0 );
    button_set_image( listbox->down_button, binding, ll, ur, white );

    ll = make_point2d( 34.0/64.0, 1.0/64.0 );
    ur = make_point2d( 61.0/64.0, 16.0/64.0 );
    button_set_disabled_image( listbox->down_button, binding, ll, ur, white );

    ll = make_point2d( 34.0/64.0, 33.0/64.0 );
    ur = make_point2d( 61.0/64.0, 48.0/64.0 );
    button_set_hilit_image( listbox->down_button, binding, ll, ur, white );

    ll = make_point2d( 2.0/64.0, 33.0/64.0 );
    ur = make_point2d( 29.0/64.0, 48.0/64.0 );
    button_set_clicked_image( listbox->down_button, binding, ll, ur, white );

    button_set_click_event_cb( listbox->down_button, listbox_arrow_click_cb, listbox );

    button_set_click_event_cb( listbox->down_button, listbox_arrow_click_cb, listbox );

    listbox->item_change_cb = NULL;
    listbox->item_change_cb_userdata = NULL;

    listbox->item_list = item_list;
    listbox->cur_item = get_list_head( listbox->item_list );

    listbox->label_gen_func = func;

    listbox->visible = False;
    listbox->active = False;

    update_button_enabled_states( listbox );

    update_button_positions( listbox );

    return listbox;
}

void listbox_delete( listbox_t *listbox )
{
    check_assertion( listbox != NULL, "listbox is NULL" );

    listbox_set_visible( listbox, False );
    listbox_set_active( listbox, False );

    button_delete( listbox->up_button );
    button_delete( listbox->down_button );

    free(listbox);
}

void listbox_set_item_change_event_cb( listbox_t *listbox, listbox_item_change_event_cb_t cb, void *userdata )
{
    check_assertion( listbox != NULL, "listbox is NULL" );

    listbox->item_change_cb = cb;
    listbox->item_change_cb_userdata = userdata;
}

void listbox_set_item_list( listbox_t *listbox, list_t item_list, listbox_list_elem_to_string_fptr_t func )
{
    check_assertion( listbox != NULL, "listbox is NULL" );

    listbox->item_list = item_list;
    listbox->cur_item = get_list_head(item_list);
    listbox->label_gen_func = func;

    update_button_enabled_states( listbox );
    ui_set_dirty();
}

list_elem_t listbox_get_current_item( listbox_t *listbox )
{
    check_assertion( listbox != NULL, "listbox is NULL" );
    return listbox->cur_item;
}

void listbox_set_current_item( listbox_t *listbox, list_elem_t item )
{
    check_assertion( listbox != NULL, "listbox is NULL" );
    listbox->cur_item = item;
    update_button_enabled_states( listbox );
}

void listbox_set_position( listbox_t *listbox, point2d_t pos )
{
    check_assertion( listbox != NULL, "listbox is NULL" );
    listbox->pos = pos;
    update_button_positions( listbox );
}

scalar_t listbox_get_width( listbox_t *listbox )
{
    check_assertion( listbox != NULL, "listbox is NULL" );
    return listbox->w;
}

scalar_t listbox_get_height( listbox_t *listbox )
{
    check_assertion( listbox != NULL, "listbox is NULL" );
    return listbox->h;
}

void listbox_set_visible( listbox_t *listbox, bool_t visible )
{
    check_assertion( listbox != NULL, "listbox is NULL" );

    /* This check is necessary to prevent infinite mutual recursion */
    if ( listbox->active != visible ) {
        listbox_set_active( listbox, visible );
    }

    if ( !listbox->visible && visible ) {
        ui_add_widget_draw_callback( listbox, listbox_draw_cb );

        ui_set_dirty();
    } else if ( listbox->visible && !visible ) {
        ui_delete_widget_draw_callback( listbox );
        ui_set_dirty();
    }
    listbox->visible = visible;
}

bool_t listbox_is_visible( listbox_t *listbox )
{
    check_assertion( listbox != NULL, "listbox is NULL" );
    return listbox->visible;
}

void listbox_set_active( listbox_t *listbox, bool_t active )
{
    check_assertion( listbox != NULL, "listbox is NULL" );

    button_set_active( listbox->up_button, active );
    button_set_active( listbox->down_button, active );
    listbox->active = active;
    if ( !active ) {
        listbox_set_visible( listbox, False );
    }

}

bool_t listbox_is_active( listbox_t *listbox )
{
    check_assertion( listbox != NULL, "listbox is NULL" );
    return listbox->active;
}

/* EOF */