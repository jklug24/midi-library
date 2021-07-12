/* Name, ui.c, CS 24000, Spring 2020
 * Last updated April 9, 2020
 */

/* Add any includes here */
#include "../include/ui.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

struct ui_widgets {
    GtkWidget *list_box;
    GtkWidget *file_info;
    GtkWidget *t_scale;
    GtkWidget *original_area;
    GtkWidget *after_area;
    GtkWidget *warp_t;
    GtkWidget *change_o;
    GtkWidget *map_i;
} g_widgets;

struct note_on {
    event_t *event;
    uint32_t time_start;
};

tree_node_t *g_current_node;
song_data_t *g_current_song;
song_data_t *g_modified_song;

/* Define update_song_list here */
void update_song_list() {
    return;
}

/* Define update_drawing_area here */
void update_drawing_area() {
    gdk_window_invalidate_rect(gtk_widget_get_window(g_widgets.original_area), NULL, TRUE);
    gdk_window_invalidate_rect(gtk_widget_get_window(g_widgets.after_area), NULL, TRUE);
}

void update_modified_drawing_area() {
    deep_copy_song(g_current_song, &g_modified_song);
    float multiplier = gtk_spin_button_get_value(GTK_SPIN_BUTTON(g_widgets.warp_t));
    int octave = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(g_widgets.change_o));
    warp_time(g_modified_song, multiplier);
    change_octave(g_modified_song, octave);
    gdk_window_invalidate_rect(gtk_widget_get_window(g_widgets.after_area), NULL, TRUE);
}

/* Define update_info here */
void update_info() {
    char file_info_string[MAX_BUFF];
    int min = -1;
    int max = -1;
    int len = 0;
    range_of_song(g_current_node->song, &min, &max, &len);
    sprintf(file_info_string, "File name: %s\nFull path: %s\nNote range: [%d, %d]\nOriginal length: %d",
            g_current_node->song_name, g_current_node->song->path, min, max, len);
    gtk_label_set_text(GTK_LABEL(g_widgets.file_info), file_info_string);
    gtk_widget_set_sensitive(g_widgets.t_scale, TRUE);
    gtk_widget_set_sensitive(g_widgets.warp_t, TRUE);
    gtk_widget_set_sensitive(g_widgets.change_o, TRUE);
    gtk_widget_set_sensitive(g_widgets.map_i, TRUE);
}

/* Define update_song here */

/* Define range_of_song here */
void range_of_song(song_data_t *song, int *min, int *max, int *len) {
    assert(song);
    track_node_t *track_node = song->track_list;
    while (track_node) {
        int track_len = 0;
        event_node_t *event_node = track_node->track->event_list;
        while(event_node) {
            track_len += event_node->event->delta_time;
            if (event_type(event_node->event) == MIDI_EVENT_T) {
                uint8_t status = event_node->event->type & 0xF0;
                if ((status == 0xA0) || (status == 0x90) || (status == 0x80)) {
                    if (*min == -1 || event_node->event->midi_event.data[0] < *min) {
                        *min = event_node->event->midi_event.data[0];
                    }
                    if (*max == -1 || event_node->event->midi_event.data[0] > *max) {
                        *max = event_node->event->midi_event.data[0];
                    }
                }
            }
            event_node = event_node->next_event;
        }
        if (song->format == 1) {
            *len = (*len > track_len) ? *len : track_len;
        } else {
            *len += track_len;
        }
        track_node = track_node->next_track;
    }
}

/* Define activate here */
void activate(GtkApplication *app, gpointer data) {
    g_current_node = NULL;
    g_current_song = NULL;
    g_modified_song = NULL;

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW (window), "Midi Library");
    gtk_window_set_default_size(GTK_WINDOW (window), 200, 400);

    GtkWidget *full_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_add(GTK_CONTAINER(window), full_box);


    GtkWidget *left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(full_box), left_box, FALSE, FALSE, 5);

    GtkWidget *top_left_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(left_box), top_left_box, FALSE, FALSE, 5);

    GtkWidget *add_song_button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(top_left_box), add_song_button_box, TRUE, FALSE, 0);

    GtkWidget *button_add_song = gtk_button_new_with_label("Add Song from File");
    g_signal_connect(button_add_song, "clicked", G_CALLBACK(add_song_cb), app);
    gtk_container_add(GTK_CONTAINER(add_song_button_box), button_add_song);

    GtkWidget *load_dir_button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(top_left_box), load_dir_button_box, TRUE, FALSE, 0);

    GtkWidget *button_load_dir = gtk_button_new_with_label("Load from Directory");
    g_signal_connect(button_load_dir, "clicked", G_CALLBACK(load_songs_cb), NULL);
    gtk_container_add(GTK_CONTAINER(load_dir_button_box), button_load_dir);

    g_widgets.list_box = gtk_list_box_new();
    gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(g_widgets.list_box), TRUE);
    g_signal_connect(g_widgets.list_box, "row-activated", G_CALLBACK(song_selected_cb), NULL);
    GtkWidget *list_placeholder = gtk_label_new("Load files to start...");
    gtk_list_box_set_placeholder(GTK_LIST_BOX(g_widgets.list_box), list_placeholder);
    gtk_widget_show(list_placeholder);
    gtk_box_pack_start(GTK_BOX(left_box), g_widgets.list_box, TRUE, TRUE, 0);

    GtkWidget *search_bar = gtk_search_entry_new();
    gtk_box_pack_start(GTK_BOX(left_box), search_bar, FALSE, FALSE, 5);


    gtk_box_pack_start(GTK_BOX(full_box), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 5);


    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(full_box), right_box, TRUE, TRUE, 5);

    g_widgets.file_info = gtk_label_new("Select a file from the list to start...\n\n\n");
    gtk_box_pack_start(GTK_BOX(right_box), g_widgets.file_info, FALSE, FALSE, 5);

    GtkWidget *t_scale_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(right_box), t_scale_box, FALSE, FALSE, 5);
    GtkWidget *t_scale_label = gtk_label_new("T scale:");
    gtk_box_pack_start(GTK_BOX(t_scale_box), t_scale_label, TRUE, TRUE, 5);
    g_widgets.t_scale = gtk_spin_button_new_with_range(1, 10000, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_widgets.t_scale), 10);
    g_signal_connect(G_OBJECT(g_widgets.t_scale), "value-changed", G_CALLBACK(time_scale_cb), NULL);
    gtk_widget_set_sensitive(g_widgets.t_scale, FALSE);
    gtk_box_pack_start(GTK_BOX(t_scale_box), g_widgets.t_scale, TRUE, TRUE, 5);

    GtkWidget *original_frame = gtk_frame_new("Original song:");
    gtk_widget_set_size_request(original_frame, 600, 200);
    gtk_box_pack_start(GTK_BOX(right_box), original_frame, TRUE, TRUE, 5);
    GtkWidget *original_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(original_frame), original_scroll);
    g_widgets.original_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(original_scroll), g_widgets.original_area);
    gtk_widget_show(g_widgets.original_area);
    g_signal_connect(G_OBJECT(g_widgets.original_area), "draw", G_CALLBACK(draw_cb), &g_current_song);

    GtkWidget *after_frame = gtk_frame_new("After effect:");
    gtk_widget_set_size_request(after_frame, 600, 200);
    gtk_box_pack_start(GTK_BOX(right_box), after_frame, TRUE, TRUE, 5);
    GtkWidget *after_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(after_frame), after_scroll);
    g_widgets.after_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(after_scroll), g_widgets.after_area);
    g_signal_connect(G_OBJECT(g_widgets.after_area), "draw", G_CALLBACK(draw_cb), &g_modified_song);

    GtkWidget *warp_time_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(right_box), warp_time_box, FALSE, FALSE, 5);
    GtkWidget *warp_time_label = gtk_label_new("Warp time:");
    gtk_box_pack_start(GTK_BOX(warp_time_box), warp_time_label, TRUE, TRUE, 5);
    g_widgets.warp_t = gtk_spin_button_new_with_range(.1, 10, .1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_widgets.warp_t), 1);
    g_signal_connect(G_OBJECT(g_widgets.warp_t), "value-changed", G_CALLBACK(warp_time_cb), NULL);
    gtk_widget_set_sensitive(g_widgets.warp_t, FALSE);
    gtk_box_pack_start(GTK_BOX(warp_time_box), g_widgets.warp_t, TRUE, TRUE, 5);

    GtkWidget *change_o_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(right_box), change_o_box, FALSE, FALSE, 5);
    GtkWidget *change_o_label = gtk_label_new("Change octave:");
    gtk_box_pack_start(GTK_BOX(change_o_box), change_o_label, TRUE, TRUE, 5);
    g_widgets.change_o = gtk_spin_button_new_with_range(-5, 5, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_widgets.change_o), 0);
    g_signal_connect(G_OBJECT(g_widgets.change_o), "value-changed", G_CALLBACK(song_octave_cb), NULL);
    gtk_widget_set_sensitive(g_widgets.change_o, FALSE);
    gtk_box_pack_start(GTK_BOX(change_o_box), g_widgets.change_o, TRUE, TRUE, 5);

    GtkWidget *map_i_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(right_box), map_i_box, FALSE, FALSE, 5);
    GtkWidget *map_i_label = gtk_label_new("Map instrument:");
    gtk_box_pack_start(GTK_BOX(map_i_box), map_i_label, TRUE, TRUE, 5);
    g_widgets.map_i = gtk_combo_box_text_new();
    g_signal_connect(G_OBJECT(g_widgets.map_i), "changed", G_CALLBACK(instrument_map_cb), NULL);
    gtk_widget_set_sensitive(g_widgets.change_o, FALSE);
    gtk_box_pack_start(GTK_BOX(map_i_box), g_widgets.map_i, TRUE, TRUE, 5);

    gtk_widget_show_all(window);
}

/* Define add_song_cb here */
void add_song_cb(GtkButton *button, gpointer app) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Add Song");
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res = 0;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File",
                                                    GTK_WINDOW(window), action, "Cancel", GTK_RESPONSE_CANCEL,
                                                    "Open", GTK_RESPONSE_ACCEPT, NULL);
    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);
        if ((filename[strlen(filename) - 4] == '.') &&
            (filename[strlen(filename) - 3] == 'm') &&
            (filename[strlen(filename) - 2] == 'i') &&
            (filename[strlen(filename) - 1] == 'd')) {
            tree_node_t *new = malloc(sizeof(tree_node_t));
            new->song = parse_file(filename);
            new->left_child = NULL;
            new->right_child = NULL;
            new->song_name = new->song->path;
            char *ptr = NULL;
            while ((ptr = strchr(new->song_name, '/')) != NULL) {
                new->song_name = ptr + 1;
            }
            tree_insert(&g_song_library, new);

            GtkWidget *label = gtk_label_new(new->song_name);
            gtk_widget_show(label);
            gtk_list_box_insert(GTK_LIST_BOX(g_widgets.list_box), label, -1);
        }
    }
    gtk_widget_destroy(dialog);
}

/* Define load_songs_cb here */
void load_songs_cb(GtkButton *button, gpointer app) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Add Song");
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res = 0;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File",
                                                    GTK_WINDOW(window), action, "Cancel", GTK_RESPONSE_CANCEL,
                                                    "Open", GTK_RESPONSE_ACCEPT, NULL);
    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        char *dir;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        dir = gtk_file_chooser_get_filename(chooser);
        make_library(dir);
        update_song_list();
    }
}

/* Define song_selected_cb here */
void song_selected_cb(GtkListBox *box, GtkListBoxRow *row) {
    const char *filename = gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(row))));
    g_current_node = *find_parent_pointer(&g_song_library, filename);
    g_current_song = g_current_node->song;
    deep_copy_song(g_current_song, &g_modified_song);
    update_info();
    update_drawing_area();
}

/* Define search_bar_cb here */

/* Define time_scale_cb here */
void time_scale_cb(GtkSpinButton *sb, gpointer data) {
    update_drawing_area();
}

/* Define draw_cb here */
gboolean draw_cb(GtkDrawingArea *widget, cairo_t *cr, gpointer data) {
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    song_data_t **song_ptr = (song_data_t **)data;

    if (!(*song_ptr)) {
        return TRUE;
    }

    song_data_t *song = *song_ptr;

    int t_scale = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(g_widgets.t_scale));
    int min = -1;
    int max = -1;
    int len = 0;
    int dummy_len = 0;

    range_of_song(song, &min, &max, &len);
    range_of_song(g_current_song, &min, &max, &dummy_len);
    range_of_song(g_modified_song, &min, &max, &dummy_len);

    int width = len / t_scale;
    int height = gtk_widget_get_allocated_height(GTK_WIDGET(widget));
    float div = (float) (height - (2 * PIXEL_BUF)) / ((float)max - (float)min);
    gtk_widget_set_size_request(GTK_WIDGET(widget), width, height);

    struct note_on notes[16][128] = {};
    uint8_t instrument[16] = {};
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 128; j++) {
            notes[i][j].event = NULL;
            notes[i][j].time_start = -1;
        }
        instrument[i] = 0;
    }
    instrument[10] = 113;


    int time = 0;
    double middle_c = height - ((60 - min) * div) - PIXEL_BUF;
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, 0, middle_c);
    cairo_line_to(cr, width, middle_c);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);
    cairo_set_line_width(cr, 3);

    track_node_t *track = song->track_list;
    while (track) {
        event_node_t *event_n = track->track->event_list;
        while (event_n) {
            event_t *event = event_n->event;
            if (event_type(event) == MIDI_EVENT_T) {

                if (((event->type & 0xF0) == 0x80) ||
                    (((event->type & 0xF0) == 0x90) && (event->midi_event.data[1] == 0))) {
                    int chan = event->type & 0x0F;
                    int note = event->midi_event.data[0] & 0x7F;
                    if (notes[chan][note].event != NULL) {
                        cairo_set_source_rgb(cr, COLOR_PALETTE[instrument[chan]]->red,
                                             COLOR_PALETTE[instrument[chan]]->green,
                                             COLOR_PALETTE[instrument[chan]]->blue);
                        double start = notes[chan][note].time_start / t_scale;
                        double y = height - (((notes[chan][note].event->midi_event.data[0] & 0x7F) - min) * div) - PIXEL_BUF;
                        double end = time / t_scale;
                        cairo_move_to(cr, start, y);
                        cairo_line_to(cr ,end, y);
                        cairo_stroke(cr);
                        notes[chan][note].event = NULL;
                        //return TRUE;
                    }

                } else if ((event->type & 0xF0) == 0x90) {
                    int chan = event->type & 0x0F;
                    int note = event->midi_event.data[0] & 0x7F;
                    notes[chan][note].event = event;
                    notes[chan][note].time_start = time;

                } else if ((event->type & 0xF0) == 0xC0) {
                    int chan = event->type & 0x0F;
                    instrument[chan] = event->midi_event.data[0] & 0x7F;
                }
            }
            time += event->delta_time;
            event_n = event_n->next_event;
        }
        if (song->format == 1) {
            time = 0;
        }
        track = track->next_track;
    }
    return TRUE;
}

/* Define warp_time_cb here */
void warp_time_cb(GtkSpinButton *sb, gpointer data) {
    update_modified_drawing_area();
}

/* Define song_octave_cb here */
void song_octave_cb(GtkSpinButton *sb, gpointer data) {
    update_modified_drawing_area();
}

/* Define instrument_map_cb here */
void instrument_map_cb(GtkComboBoxText *cbt, gpointer data) {
    update_modified_drawing_area();
}

/* Define note_map_cb here */

/* Define save_song_cb here */

/* Define remove_song_cb here */

/*
 * Function called prior to main that sets up the instrument to color mapping
 */

void build_color_palette()
{
    static GdkRGBA palette[16];

    memset(COLOR_PALETTE, 0, sizeof(COLOR_PALETTE));
    char* color_specs[] = {
            // Piano, red
            "#ff0000",
            // Chromatic percussion, brown
            "#8b4513",
            // Organ, purple
            "#800080",
            // Guitar, green
            "#00ff00",
            // Bass, blue
            "#0000ff",
            // Strings, cyan
            "#00ffff",
            // Ensemble, teal
            "#008080",
            // Brass, orange
            "#ffa500",
            // Reed, magenta
            "#ff00ff",
            // Pipe, yellow
            "ffff00",
            // Synth lead, indigo
            "#4b0082",
            // Synth pad, dark slate grar
            "#2f4f4f",
            // Synth effects, silver
            "#c0c0c0",
            // Ehtnic, olive
            "#808000",
            // Percussive, silver
            "#c0c0c0",
            // Sound effects, gray
            "#808080",
    };

    for (int i = 0; i < 16; ++i) {
        gdk_rgba_parse(&palette[i], color_specs[i]);
        for (int j = 0; j < 8; ++j) {
            COLOR_PALETTE[i * 8 + j] = &palette[i];
        }
    }
} /* build_color_palette() */
