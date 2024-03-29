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
#include "audio.h"
#include "game_config.h"
#include "multiplayer.h"
#include "gl_util.h"
#include "fps.h"
#include "render_util.h"
#include "phys_sim.h"
#include "view.h"
#include "course_render.h"
#include "tux.h"
#include "tux_shadow.h"
#include "keyboard.h"
#include "loop.h"
#include "fog.h"
#include "viewfrustum.h"
#include "hud.h"
#include "game_logic_util.h"
#include "fonts.h"
#include "ui_mgr.h"
#include "ui_snow.h"
#include "ui_theme.h"
#include "joystick.h"
#include "part_sys.h"

#define NEXT_MODE RACING

static char *loaded_course = NULL;
static race_conditions_t loaded_conditions = (race_conditions_t)-1;

void draw_loading_text( void )
{
    int w = getparam_x_resolution();
    int h = getparam_y_resolution();
    int x_org, y_org;
    char *string;
    int string_w, asc, desc;
    font_t *font;

    x_org = w/2.0;
    y_org = h/2.0;

    if ( !get_font_binding( "loading", &font ) ) {
        print_warning( IMPORTANT_WARNING, "Couldn't get font for binding loading" );
    } else {
        string = "Loading, Please Wait...";
        get_font_metrics( font, string, &string_w, &asc, &desc );

        glPushMatrix();
        {
            glTranslatef( w/2.0 - string_w/2.0, h/2.0 - desc, 0 );
            bind_font_texture( font );
            draw_string( font, string );
        }
        glPopMatrix();
    }
}

void loading_init(void) 
{
    winsys_set_display_func( main_loop );
    winsys_set_idle_func( main_loop );
    winsys_set_reshape_func( reshape );
    winsys_set_mouse_func( NULL );
    winsys_set_motion_func( NULL );
    winsys_set_passive_motion_func( NULL );

    play_music( "loading" );
}

void loading_loop(scalar_t time_step)
{
    int width, height;
    width  = getparam_x_resolution();
    height = getparam_y_resolution();

    check_gl_error();
    update_audio();
    set_gl_options(GUI);
    clear_rendering_context();
    ui_setup_display();

    if (getparam_ui_snow()) {
        update_ui_snow(time_step, g_game.race.windy);
        draw_ui_snow();
    }
    ui_draw_menu_decorations();
    draw_loading_text();

    reshape(width, height);
    winsys_swap_buffers();

    if (loaded_course == NULL || loaded_course != g_game.race.course || loaded_conditions != g_game.race.conditions) 
    {
        /* load the course */
        load_course(g_game.race.course);
        loaded_course = g_game.race.course;
        loaded_conditions = g_game.race.conditions;
    }
    set_course_mirroring(g_game.race.mirrored);

    /* we're done here, enter intro mode */
    set_game_mode(INTRO);
} 

void loading_register()
{
    register_loop_funcs(LOADING, loading_init, loading_loop, NULL);
}

/* EOF */