#include <locus.h>
#include <locus-ui.h>
#include <stdio.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

Locus app;

#define PORTAL_BUS_NAME "org.freedesktop.portal.Desktop"
#define PORTAL_OBJECT_PATH "/org/freedesktop/portal/desktop"
#define PORTAL_FILE_CHOOSER_INTERFACE "org.freedesktop.portal.FileChooser"

static GMainLoop *loop = NULL;
static char *selected_path = NULL;

static char *get_clipboard_content() {
    FILE *fp;
    char *buffer = malloc(4096); 
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for clipboard content.\n");
        return NULL;
    }

    
    fp = popen("wl-paste", "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to run wl-paste command.\n");
        free(buffer);
        return NULL;
    }

    
    if (fgets(buffer, 4096, fp) != NULL) {
        
        buffer[strcspn(buffer, "\n")] = 0;
        pclose(fp);
        return buffer; 
    }

    pclose(fp);
    free(buffer);
    return NULL;
}

static void
handle_response(GDBusConnection *connection,
                const char *sender_name,
                const char *object_path,
                const char *interface_name,
                const char *signal_name,
                GVariant *parameters,
                gpointer user_data)
{
    guint32 response;
    GVariant *results;

    g_variant_get(parameters, "(u@a{sv})", &response, &results);
    
    if (response == 0) { 
        GVariant *uris;
        if (g_variant_lookup(results, "uris", "@as", &uris)) {
            const char **uri_array;
            uri_array = g_variant_get_strv(uris, NULL);
            if (uri_array && uri_array[0]) {
                g_free(selected_path); 
                selected_path = g_filename_from_uri(uri_array[0], NULL, NULL);
            }
            g_variant_unref(uris);
        }
    } else {
        fprintf(stderr, "Error selecting file: response %u\n", response);
    }
    
    g_variant_unref(results);
    g_main_loop_quit(loop);
}

void file_chooser() {
    GError *error = NULL;
    GDBusConnection *connection;
    GDBusProxy *portal;
    char *handle;
    GVariant *ret;

    loop = g_main_loop_new(NULL, FALSE);

    connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (error) {
        g_error("Failed to connect to session bus: %s", error->message);
        g_error_free(error);
        return; 
    }

    portal = g_dbus_proxy_new_sync(connection,
                                   G_DBUS_PROXY_FLAGS_NONE,
                                   NULL,
                                   PORTAL_BUS_NAME,
                                   PORTAL_OBJECT_PATH,
                                   PORTAL_FILE_CHOOSER_INTERFACE,
                                   NULL,
                                   &error);
    if (error) {
        g_error("Failed to create portal proxy: %s", error->message);
        g_error_free(error);
        return; 
    }

    GVariantBuilder opt_builder;
    g_variant_builder_init(&opt_builder, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&opt_builder, "{sv}", "modal", g_variant_new_boolean(TRUE));

    error = NULL;
    ret = g_dbus_proxy_call_sync(portal,
                                 "OpenFile",
                                 g_variant_new("(ssa{sv})",
                                             "",  
                                             "Open File",  
                                             &opt_builder),
                                 G_DBUS_CALL_FLAGS_NONE,
                                 -1,
                                 NULL,
                                 &error);
    if (error) {
        g_error("Failed to open file chooser: %s", error->message);
        g_error_free(error);
        return; 
    }

    g_variant_get(ret, "(o)", &handle);
    g_variant_unref(ret);

    g_dbus_connection_signal_subscribe(connection,
                                       PORTAL_BUS_NAME,
                                       "org.freedesktop.portal.Request",
                                       "Response",
                                       handle,
                                       NULL,
                                       G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                       handle_response,
                                       NULL,
                                       NULL);

    g_main_loop_run(loop);

    g_free(handle);
    g_object_unref(portal);
    g_object_unref(connection);
    g_main_loop_unref(loop);
}

static void launch_mpv(const char *file_path) {
    if (file_path) {
        GError *error = NULL;
        
        char *argv[] = { "mpv", (char *)file_path, NULL };
        if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error)) {
            g_error("Failed to launch MPV: %s", error->message);
            g_error_free(error);
        }
    }
}

void draw(cairo_t *cr, int width, int height) {
    locus_color(cr, 50, 50, 50, 1);
    locus_rectangle(cr, width * 0.50, height * 0.50, width, height, 0, 0);

    locus_color(cr, 255, 178, 90, 0.6);
    locus_rectangle(cr, width * 0.50, height * 0.25, width * 0.80, height * 0.30, 0, 0);
    locus_color(cr, 0, 0, 0, 1);
    locus_text(cr, "Local Files", width * 0.50, height * 0.25, height * 0.05, NORMAL);

    locus_color(cr, 255, 178, 90, 0.6);
    locus_rectangle(cr, width * 0.50, height * 0.75, width * 0.80, height * 0.30, 0, 0);
    locus_color(cr, 0, 0, 0, 1);
    locus_text(cr, "Youtube (Clipboard)", width * 0.50, height * 0.75, height * 0.05, NORMAL);
    
    if (app.state == 1) {
        locus_color(cr, 120, 0, 0, 1);
        locus_text(cr, "Nothing in Clipboard!", width * 0.50, height * 0.85, height * 0.04, NORMAL);
        app.state = 0;
        printf("no");
    }
}

void touch(int id, double x, double y, int state) {
    int width = app.width;
    int height = app.height;

    double rect1_x = width * 0.50 - (width * 0.80 / 2);
    double rect1_y = height * 0.25 - (height * 0.30 / 2);
    double rect1_width = width * 0.80;
    double rect1_height = height * 0.30;

    double rect2_x = width * 0.50 - (width * 0.80 / 2);
    double rect2_y = height * 0.75 - (height * 0.30 / 2);
    double rect2_width = width * 0.80;
    double rect2_height = height * 0.30;

    if (state == 0) {
        if (x >= rect1_x && x <= rect1_x + rect1_width &&
            y >= rect1_y && y <= rect1_y + rect1_height) {
            file_chooser();  
            if (selected_path) { 
                launch_mpv(selected_path); 
            }
        } 
        else if (x >= rect2_x && x <= rect2_x + rect2_width &&
                 y >= rect2_y && y <= rect2_y + rect2_height) {
            char *clipboard_content = get_clipboard_content(); 
            if (clipboard_content) {
                launch_mpv(clipboard_content); 
                free(clipboard_content); 
                app.redraw = 1;
            } else {
                app.state = 1;
                app.redraw = 1;
                fprintf(stderr, "No content to launch with MPV from clipboard.\n");
            }
        }
    }
}

int main() {
    locus_init(&app, 100, 100);
    locus_create_window(&app, "MPV Helper");
    locus_set_draw_callback(&app, draw);
    locus_set_touch_callback(&app, touch);
    locus_run(&app);
    locus_cleanup(&app);

    if (selected_path) {
        g_free(selected_path); 
    }

    return 0;
}
