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

#if defined(HAVE_SDL) && defined(HAVE_SDL_MIXER)

#include "SDL.h"
#include "SDL_mixer.h"
#include "audio.h"
#include "audio_data.h"
#include "hash.h"

typedef struct {
    int num_sounds;
    char **sound_names; /* array of sound names */
    Mix_Chunk **chunks; /* array of cached Mix_Chunk* */
    int loop_count;
    int channel;
    int volume;
}
sound_context_data_t;

typedef struct {
    char *music_name;
    Mix_Music *music;
    int loop;
}
music_context_data_t;

static hash_table_t sound_contexts_;
static hash_table_t music_contexts_;

static bool_t initialized_ = False;

/* Data related to currently-playing music */
static Mix_Music *current_music_data_ = NULL;
static char *current_music_name_ = NULL;

/* initializes the audio module. */
void init_audio()
{
    int hz, channels, buffer;
    Uint16 format;

    check_assertion(!initialized_, "init_audio called twice");

    sound_contexts_ = create_hash_table();
    music_contexts_ = create_hash_table();
    initialized_ = True;

    /* init SDL audio */
    if (getparam_no_audio() == False) {
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            handle_error(1, "couldn't initialize SDL: %s", SDL_GetError());
        }
        /* open the audio device */
        switch (getparam_audio_freq_mode()) {
        case 0:  hz = 11025; break;
        case 1:  hz = 22050; break;
        case 2:  hz = 44100; break;
        default: hz = 22050;
            setparam_audio_freq_mode(1);
        }

        switch (getparam_audio_format_mode()) {
        case 0:  format = AUDIO_U8; break;
        case 1:  format = AUDIO_S16SYS; break;
        default: format = AUDIO_S16SYS;
            setparam_audio_format_mode(1);
        }

        if (getparam_audio_stereo()) {
            channels = 2;
        } else {
            channels = 1;
        }

        buffer = getparam_audio_buffer_size();

        if (Mix_OpenAudio(hz, format, channels, buffer) < 0) {
            print_warning(1, "warning: couldn't set %d Hz %d-bit audio\nreason: %s\n", hz, getparam_audio_format_mode() == 0 ? 8 : 16, SDL_GetError());
        } else {
            print_debug(DEBUG_SOUND, "opened audio device at %d Hz %d-bit audio", hz, getparam_audio_format_mode() == 0 ? 8 : 16);
        }
    }
}

bool_t is_audio_open()
{
    int tmp_freq;
    Uint16 tmp_format;
    int tmp_channels;

    return (bool_t)Mix_QuerySpec(&tmp_freq, &tmp_format, &tmp_channels);
}

void bind_sounds_to_context(char *sound_context, char **names, int num_sounds)
{
    int i;
    sound_context_data_t *data;

    check_assertion(initialized_, "audio module not initialized");
    if (get_hash_entry(sound_contexts_, sound_context, (hash_entry_t*)&data)) {
        /* Entry for this context already exists; decrement ref_cts & delete arrays */
        for (i = 0; i < data->num_sounds; i++) {
            decr_sound_data_ref_ctr(data->sound_names[i]);
            free(data->sound_names[i]);
        }
        free(data->sound_names);
        free(data->chunks);
        del_hash_entry(sound_contexts_, sound_context, NULL);
    } else {
        data = (sound_context_data_t*)malloc(sizeof(sound_context_data_t));
    }
    if (num_sounds == 0) {
        /* delete the context */
        free(data);
        return;
    }
    check_assertion(num_sounds > 0, "num_sounds isn't > 0");
    data->num_sounds = num_sounds;
    data->sound_names = (char**)malloc(sizeof(char*)*num_sounds);
    data->chunks = (Mix_Chunk**)malloc(sizeof(Mix_Chunk*)*num_sounds);
    data->loop_count = 0;
    data->channel = 0;
    data->volume = 128;

    for (i = 0; i < num_sounds; i++) {
        data->sound_names[i] = string_copy(names[i]);
        incr_sound_data_ref_ctr(names[i]);
        data->chunks[i] = NULL;
    }
    add_hash_entry(sound_contexts_, sound_context, (hash_entry_t)data);
}

void bind_music_to_context(char *music_context, char *name, int loop)
{
    music_context_data_t *data;

    check_assertion(initialized_, "audio module not initialized");
    if (get_hash_entry(music_contexts_, music_context, (hash_entry_t*)&data)) {
        /* Entry for this context already exists; decrement ref count & delete string */
        /* check if music is playing, stop if it is */
        if (current_music_name_ != NULL && strcmp(current_music_name_, data->music_name) == 0) {
            Mix_HaltMusic();
            current_music_name_ = NULL;
            current_music_data_ = NULL;
            check_assertion(get_music_playing_status(data->music_name), "inconsistent music playing status info");
            set_music_playing_status(data->music_name, False);
        }
        decr_music_data_ref_ctr(data->music_name);
        free(data->music_name);
        del_hash_entry(music_contexts_, music_context, NULL);
    } else {
        data = (music_context_data_t*)malloc(sizeof(music_context_data_t));
    }
    if (name == NULL || name[0] == '\0') {
        /* delete the context */
        free(data);
        return;
    }
    data->music_name = string_copy(name);
    incr_music_data_ref_ctr(name);
    data->music = NULL;
    data->loop = loop;
    add_hash_entry(music_contexts_, music_context, (hash_entry_t)data);
}

static void flush_cached_sound_chunks()
{
    hash_search_t iter;
    sound_context_data_t *data;

    check_assertion(initialized_, "audio module not initialized");
    begin_hash_scan(sound_contexts_, &iter);
    while (next_hash_entry(iter, NULL, (hash_entry_t*)&data)) {
        int i;
        for (i = 0; i < data->num_sounds; i++) {
            data->chunks[i] = NULL;
        }
        if (data->loop_count == -1) {
            Mix_HaltChannel(data->channel);
        }
    }
    end_hash_scan(iter);
} 

static void flush_cached_music()
{
    hash_search_t iter;
    music_context_data_t *data;

    check_assertion(initialized_, "audio module not initialized");
    begin_hash_scan(music_contexts_, &iter);
    while (next_hash_entry(iter, NULL, (hash_entry_t*)&data)) {
        data->music = NULL;
    }
    end_hash_scan(iter);
} 

static Mix_Chunk* get_Mix_Chunk(sound_context_data_t *data, char **name)
{
    int i;

    check_assertion(initialized_, "audio module not initialized");
    if (is_sound_data_dirty()) {
        flush_cached_sound_chunks();
        mark_sound_data_clean();
    }
    /* pick a random sound */
    i = (int)(((double)data->num_sounds) * rand() / (RAND_MAX + 1.0));
    if (data->chunks[i] == NULL) {
        bool_t found;
        found = get_sound_data(data->sound_names[i], &(data->chunks[i]));
        check_assertion(found, "sound name not found");
        check_assertion(data->chunks[i] != NULL, "sound chunk not set");
    }
    if (name != NULL) {
        *name = data->sound_names[i];
    }
    return data->chunks[i];
}

static void get_music_context_data(char *music_context, Mix_Music **music, char **name, int *loop)
{
    music_context_data_t *data;

    check_assertion(initialized_, "audio module not initialized");
    check_assertion(music != NULL, "music is null");
    if (is_music_data_dirty()) {
        flush_cached_music();
        mark_music_data_clean();
    }
    if (get_hash_entry(music_contexts_, music_context, (hash_entry_t*)&data)) {
        if (data->music == NULL) {
            bool_t found;
            found = get_music_data(data->music_name, &(data->music));
            check_assertion(found, "music name not found");
            check_assertion(data->music!=NULL, "music data not set");
        }
        *music = data->music;
        if (name != NULL) {
            *name = data->music_name;
        }
        if (loop != NULL) {
            *loop = data->loop;
        }
    } else {
        /* couldn't find any data for this sound_context */
        *music = NULL;
    }
}

bool_t play_sound(char *sound_context, int loop)
{
    Mix_Chunk *chunk;
    sound_context_data_t *data;

    check_assertion(initialized_, "audio module not initialized");
    if (getparam_sound_enabled() == False) {
        return False;
    }
    if (!is_audio_open()) {
        return False;
    }
    check_assertion(sound_context != NULL, "sound_context is null");
    if (!get_hash_entry(sound_contexts_, sound_context, (hash_entry_t*)&data)) {
        return False;
    }
    chunk = get_Mix_Chunk(data, NULL);
    if (chunk == NULL) {
        return False;
    }
    if (data->loop_count == -1) {
        return False;
    }
    Mix_VolumeChunk(chunk, data->volume);
    data->channel = Mix_PlayChannel(-1, chunk, loop);
    data->loop_count = loop;
    return True;
}

bool_t halt_sound(char *sound_context)
{
    Mix_Chunk *chunk;
    sound_context_data_t *data;

    check_assertion(initialized_, "audio module not initialized");
    if (getparam_sound_enabled() == False) {
        return False;
    }
    if (!is_audio_open()) {
        return False;
    }
    check_assertion(sound_context != NULL, "sound_context is null");
    if (!get_hash_entry(sound_contexts_, sound_context, (hash_entry_t*)&data)) {
        return False;
    }
    chunk = get_Mix_Chunk(data, NULL);
    if (chunk == NULL) {
        return False;
    }
    if (data->loop_count == -1) {
        Mix_HaltChannel(data->channel);
    }
    data->loop_count = 0;
    data->channel = 0;
    return True;
}

bool_t set_sound_volume(char *sound_context, int volume)
{
    int i;
    sound_context_data_t *data;

    if (!get_hash_entry(sound_contexts_, sound_context, (hash_entry_t*)&data)) {
        return False;
    }
    data->volume = volume;

    i = (int)(((double)data->num_sounds)*rand() / (RAND_MAX + 1.0));
    for (i = 0; i < data->num_sounds; i++) {
        if (data->chunks[i] == NULL) {
            bool_t found;
            found = get_sound_data(data->sound_names[i], &(data->chunks[i]));
            check_assertion(found, "sound name not found");
            check_assertion(data->chunks[i] != NULL, "sound chunk not set");
        }
        Mix_VolumeChunk(data->chunks[i], data->volume);
    }
    return True;
}

bool_t play_music(char *music_context)
{
    Mix_Music* music;
    char* music_name;
    int loop;

    check_assertion(initialized_, "audio module not initialized");

    if (getparam_music_enabled() == False) {
        return False;
    }
    if (!is_audio_open()) {
        return False;
    }
    if (music_context == NULL) {
        if (current_music_name_ != NULL) {
            set_music_playing_status(current_music_name_, False);
        }
        Mix_HaltMusic();
        current_music_name_ = NULL;
        current_music_data_ = NULL;
        return False;
    }
    get_music_context_data(music_context, &music, &music_name, &loop);

    if (music == NULL) {
        return False;
    }
    if (current_music_data_ != music) {
        if (current_music_name_ != NULL) {
            set_music_playing_status(current_music_name_, False);
        }
        Mix_HaltMusic();
    }
    if (!Mix_PlayingMusic()) {
        Mix_PlayMusic(music, loop);
        set_music_playing_status(music_name, True);
    }
    current_music_data_ = music;
    current_music_name_ = music_name;
    return True;
}

void update_audio()
{
    int volume;
    check_assertion(initialized_, "audio module not initialized");
    if (!is_audio_open()) {
        return;
    }
    if (getparam_music_enabled() == False) {
        Mix_HaltMusic();
    }
    /* Set sounds volume level */
    volume = getparam_sound_volume();
    if (volume < 0) {
        volume = 0;
    }
    if (volume > 128) {
        volume = 128;
    }
    setparam_sound_volume(volume);
    Mix_Volume(-1, volume); /* channel of -1 sets volume for all channels */
    /* Set music volume level */
    volume = getparam_music_volume();
    if (volume < 0) {
        volume = 0;
    }
    if (volume > 128) {
        volume = 128;
    }
    setparam_music_volume(volume);
    Mix_VolumeMusic(volume);
    /* Update music status */
    if (current_music_name_ != NULL && !Mix_PlayingMusic()) {
        set_music_playing_status(current_music_name_, False);
        current_music_name_ = NULL;
        current_music_data_ = NULL;
    }
}

bool_t is_music_playing()
{
    check_assertion(initialized_, "audio module not initialized");
    check_assertion((current_music_name_ == NULL && current_music_data_ == NULL) || (current_music_name_ != NULL && current_music_data_ != NULL), "inconsistent state in sound module");
    return (bool_t)(current_music_name_ != NULL);
}

void shutdown_audio()
{
    if (is_audio_open()) {
        Mix_CloseAudio();
    }
}

static int bind_sounds_cb(ClientData cd, Tcl_Interp *ip, int argc, char *argv[]) 
{
    check_assertion(initialized_, "audio module not initialized");
    if (argc < 2) {
        Tcl_AppendResult(ip, argv[0], ": invalid number of arguments\n", "Usage: ", argv[0], " <sound context> <sound name> [<sound name> ...]", (char*)0);
        return TCL_ERROR;
    } 
    bind_sounds_to_context(argv[1], argv + 2, argc - 2);
    return TCL_OK;
} 

static int bind_music_cb(ClientData cd, Tcl_Interp *ip, int argc, char *argv[]) 
{
    int loops;
    check_assertion(initialized_, "audio module not initialized");
    if (argc != 4) {
        Tcl_AppendResult(ip, argv[0], ": invalid number of arguments\n", "Usage: ", argv[0], " <music context> <music name> <loops>", (char*)0);
        return TCL_ERROR;
    } 
    if (Tcl_GetInt(ip, argv[3], &loops)) {
        Tcl_AppendResult(ip, "invalid value for loops parameter", (char*)0);
        return TCL_ERROR;
    }
    bind_music_to_context(argv[1], argv[2], loops);
    return TCL_OK;
} 

/* registers sound and music module's Tcl callbacks */
void register_sound_tcl_callbacks(Tcl_Interp *ip)
{
    Tcl_CreateCommand(ip, "tux_bind_sounds", bind_sounds_cb, 0, 0);
    Tcl_CreateCommand(ip, "tux_bind_music",  bind_music_cb,  0, 0);
}

#endif /* defined(HAVE_SDL) && defined(HAVE_SDL_MIXER) */

/* EOF */
