#include <gtk/gtk.h>
#include <stdlib.h>

#include "include/ui.h"

int main(int argc, char* argv[]) {
    setenv("DISPLAY", "127.0.0.1:0", true);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    GtkApplication* app = gtk_application_new("org.purdue.cs240proj", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
