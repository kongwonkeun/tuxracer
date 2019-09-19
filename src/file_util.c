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
#include "file_util.h"

#define PATH_MAX 512

static void convert_path( char *new_path, char *orig_path ) 
{
    strcpy( new_path, orig_path );
}

bool_t file_exists( char *filename )
{
#if defined(WIN32)
    /* Test existence by opening file -- I'm not a Win32 programmer,
       so if there's a better way let me know */
    FILE *file;
    file = fopen( filename, "r" );

    if ( file == NULL ) {
        return False;
    } else {
        if ( fclose( file ) != 0 ) {
            handle_error( 1, "error closing file %s", filename );
        }
        return True;
    }
#endif /* defined(WIN32) */
}


bool_t dir_exists( char *dirname )
{
#if defined(WIN32)
    /* Win32 */
    char curdir[PATH_MAX];
    bool_t dir_exists = False;

    if ( _getcwd( curdir, BUFF_LEN - 1 ) == NULL ) { //---- kong ---- getcwd to _getcwd
        handle_system_error( 1, "getcwd failed" );
    }

    if ( _chdir( dirname ) == -1 ) { //---- kong ---- chdir to _chdir
        return False;
    }

    if ( _chdir( curdir ) == -1 ) { //---- kong ---- chdir to _chdir
        handle_system_error( 1, "Couldn't access directory %s", curdir );
    }
    return True;
#endif /* defined(WIN32) */
}

list_t get_dir_file_list(char *dirname) 
{
#if defined (WIN32)
    list_t dirlist = NULL;
    list_elem_t cur_elem = NULL;
    HANDLE hFind;
    WIN32_FIND_DATA finddata;
    char curdir[PATH_MAX];

    if (_getcwd(curdir, BUFF_LEN - 1) == NULL) {
        handle_system_error(1, "getcwd failed");
    }

    if (_chdir(dirname) == -1) {
        return NULL;
    }

    dirlist = create_list();

    if ((hFind = FindFirstFile(L"*.*", &finddata)) ==  INVALID_HANDLE_VALUE) { //---- kong ---- add L
        return dirlist;
    }

    do {
        cur_elem = insert_list_elem(dirlist, cur_elem, string_copy((char *)finddata.cFileName));
    } while (FindNextFile(hFind, &finddata));

    if (!FindClose(hFind)) {
        handle_system_error(1, "couldn't close directory %s", dirname);
    }

    if (_chdir(curdir) == -1) {
        handle_system_error(1, "couldn't access directory %s", curdir);
    }

    return dirlist;
#endif /* defined (WIN32) */
}

void free_dir_file_list( list_t dirlist )
{
    list_elem_t cur_elem;
    char *string;

    if ( dirlist == NULL ) {
        return;
    }

    for ( cur_elem = get_list_head( dirlist );
        cur_elem != NULL;
        cur_elem = get_next_list_elem( dirlist, cur_elem ) )
    {
        string = (char*) get_list_elem_data( cur_elem );
        free( string );
    }
    del_list( dirlist );
}

/* EOF */