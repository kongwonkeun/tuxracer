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
#include "audio_data.h"
#include "audio.h"

#if defined(HAVE_SDL) && defined(HAVE_SDL_MIXER)

#include "SDL.h"
#include "SDL_mixer.h"
#include "hash.h"
#include "list.h"

#define SOUNDFILE_MISSING_WARNING_LEVEL 10
#define SOUNDFILE_BUSY_WARNING_LEVEL 10

typedef struct {
    Mix_Chunk *data;    /* sound data */
    int ref_ctr;        /* reference counter */
}
sound_record_t;

typedef struct {
    Mix_Music *data;    /* music data */
    int ref_ctr;        /* reference counter */
    bool_t playing;     /* is this music playing? */
}
music_record_t;

static hash_table_t sound_hash_;
static hash_table_t music_hash_;
static bool_t initialized_ = False;
static bool_t sound_dirty_ = False;
static bool_t music_dirty_ = False;

/* initializes audio module */
void init_audio_data()
{
    if (!initialized_) {
        sound_hash_ = create_hash_table();
        check_assertion(sound_hash_ != NULL, "sound_hash_ is null");
        music_hash_ = create_hash_table();
        check_assertion(music_hash_ != NULL, "music_hash_ is null");
        initialized_ = True;
    }
}

static bool_t load_sound_or_music(char *name, char *filename, bool_t load_music)
{
    char *data_ptr;
    char *record_ptr;
    char *temp_record_ptr;
    hash_table_t hash;
    char *desc;
    int ref_ctr = 0;

    check_assertion(initialized_, "audio_data module not initialized");
    check_assertion(name != NULL, "null name");
    check_assertion(filename != NULL, "null filename");

    if (!is_audio_open()) {
        return False;
    }

    if (load_music) {
        hash = music_hash_;
        desc = "music";
    } else {
        hash = sound_hash_;
        desc = "sound";
    }

    if (get_hash_entry(hash, name, (hash_entry_t*)&record_ptr)) {
        print_debug(DEBUG_SOUND, "Overwriting %s name %s", desc, name);

        /* Need to save ref_ctr */
        if (load_music) {
            music_record_t *mrec = (music_record_t*)record_ptr;
            if (mrec->playing) {
                /* Can't overwrite since music is playing */
                print_warning(SOUNDFILE_BUSY_WARNING_LEVEL, "Can't overwrite %s name %s since it is playing", desc, name);
                return False;
            }
            ref_ctr = mrec->ref_ctr;
        } else {
            sound_record_t *srec = (sound_record_t*)record_ptr;
            ref_ctr = srec->ref_ctr;
        }
        del_hash_entry(hash, name, NULL);
    }

    if (load_music) {
        data_ptr = (char*)Mix_LoadMUS(filename);
    } else {
        data_ptr = (char*)Mix_LoadWAV(filename);
    }

    if (data_ptr == NULL) {
        print_warning(SOUNDFILE_MISSING_WARNING_LEVEL, "FAILED to load %s file %s: %s", desc, filename, Mix_GetError());
        return False;
    }

    print_debug(DEBUG_SOUND, "Successfully loaded %s file %s", desc, filename);

    if (load_music) {
        music_record_t *mrec = (music_record_t*)malloc(sizeof(music_record_t));
        mrec->data = (Mix_Music*)data_ptr;
        mrec->ref_ctr = ref_ctr;
        mrec->playing = False;
        record_ptr = (char*)mrec;
    } else {
        sound_record_t *srec = (sound_record_t*)malloc(sizeof(sound_record_t));
        srec->data = (Mix_Chunk*)data_ptr;
        srec->ref_ctr = ref_ctr;
        record_ptr = (char*)srec;
    }
    add_hash_entry(hash, name, (hash_entry_t)record_ptr);
    /* Make sure it's there */
    check_assertion(get_hash_entry(hash, name, (void*)&temp_record_ptr) && (record_ptr == temp_record_ptr), "failed addition to hash table");
    temp_record_ptr = NULL; /* to prevent warnings when assert turned off */

    if (load_music) {
        music_dirty_ = True;
    } else {
        sound_dirty_ = True;
    }
    return True;
}

bool_t load_sound( char *name, char *filename ) 
{ 
    check_assertion(initialized_, "audio_data module not initialized");
    return load_sound_or_music(name, filename, False);
}

bool_t load_music(char *name, char *filename) 
{ 
    check_assertion(initialized_, "audio_data module not initialized");
    return load_sound_or_music(name, filename, True);
}

bool_t get_sound_data(char *name, Mix_Chunk **data)
{
    sound_record_t *srec;

    check_assertion(initialized_, "audio_data module not initialized");
    check_assertion(name != NULL, "null name");

    if (!get_hash_entry(sound_hash_, name, (hash_entry_t*)&srec)) {
        return False;
    }
    if (data) {
        *data = srec->data;
    }
    return True;
}

bool_t get_music_data(char *name, Mix_Music **data)
{
    music_record_t *mrec;

    check_assertion(initialized_, "audio_data module not initialized");
    check_assertion(name != NULL, "null name");

    if (!get_hash_entry(music_hash_, name, (hash_entry_t*)&mrec)) {
        return False;
    }
    if (data) {
        *data = mrec->data;
    }
    return True;
}

void set_music_playing_status(char *name, bool_t playing)
{
    music_record_t *mrec;

    check_assertion(initialized_, "audio_data module not initialized");
    check_assertion(name != NULL, "null name");

    if (!get_hash_entry(music_hash_, name, (hash_entry_t*)&mrec)) {
        check_assertion(0, "couldn't find music");
    }
    mrec->playing = playing;
}

bool_t get_music_playing_status(char *name)
{
    music_record_t *mrec;

    check_assertion(initialized_, "audio_data module not initialized");
    check_assertion(name != NULL, "null name");

    if (!get_hash_entry(music_hash_, name, (hash_entry_t*)&mrec)) {
        check_assertion(0, "couldn't find music");
    }
    return mrec->playing;
}

void incr_sound_data_ref_ctr(char *name)
{
    sound_record_t *srec;
    bool_t found;

    check_assertion(initialized_, "audio_data module not initialized");
    check_assertion(name != NULL, "null name");
    found = get_hash_entry(sound_hash_, name, (hash_entry_t*)&srec);
    check_assertion(found, "hashtable entry not found");
    srec->ref_ctr += 1;
    print_debug(DEBUG_SOUND, "incremented reference counter of sound %s to %d", name, srec->ref_ctr);
}

void decr_sound_data_ref_ctr(char *name)
{
    sound_record_t *srec;
    bool_t found;

    check_assertion(initialized_, "audio_data module not initialized");
    check_assertion(name != NULL, "null name");
    found = get_hash_entry(sound_hash_, name, (hash_entry_t*)&srec);
    check_assertion(found, "hashtable entry not found");
    srec->ref_ctr -= 1;
    print_debug(DEBUG_SOUND, "decremented reference counter of sound %s to %d", name, srec->ref_ctr);
}

void incr_music_data_ref_ctr(char *name)
{
    music_record_t *mrec;
    bool_t found;

    check_assertion(initialized_, "audio_data module not initialized");
    check_assertion(name != NULL, "null name");
    found = get_hash_entry(music_hash_, name, (hash_entry_t*)&mrec);
    check_assertion(found, "hashtable entry not found");
    mrec->ref_ctr += 1;
    print_debug(DEBUG_SOUND, "incremented reference counter of music %s to %d", name, mrec->ref_ctr);
}

void decr_music_data_ref_ctr(char *name)
{
    music_record_t *mrec;
    bool_t found;

    check_assertion(initialized_, "audio_data module not initialized");
    check_assertion(name != NULL, "null name");
    found = get_hash_entry(music_hash_, name, (hash_entry_t*)&mrec);
    check_assertion(found, "hashtable entry not found");
    mrec->ref_ctr -= 1;
    print_debug(DEBUG_SOUND, "decremented reference counter of music %s to %d", name, mrec->ref_ctr);
}

void delete_unused_audio_data()
{
    hash_search_t iter;
    sound_record_t *srec;
    music_record_t *mrec;
    char *key;
    list_t keys_to_delete;
    list_elem_t elem;
    bool_t found;

    check_assertion(initialized_, "audio_data module not initialized");

    /* clean sounds first */
    keys_to_delete = create_list();

    begin_hash_scan(sound_hash_, &iter);

    while (next_hash_entry(iter, &key, (hash_entry_t*)&srec)) {
        if (srec->ref_ctr == 0) {
            /* save key; we'll delete later */
            insert_list_elem(keys_to_delete, NULL, key);
            Mix_FreeChunk(srec->data);
        }
    }
    end_hash_scan(iter);

    for (elem = get_list_head(keys_to_delete); elem != NULL; elem = get_next_list_elem(keys_to_delete, elem)) {
        key = (char*)get_list_elem_data(elem);
        found = del_hash_entry(sound_hash_, key, NULL);
        check_assertion(found, "key disappeared!");
    }
    del_list(keys_to_delete);

    /* now clean music */
    keys_to_delete = create_list();
    begin_hash_scan(music_hash_, &iter);

    while (next_hash_entry(iter, &key, (hash_entry_t*)&mrec)) {
        if (mrec->ref_ctr == 0) {
            /* we shouldn't be playing music with ref cnt of 0 */
            check_assertion(mrec->playing == False, "playing music with reference count of 0");
            /* save key; we'll delete later */
            insert_list_elem(keys_to_delete, NULL, key);
            Mix_FreeMusic(mrec->data);
        }
    }
    end_hash_scan(iter);

    for (elem = get_list_head(keys_to_delete); elem != NULL; elem = get_next_list_elem(keys_to_delete, elem)) {
        key = (char*)get_list_elem_data(elem);
        found = del_hash_entry(music_hash_, key, NULL);
        check_assertion(found, "key disappeared!");
    }
    del_list(keys_to_delete);
}

bool_t is_sound_data_dirty()
{
    check_assertion(initialized_, "audio_data module not initialized");
    return sound_dirty_;
}

bool_t is_music_data_dirty()
{
    check_assertion(initialized_, "audio_data module not initialized");
    return music_dirty_;
}

void mark_sound_data_clean()
{
    check_assertion(initialized_, "audio_data module not initialized");
    sound_dirty_ = False;
}

void mark_music_data_clean()
{
    check_assertion(initialized_, "audio_data module not initialized");
    music_dirty_ = False;
}

static int load_sound_cb(ClientData cd, Tcl_Interp *ip, int argc, char *argv[]) 
{
    Tcl_Obj *result;

    check_assertion(initialized_, "audio_data module not initialized");

    if (argc != 3) {
        Tcl_AppendResult(ip, argv[0], ": invalid number of arguments\n", "Usage: ", argv[0], " <name> <sound file>", (char*)0);
        return TCL_ERROR;
    } 
    result = Tcl_NewBooleanObj(load_sound(argv[1], argv[2]));
    Tcl_SetObjResult(ip, result);
    return TCL_OK;
} 

static int load_music_cb( ClientData cd, Tcl_Interp *ip, int argc, char *argv[]) 
{
    Tcl_Obj *result;

    check_assertion(initialized_, "audio_data module not initialized");

    if (argc != 3) {
        Tcl_AppendResult(ip, argv[0], ": invalid number of arguments\n", "Usage: ", argv[0], " <name> <sound file>", (char*)0);
        return TCL_ERROR;
    } 
    result = Tcl_NewBooleanObj(load_music(argv[1], argv[2]));
    Tcl_SetObjResult( ip, result );
    return TCL_OK;
} 

/* register sound and music data's Tcl callbacks */
void register_sound_data_tcl_callbacks(Tcl_Interp *ip)
{
    Tcl_CreateCommand(ip, "tux_load_sound", load_sound_cb, 0, 0);
    Tcl_CreateCommand(ip, "tux_load_music", load_music_cb, 0, 0);
}

#endif /* defined(HAVE_SDL) && defined(HAVE_SDL_MIXER) */

/* EOF */