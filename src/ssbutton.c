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
#include "ssbutton.h"
#include "button.h"
#include "ui_mgr.h"
#include "ui_theme.h"

typedef struct {
    char *binding;   /* name of texture binding */
    colour_t colour; /* colour to use when drawing texture */
    point2d_t ll;    /* lower left */
    point2d_t ur;    /* upper right */
}
texture_region_t;

struct ssbutton_ {
    point2d_t pos;
    scalar_t w;
    scalar_t h;

    int num_states;
    texture_region_t *regions;

    int current_state;

    button_t *button;

    bool_t enabled;
    bool_t visible;
    bool_t active;
};

void ssbutton_draw(ssbutton_t *ssbutton)
{
    int i;

    check_assertion(ssbutton != NULL, "ssbutton is NULL");

    /* Set the image for the button to match the current state */
    i = ssbutton->current_state;
    button_set_image( 
        ssbutton->button, 
        ssbutton->regions[i].binding,
            ssbutton->regions[i].ll, 
            ssbutton->regions[i].ur, 
            ssbutton->regions[i].colour 
    );
    button_set_disabled_image( 
        ssbutton->button, 
            ssbutton->regions[i].binding,
            ssbutton->regions[i].ll, 
            ssbutton->regions[i].ur, 
            ui_disabled_colour 
    );
    button_draw( ssbutton->button );
}

static void ssbutton_draw_cb(void *widget)
{
    check_assertion(widget != NULL, "widget is NULL");
    ssbutton_draw((ssbutton_t*)widget);
}

static void ssbutton_button_click_event_cb(button_t *button, void *userdata)
{
    ssbutton_t *ssbutton;
    ssbutton = (ssbutton_t*)userdata;
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    ssbutton->current_state = (ssbutton->current_state + 1) % ssbutton->num_states;
}

void ssbutton_simulate_mouse_click(ssbutton_t *ssbutton)
{
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    button_simulate_mouse_click(ssbutton->button);
}

ssbutton_t* ssbutton_create(point2d_t pos, scalar_t w, scalar_t h, int num_states)
{
    ssbutton_t* ssbutton;
    int i;

    ssbutton = (ssbutton_t*)malloc(sizeof(ssbutton_t));
    check_assertion(ssbutton != NULL, "out of memory");
    ssbutton->pos = pos;
    ssbutton->w = w;
    ssbutton->h = h;
    ssbutton->num_states = num_states;
    ssbutton->regions = (texture_region_t*)malloc(sizeof(texture_region_t)*num_states);

    for (i = 0; i < num_states; i++) {
            ssbutton->regions[i].binding = NULL; //---- kong ---- ????
    }
    ssbutton->current_state = 0;
    ssbutton->button = button_create(pos, w, h, NULL, NULL);
    button_set_click_event_cb(ssbutton->button, ssbutton_button_click_event_cb, ssbutton);
    ssbutton->enabled = True;
    ssbutton->visible = False;
    ssbutton->active = False;
    return ssbutton;
}

void ssbutton_delete(ssbutton_t *ssbutton)
{
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    ssbutton_set_visible(ssbutton, False);
    ssbutton_set_active(ssbutton, False);
    free(ssbutton->regions);
    button_delete(ssbutton->button);
    free(ssbutton);
}

void ssbutton_set_state(ssbutton_t *ssbutton, int state)
{
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    ssbutton->current_state = state;
    if (ssbutton->visible) {
        ui_set_dirty();
    }
}

int ssbutton_get_state(ssbutton_t *ssbutton)
{
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    return ssbutton->current_state;
}

void ssbutton_set_state_image(ssbutton_t *ssbutton, int state, char *binding, point2d_t ll, point2d_t ur, colour_t colour)
{
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    ssbutton->regions[state].binding = binding;
    ssbutton->regions[state].colour = colour;
    ssbutton->regions[state].ll = ll;
    ssbutton->regions[state].ur = ur;
}

void ssbutton_set_hilit_image(ssbutton_t *ssbutton, char *binding, point2d_t ll, point2d_t ur, colour_t colour)
{
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    button_set_hilit_image(ssbutton->button, binding, ll, ur, colour);
}

void ssbutton_set_position(ssbutton_t *ssbutton, point2d_t pos)
{
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    button_set_position(ssbutton->button, pos);
}

scalar_t ssbutton_get_width(ssbutton_t *ssbutton)
{
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    return ssbutton->w;
}

scalar_t ssbutton_get_height(ssbutton_t *ssbutton)
{
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    return ssbutton->h;
}

void ssbutton_set_enabled(ssbutton_t *ssbutton, bool_t enabled)
{
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    ssbutton->enabled = enabled;
    button_set_enabled(ssbutton->button, enabled);
}

bool_t ssbutton_is_enabled(ssbutton_t *ssbutton)
{
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    return ssbutton->enabled;
}

void ssbutton_set_visible(ssbutton_t *ssbutton, bool_t visible)
{
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    if (ssbutton->active != visible) {
        ssbutton_set_active(ssbutton, visible);
    }
    if (!ssbutton->visible && visible) {
        ui_add_widget_draw_callback(ssbutton, ssbutton_draw_cb);
        ui_set_dirty();
    } else if (ssbutton->visible && !visible) {
        ui_delete_widget_draw_callback(ssbutton);
        ui_set_dirty();
    }
    ssbutton->visible = visible;
}

bool_t ssbutton_is_visible(ssbutton_t *ssbutton)
{
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    return ssbutton->visible;
}

void ssbutton_set_active(ssbutton_t *ssbutton, bool_t active)
{
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    button_set_active(ssbutton->button, active);
    ssbutton->active = active;
    if (!active) {
        ssbutton_set_visible(ssbutton, False);
    }
}

bool_t ssbutton_is_active(ssbutton_t *ssbutton)
{
    check_assertion(ssbutton != NULL, "ssbutton is NULL");
    return ssbutton->active;
}

/* EOF */