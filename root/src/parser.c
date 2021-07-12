/* Name, parser.c, CS 24000, Spring 2020
 * Last updated March 27, 2020
 */

/* Add any includes here */

#include "../include/parser.h"
#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "../include/event_tables.h"

uint8_t g_midi_status = 0xFF;

/*
 * Define parse_file here
 */

song_data_t *parse_file(const char *file_name) {
    assert(file_name);
    FILE *file = fopen(file_name, "r");
    assert(file);
    song_data_t *song = malloc(sizeof(song_data_t));
    song->track_list = NULL;
    assert(song);
    song->path = malloc(strlen(file_name) + 1);
    strcpy(song->path, file_name);
    parse_header(file, song);
    for (int i = 0; i < song->num_tracks; i++) {
        parse_track(file, song);
    }
    assert(getc(file) == EOF);
    fclose(file);
    return song;
} /* parse_file() */

/*
 * Define parse_header here
 */

void parse_header(FILE *file, song_data_t *song_data) {
    assert(file);
    assert(song_data);
    char type[] = "MThd";
    for (int i = 0; i < 4; i++) {
        assert(getc(file) == type[i]);
    }
    uint32_t length = 0;
    uint8_t length_buff[4] = {};
    for (int i = 0; i < 4; i++) {
        length_buff[i] = getc(file);
    }
    length = end_swap_32(&length_buff[0]);
    assert(length == 6);
    uint8_t format_buff[2] = {};
    format_buff[0] = getc(file);
    format_buff[1] = getc(file);
    song_data->format = end_swap_16(&format_buff[0]);
    assert(song_data->format >= 0);
    assert(song_data->format < 3);
    uint8_t track_buff[2] = {};
    track_buff[0] = getc(file);
    track_buff[1] = getc(file);
    song_data->num_tracks = end_swap_16(&track_buff[0]);
    if (song_data->format == 0) {
        assert(song_data->num_tracks == 1);
    } else {
        assert(song_data->num_tracks >= 1);
    }
    uint8_t div_buff[2] = {};
    div_buff[0] = getc(file);
    div_buff[1] = getc(file);
    if (!(div_buff[1] & 0x80)) {
        song_data->division.uses_tpq = false;
        song_data->division.ticks_per_qtr = end_swap_16(&div_buff[0]);
    } else {
        song_data->division.uses_tpq = true;
        song_data->division.ticks_per_frame = div_buff[1];
        song_data->division.frames_per_sec = div_buff[0];
    }
    assert(getc(file) == 'M');
    fseek(file, -1, SEEK_CUR);
} /* parse_header() */

/*
 * Define parse_track here
 */

void parse_track(FILE *file, song_data_t *song_data) {
    track_node_t *new = malloc(sizeof(track_node_t));
    assert(new);
    new->next_track = NULL;
    new->track = malloc(sizeof(track_t));
    assert(new->track);
    char type[] = "MTrk";
    for (int i = 0; i < 4; i++) {
        assert(getc(file) == type[i]);
    }
    uint8_t length_buff[4] = {};
    for (int i = 0; i < 4; i++) {
        length_buff[i] = getc(file);
    }
    new->track->length = end_swap_32(&length_buff[0]);
    int start = ftell(file);
    int len = new->track->length;
    event_node_t *event_head = malloc(sizeof(event_node_t));
    assert(event_head);
    event_head->event = parse_event(file);
    new->track->event_list = event_head;
    while ((ftell(file) - start) < len) {
        event_head->next_event = malloc(sizeof(event_node_t));
        assert(event_head->next_event);
        event_head = event_head->next_event;
        event_head->next_event = NULL;
        event_head->event = parse_event(file);
    }
    if (!song_data->track_list) {
        song_data->track_list = new;
    } else {
        int n = 2;
        track_node_t *head = song_data->track_list;
        while (head->next_track) {
            head = head->next_track;
            n++;
        }
        head->next_track = new;
    }
} /* parse_track() */

/*
 * Define parse_event here
 */

event_t *parse_event(FILE *file) {
    event_t *new = malloc(sizeof(event_t));
    assert(new);
    new->delta_time = parse_var_len(file);
    uint8_t type = getc(file);
    if (type == 0xF0) {
        new->type = SYS_EVENT_1;
        new->sys_event = parse_sys_event(file, type);
    } else if (type == 0xF7) {
        new->type = SYS_EVENT_2;
        new->sys_event = parse_sys_event(file, type);
    } else if (type == 0xFF) {
        new->type = META_EVENT;
        new->meta_event = parse_meta_event(file);
    } else {
        new->midi_event = parse_midi_event(file, type);
        new->type = new->midi_event.status;
    }
    return new;
} /* parse_event() */

/*
 * Define parse_sys_event here
 */

sys_event_t parse_sys_event(FILE *file, uint8_t type) {
    sys_event_t new = {};
    uint32_t len = parse_var_len(file);
    new.data_len = len;
    if (len) {
        new.data = malloc(len * sizeof(uint8_t));
        for (int i = 0; i < len; i++) {
            new.data[i] = getc(file);
        }
    } else {
        new.data = NULL;
    }
    return new;
} /* parse_sys_event() */

/*
 * Define parse_meta_event here
 */

meta_event_t parse_meta_event(FILE *file) {
    meta_event_t new = {};
    uint8_t type = 0;
    type = getc(file);
    uint32_t len = parse_var_len(file);
    assert(META_TABLE[type].name != NULL);
    assert((META_TABLE[type].data_len == len)
           || (META_TABLE[type].data_len == 0));
    new.name = META_TABLE[type].name;
    new.data_len = len;
    if (len) {
        new.data = malloc(len * sizeof(uint8_t));
        for (int i = 0; i < len; i++) {
            new.data[i] = getc(file);
        }
    } else {
        new.data = NULL;
    }
    return new;
} /* parse_meta_event() */

/*
 * Define parse_midi_event here
 */

midi_event_t parse_midi_event(FILE *file, uint8_t status) {
    midi_event_t new = {};
    uint8_t status_bit = status >> 7;
    if (status_bit) {
        new.status = status;
        g_midi_status = status;
    } else {
        new.status = g_midi_status;
    }
    new.data_len = MIDI_TABLE[new.status].data_len;
    new.name = MIDI_TABLE[new.status].name;
    if (new.data_len) {
        new.data = malloc(new.data_len * sizeof(uint8_t));
        if (!status_bit) {
            new.data[0] = status;
            for (int i = 1; i < new.data_len; i++) {
                new.data[i] = getc(file);
            }
        } else {
            for (int i = !status_bit; i < new.data_len; i++) {
                new.data[i] = getc(file);
            }
        }
    } else {
        new.data = NULL;
    }
    return new;
} /* parse_midi_event() */

/*
 * Define parse_var_len here
 */

uint32_t parse_var_len(FILE *file) {
    uint32_t val = 0;
    uint8_t c = 0;
    if ((val = getc(file)) & 0x80) {
        val &= 0x7f;
        do {
            c = getc(file);
            val = (val << 7) + (c & 0x7f);
        } while (c & 0x80);
    }
    return val;
} /* parse_var_len() */

/*
 * Define event_type here
 */

uint8_t event_type(event_t *event) {
    if ((event->type == SYS_EVENT_1) || (event->type == SYS_EVENT_2)) {
        return SYS_EVENT_T;
    } else if (event->type == META_EVENT) {
        return META_EVENT_T;
    } else {
        return MIDI_EVENT_T;
    }
} /* event_type() */

/*
 * Define free_song here
 */

void free_song(song_data_t *song) {
    free(song->path);
    track_node_t *next = song->track_list->next_track;
    while (song->track_list) {
        free_track_node(song->track_list);
        song->track_list = next;
        if (song->track_list) {
            next = song->track_list->next_track;
        }
    }
    free(song);
} /* free_song() */

/*
 * Define free_track_node here
 */

void free_track_node(track_node_t *track_node) {
    track_node->next_track = NULL;
    event_node_t *next = track_node->track->event_list->next_event;
    while (track_node->track->event_list) {
        free_event_node(track_node->track->event_list);
        track_node->track->event_list = next;
        if (track_node->track->event_list) {
            next = track_node->track->event_list->next_event;
        }
    }
    free(track_node->track);
    free(track_node);
} /* free_track_node() */

/*
 * Define free_event_node here
 */

void free_event_node(event_node_t *event_node) {
    switch (event_type(event_node->event)) {
        case SYS_EVENT_T:
            free(event_node->event->sys_event.data);
            break;
        case META_EVENT_T:
            free(event_node->event->meta_event.data);
            break;
        case MIDI_EVENT_T:
            free(event_node->event->midi_event.data);
            break;
    }
    free(event_node->event);
    free(event_node);
} /* free_event_node() */

/*
 * Define end_swap_16 here
 */

uint16_t end_swap_16(uint8_t buff[2]) {
    uint16_t new = buff[0];
    new <<= 8;
    new |= buff[1];
    return new;
} /* end_swap_16() */

/*
 * Define end_swap_32 here
 */

uint32_t end_swap_32(uint8_t buff[4]) {
    uint32_t new = buff[0];
    new <<= 8;
    new |= buff[1];
    new <<= 8;
    new |= buff[2];
    new <<= 8;
    new |= buff[3];
    return new;
} /* end_swap_32() */
