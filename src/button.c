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
#include "button.h"
#include "fonts.h"
#include "textures.h"
#include "ui_mgr.h"

typedef struct {
    char *binding;   /* name of texture binding */
    colour_t colour; /* colour to use when drawing texture */
    point2d_t ll;    /* lower left */
    point2d_t ur;    /* upper right */
}
texture_region_t;

struct button_ {
    point2d_t pos;
    scalar_t w;
    scalar_t h;
    texture_region_t tex;
    texture_region_t hilit_tex;
    texture_region_t clicked_tex;
    texture_region_t disabled_tex;
    char *font_binding;
    char *hilit_font_binding;
    char *disabled_font_binding;
    char *label;
    button_click_event_cb_t click_cb;
    void *click_cb_userdata;
    bool_t clicked; /* is the button currently pushed down? */
    bool_t focused; /* is the mouse pointer currently in the button? */
    bool_t enabled; /* can the button be clicked? */
    bool_t visible; /* is the button being drawn by the UI manager? (visible implies active) */
    bool_t active;  /* is the button receiving mouse events? */
};

static bool_t in_bbox(button_t* button, int x, int y)
{
    return (bool_t)(
        (x >= button->pos.x) &&
        (x <= button->pos.x + button->w) &&
        (y >= button->pos.y) &&
        (y <= button->pos.y + button->h)
    );
}

static void button_mouse_down_cb(void* widget, winsys_mouse_button_t which_button, int x, int y)
{
    button_t *button = (button_t*)widget;

    check_assertion(button != NULL, "button is NULL");

    if (which_button != WS_LEFT_BUTTON) {
        return;
    }

    if (!in_bbox(button, x, y)) {
        return;
    }

    if (!button->enabled) {
        return;
    }

    if (button->clicked == False) {
        print_debug(DEBUG_UI, "Button is down");
        button->clicked = True;
        ui_set_dirty();
    }
}

void button_perform_click_action(button_t* button)
{
    check_assertion(button != NULL, "button is NULL");

    if (button->click_cb) {
        button->click_cb(button, button->click_cb_userdata);
    }
    
}

static void button_mouse_up_cb(void* widget, winsys_mouse_button_t which_button, int x, int y)
{
    button_t *button = (button_t*)widget;

    check_assertion(button != NULL, "button is NULL");

    if (which_button != WS_LEFT_BUTTON) {
        return;
    }
    if (!button->enabled) {
        return;
    }
    if (!in_bbox(button, x, y)) {
        if (button->clicked) {
            print_debug(DEBUG_UI, "Button is up (not clicked)");
            button->clicked = False;
            ui_set_dirty();
        }
        return;
    } 
    if (button->clicked) {
        button->clicked = False;
        print_debug(DEBUG_UI, "Button was clicked");
        button_perform_click_action(button);
        ui_set_dirty();
    }
}

void button_simulate_mouse_click(button_t* button)
{
    check_assertion(button != NULL, "button is NULL");

    if (!button->enabled || !button->active) {
        return;
    }
    button_perform_click_action(button);
}

static void button_mouse_motion_cb(void* widget, int x, int y)
{
    button_t *button = (button_t*)widget;

    check_assertion(button != NULL, "button is NULL");

    if (!button->enabled) {
        return;
    }

    if (in_bbox(button, x, y)) {
        if (button->focused == False) {
            print_debug(DEBUG_UI, "Mouse entered button");
            button->focused = True;
            ui_set_dirty();
        }
    } else {
        if (button->focused == True) {
            print_debug(DEBUG_UI, "Mouse left button");
            button->focused = False;
            ui_set_dirty();
        }
    }
}

void button_draw(button_t* button)
{
    GLuint texobj;
    texture_region_t *tex;
    point2d_t pos;
    scalar_t w, h;
    char *font_binding;

    check_assertion(button != NULL, "button is NULL");

    pos = button->pos;
    w = button->w;
    h = button->h;

    glEnable(GL_TEXTURE_2D);

    tex = NULL;
    font_binding = NULL;

    if (!button->enabled) {
        if (button->disabled_tex.binding) {
            tex = &button->disabled_tex;
        } else if (button->tex.binding) {
            tex = &button->tex;
        }
        if (button->disabled_font_binding) {
            font_binding = button->disabled_font_binding;
        } else if (button->font_binding) {
            font_binding = button->font_binding;
        }
    } else if (button->clicked) {
        if (button->clicked_tex.binding) {
            tex = &button->clicked_tex;
        } else if (button->hilit_tex.binding) {
            tex = &button->hilit_tex;
        } else if (button->tex.binding) {
            tex = &button->tex;
        } 
        if (button->hilit_font_binding) {
            font_binding = button->hilit_font_binding;
        } else if (button->font_binding) {
            font_binding = button->font_binding;
        }
    } else if (button->focused) {
        if (button->hilit_tex.binding) {
            tex = &button->hilit_tex;
        } else if (button->tex.binding) {
            tex = &button->tex;
        }
        if (button->hilit_font_binding) {
            font_binding = button->hilit_font_binding;
        } else if (button->font_binding) {
            font_binding = button->font_binding;
        }
    } else {
        if (button->tex.binding) {
            tex = &button->tex;
        }
        if (button->font_binding) {
            font_binding = button->font_binding;
        }
    }

    if (tex != NULL) {
        if (!get_texture_binding(tex->binding, &texobj)) {
            print_warning(IMPORTANT_WARNING, "Couldnt get texture object for binding %s", tex->binding);
            texobj = 0;
        } 
        glBindTexture(GL_TEXTURE_2D, texobj);
        glColor4dv((scalar_t*)&tex->colour);

        glBegin(GL_QUADS);
        {
            glTexCoord2f(tex->ll.x, tex->ll.y); glVertex3f(pos.x,     pos.y,     0);
            glTexCoord2f(tex->ur.x, tex->ll.y); glVertex3f(pos.x + w, pos.y,     0);
            glTexCoord2f(tex->ur.x, tex->ur.y); glVertex3f(pos.x + w, pos.y + h, 0);
            glTexCoord2f(tex->ll.x, tex->ur.y); glVertex3f(pos.x,     pos.y + h, 0);
        }
        glEnd();
    }

    if (font_binding && button->label != NULL) {
        font_t *font;
        int w, asc, desc;

        if (!get_font_binding(font_binding, &font)) {
            print_warning(IMPORTANT_WARNING, "Couldn't get font object for binding %s", font_binding);
            font = NULL;
        } else {
            bind_font_texture(font);
            get_font_metrics(font, button->label, &w, &asc, &desc  );

            glPushMatrix();
            {
                glTranslatef(button->pos.x + button->w / 2.0 - w / 2.0, button->pos.y + button->h / 2.0 - asc / 2.0 + desc / 2.0, 0.0);
                draw_string(font, button->label);
            }
            glPopMatrix();
        }
    }
}

static void button_draw_cb(void* widget)
{
    button_draw((button_t*)widget);
}

button_t* button_create(point2d_t pos, scalar_t w, scalar_t h, char *font_binding, char *label)
{
    button_t *button;

    button = (button_t*)malloc(sizeof(button_t));

    check_assertion(button != NULL, "out of memory");

    button->pos = pos;
    button->w = w;
    button->h = h;
    button->tex.binding = NULL;
    button->hilit_tex.binding = NULL;
    button->clicked_tex.binding = NULL;
    button->disabled_tex.binding = NULL;
    button->font_binding = font_binding;
    button->disabled_font_binding = NULL;
    button->hilit_font_binding = NULL;
    button->label = label;
    button->click_cb = NULL;
    button->click_cb_userdata = NULL;
    button->clicked = False;
    button->focused = False;
    button->enabled = True;
    button->visible = False;
    button->active = False;
    return button;
}

void button_delete(button_t* button)
{
    check_assertion(button != NULL, "button is NULL");
    button_set_visible(button, False);
    button_set_active(button, False);
    free(button);
}

void button_set_click_event_cb(button_t* button, button_click_event_cb_t cb, void *userdata)
{
    check_assertion(button != NULL, "button is NULL");
    button->click_cb = cb;
    button->click_cb_userdata = userdata;
}

void button_set_hilit_font_binding(button_t* button, char* binding)
{
    check_assertion(button != NULL, "button is NULL");
    button->hilit_font_binding = binding;
}

void button_set_disabled_font_binding(button_t* button, char* binding)
{
    check_assertion( button != NULL, "button is NULL" );
    button->disabled_font_binding = binding;
}

void button_set_image(button_t* button, char* texture_binding, point2d_t p0, point2d_t p1, colour_t colour)
{
    check_assertion(button != NULL, "button is NULL");
    button->tex.binding = texture_binding;
    button->tex.ll = p0;
    button->tex.ur = p1;
    button->tex.colour = colour;
}

void button_set_hilit_image(button_t* button, char* texture_binding, point2d_t p0, point2d_t p1, colour_t colour)
{
    check_assertion(button != NULL, "button is NULL");
    button->hilit_tex.binding = texture_binding;
    button->hilit_tex.ll = p0;
    button->hilit_tex.ur = p1;
    button->hilit_tex.colour = colour;
}

void button_set_disabled_image(button_t* button, char* texture_binding, point2d_t p0, point2d_t p1, colour_t colour)
{
    check_assertion(button != NULL, "button is NULL");
    button->disabled_tex.binding = texture_binding;
    button->disabled_tex.ll = p0;
    button->disabled_tex.ur = p1;
    button->disabled_tex.colour = colour;
}

void button_set_clicked_image(button_t* button, char* texture_binding, point2d_t p0, point2d_t p1, colour_t colour)
{
    check_assertion(button != NULL, "button is NULL");
    button->clicked_tex.binding = texture_binding;
    button->clicked_tex.ll = p0;
    button->clicked_tex.ur = p1;
    button->clicked_tex.colour = colour;
}

void button_set_position(button_t* button, point2d_t pos)
{
    check_assertion(button != NULL, "button is NULL");
    button->pos = pos;
}

scalar_t button_get_width(button_t* button)
{
    check_assertion(button != NULL, "button is NULL");
    return button->w;
}

scalar_t button_get_height(button_t* button)
{
    check_assertion(button != NULL, "button is NULL");
    return button->h;
}

void button_set_enabled(button_t* button, bool_t enabled)
{
    check_assertion(button != NULL, "button is NULL");

    if (button->enabled != enabled) {
        ui_set_dirty();
    }
    button->enabled = enabled;

    if (!enabled) {
        button->clicked = False;
        button->focused = False;
    }
}

bool_t button_is_enabled(button_t* button)
{
    check_assertion(button != NULL, "button is NULL");
    return button->enabled;
}

void button_set_visible(button_t* button, bool_t visible)
{
    check_assertion(button != NULL, "button is NULL");

    /* This check is necessary to prevent infinite mutual recursion */
    if (button->active != visible) {
        button_set_active(button, visible);
    }

    if (!button->visible && visible) {
        ui_add_widget_draw_callback(button, button_draw_cb);
        ui_set_dirty();
    } else if (button->visible && !visible) {
        ui_delete_widget_draw_callback(button);
        ui_set_dirty();
    }
    button->visible = visible;
}

bool_t button_is_visible(button_t* button)
{
    check_assertion(button != NULL, "button is NULL");
    return button->visible;
}

void button_set_active(button_t* button, bool_t active)
{
    check_assertion(button != NULL, "button is NULL");

    if (!button->active && active) {
        ui_add_mouse_motion_callback(button, button_mouse_motion_cb);
        ui_add_mouse_down_callback(button, button_mouse_down_cb);
        ui_add_mouse_up_callback(button, button_mouse_up_cb);
        ui_set_dirty();
    } else if (button->active && !active) {
        ui_delete_mouse_motion_callback(button);
        ui_delete_mouse_down_callback(button);
        ui_delete_mouse_up_callback(button);
        ui_set_dirty();
    }
    button->active = active;
    if (!active) {
        button_set_visible(button, False);
    }
}

bool_t button_is_active(button_t* button)
{
    check_assertion(button != NULL, "button is NULL");
    return button->active;
}

void button_set_highlight(button_t* button, bool_t highlight)
{
    check_assertion(button != NULL, "button is NULL");

    if (!(button->focused == highlight)) {
        ui_set_dirty();
    }
    button->focused = highlight;
}

/* EOF */
