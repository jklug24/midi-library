/* Name, alterations.c, CS 24000, Spring 2020
 * Last updated April 9, 2020
 */

/* Add any includes here */

#include "../include/alterations.h"
#include <malloc.h>
#include <assert.h>
#include <string.h>


/*Define change_event_octave here */
int change_event_octave(event_t *event, int *octaves) {
    if (event_type(event) == MIDI_EVENT_T) {
        uint8_t status = event->type & 0xF0;
        if ((status == 0x80) || (status == 0x90) || (status == 0xA0)) {
            uint8_t new = event->midi_event.data[0] + ((*octaves) * 12);
            if ((new >= 0) && (new < 128)) {
                event->midi_event.data[0] = new;
                return 1;
            }
        }
    }
    return 0;
}

/*Define change_event_time here */
int change_event_time(event_t *event, float *multiplier) {
    int old = 0;
    int temp = event->delta_time;
    while (temp) {
        temp >>= 7;
        old++;
    }
    event->delta_time = event->delta_time * (*multiplier);
    int new = 0;
    temp = event->delta_time;
    while (temp) {
        temp >>= 7;
        new++;
    }
    return new - old;
}

/*Define change_event_instrument here */
int change_event_instrument(event_t *event, remapping_t instrument) {
    if (event_type(event) == MIDI_EVENT_T) {
        if ((event->type & 0xF0) == 0xC0) {
            event->midi_event.data[0] = instrument[event->midi_event.data[0]];
            return 1;
        }
    }
    return 0;
}

/*Define change_event_note here */
int change_event_note(event_t *event, remapping_t note) {
    if (event_type(event) == MIDI_EVENT_T) {
        uint8_t status = event->type & 0xF0;
        if (status == 0x80 || status == 0x90 || status == 0xA0) {
            event->midi_event.data[0] = note[event->midi_event.data[0]];
            return 1;
        }
    }
    return 0;
}

/*Define apply_to_events here */
int apply_to_events(song_data_t *song, event_func_t function, void *remap) {
    assert(song);
    assert(function);
    int count = 0;
    track_node_t *track_node = song->track_list;
    while (track_node) {
        event_node_t *event_node = track_node->track->event_list;
        while(event_node) {
            count += function(event_node->event, remap);
            event_node = event_node->next_event;
        }
        track_node = track_node->next_track;
    }
    return count;
}

/*Define change_octave here */
int change_octave(song_data_t *song, int octaves) {
    return apply_to_events(song, (event_func_t) change_event_octave, &octaves);
}

/*Define warp_time here */
int warp_time(song_data_t *song, float multiplier) {
    assert(song);
    int count = 0;
    track_node_t *track_node = song->track_list;
    while (track_node) {
        int track_count = 0;
        event_node_t *event_node = track_node->track->event_list;
        while(event_node) {
            track_count += change_event_time(event_node->event, &multiplier);
            event_node = event_node->next_event;
        }
        track_node->track->length += track_count;
        count += track_count;
        track_node = track_node->next_track;
    }
    return count;
}

/*Define remap_instruments here */
int remap_instruments(song_data_t *song, remapping_t remap) {
    return apply_to_events(song, (event_func_t) change_event_instrument, remap);
}

/*Define remap_notes here */
int remap_notes(song_data_t *song, remapping_t remap) {
    return apply_to_events(song, (event_func_t) change_event_note, remap);
}

/*Define add_round here */
void add_round(song_data_t *song, int track_index, int ovtave_differential, unsigned int time_delay, uint8_t instrument) {
    assert(song);
    assert(song->format != 2);
    track_node_t *original = song->track_list;
    for (int i = 1; i < track_index; i++) {
        original = original->next_track;
    }
    track_node_t *new = deep_copy_track(original);
    while (original->next_track) {
        original = original->next_track;
    }
    original->next_track = new;
}

void deep_copy_song(song_data_t *original, song_data_t **new_ptr) {
    if (*new_ptr) {
        free_song(*new_ptr);
    }
    *new_ptr = malloc(sizeof(song_data_t));
    assert(*new_ptr);
    song_data_t *new = *new_ptr;

    new->path = malloc(strlen(original->path));
    new->path = strcpy(new->path, original->path);
    new->format = original->format;
    new->num_tracks = original->num_tracks;

    new->division.uses_tpq = original->division.uses_tpq;
    if (new->division.uses_tpq) {
        new->division.ticks_per_qtr = original->division.ticks_per_qtr;
    } else {
        new->division.ticks_per_frame = original->division.ticks_per_frame;
        new->division.frames_per_sec = original->division.frames_per_sec;
    }

    track_node_t *original_track = original->track_list;
    if (original_track) {
        new->track_list = deep_copy_track(original_track);
        original_track = original_track->next_track;
        new->track_list->next_track = NULL;
    }

    track_node_t *new_track = new->track_list;
    while (original_track) {
        new_track->next_track = deep_copy_track(original_track);
        new_track = new_track->next_track;
        new_track->next_track = NULL;
        original_track = original_track->next_track;
    }
}

/*Define deep_copy_track here*/
track_node_t *deep_copy_track(track_node_t *original) {
    track_node_t *new = malloc(sizeof(track_node_t));
    assert(new);

    new->track = malloc(sizeof(track_t));
    assert(new->track);
    new->track->length = original->track->length;

    event_node_t *original_event = original->track->event_list;
    if (original_event) {
        new->track->event_list = deep_copy_event(original_event);
        original_event = original_event->next_event;
        new->track->event_list->next_event = NULL;
    }

    event_node_t *new_event = new->track->event_list;
    while (original_event) {
        new_event->next_event = deep_copy_event(original_event);
        new_event = new_event->next_event;
        new_event->next_event = NULL;
        original_event = original_event->next_event;
    }
    return new;
}

/*Define deep_copy_event here*/
event_node_t *deep_copy_event(event_node_t *original) {
    event_node_t *new = malloc(sizeof(event_node_t));
    assert(new);

    new->event = malloc(sizeof(event_t));
    assert(new->event);
    new->event->delta_time = original->event->delta_time;
    new->event->type = original->event->type;


    if (event_type(original->event) == SYS_EVENT_T) {
        int len = original->event->sys_event.data_len;
        new->event->sys_event.data_len = len;
        if (len) {
            new->event->sys_event.data = malloc(len * sizeof(uint8_t));
            for (int i = 0; i < len; i++) {
                new->event->sys_event.data[i] = original->event->sys_event.data[i];
            }
        } else {
            new->event->sys_event.data = NULL;
        }

    } else if (event_type(original->event) == META_EVENT_T) {
        new->event->meta_event.name = original->event->meta_event.name;
        int len = original->event->meta_event.data_len;
        new->event->meta_event.data_len = len;
        if (len) {
            new->event->meta_event.data = malloc(len * sizeof(uint8_t));
            for (int i = 0; i < len; i++) {
                new->event->meta_event.data[i] = original->event->meta_event.data[i];
            }
        } else {
            new->event->meta_event.data = NULL;
        }

    } else if (event_type(original->event) == MIDI_EVENT_T) {
        new->event->midi_event.status = original->event->midi_event.status;
        new->event->midi_event.name = original->event->midi_event.name;
        int len = original->event->midi_event.data_len;
        new->event->midi_event.data_len = len;
        if (len) {
            new->event->midi_event.data = malloc(len * sizeof(uint8_t));
            for (int i = 0; i < len; i++) {
                new->event->midi_event.data[i] = original->event->midi_event.data[i];
            }
        } else {
            new->event->midi_event.data = NULL;
        }
    } else {
        return NULL;
    }

    return new;
}

/*
 * Function called prior to main that sets up random mapping tables
 */

void build_mapping_tables() {
    for (int i = 0; i <= 0xFF; i++) {
        I_BRASS_BAND[i] = 61;
    }

    for (int i = 0; i <= 0xFF; i++) {
        I_HELICOPTER[i] = 125;
    }

    for (int i = 0; i <= 0xFF; i++) {
        N_LOWER[i] = i;
    }
    //  Swap C# for C
    for (int i = 1; i <= 0xFF; i += 12) {
        N_LOWER[i] = i-1;
    }
    //  Swap F# for G
    for (int i = 6; i <= 0xFF; i += 12) {
        N_LOWER[i] = i+1;
    }
} /* build_mapping_tables() */