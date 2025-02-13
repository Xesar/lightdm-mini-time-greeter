/* Functions related to the GUI. */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <lightdm.h>

#include "callbacks.h"
#include "ui.h"
#include "utils.h"


static UI *new_ui(void);
static void setup_background_windows(Config *config, UI *ui);
static GtkWindow *new_background_window(GdkMonitor *monitor);
static void set_window_to_monitor_size(GdkMonitor *monitor, GtkWindow *window);
static void hide_mouse_cursor(GtkWidget *window, gpointer user_data);
static void move_mouse_to_background_window(void);
static void setup_main_window(Config *config, UI *ui);
static void place_main_window(GtkWidget *main_window, gpointer user_data);
static void create_and_attach_layout_container(UI *ui);
static void create_and_attach_password_field(Config *config, UI *ui);
static void create_and_attach_feedback_label(UI *ui);
static void attach_config_colors_to_screen(Config *config);

static void create_and_attach_time_label(UI *ui);
gboolean timer_handler(UI *ui);

/* Initialize the Main Window & it's Children */
UI *initialize_ui(Config *config)
{
    UI *ui = new_ui();

    setup_background_windows(config, ui);
    move_mouse_to_background_window();
    setup_main_window(config, ui);
    create_and_attach_layout_container(ui);
    create_and_attach_password_field(config, ui);
    create_and_attach_time_label(ui);
    create_and_attach_feedback_label(ui);

    g_timeout_add_seconds(1, (GSourceFunc)timer_handler, ui);

    attach_config_colors_to_screen(config);

    return ui;
}


/* Create a new UI with all values initialized to NULL */
static UI *new_ui(void)
{
    UI *ui = malloc(sizeof(UI));
    if (ui == NULL) {
        g_error("Could not allocate memory for UI");
    }
    ui->background_windows = NULL;
    ui->monitor_count = 0;
    ui->main_window = NULL;
    ui->layout_container = NULL;
    ui->password_label = NULL;
    ui->password_input = NULL;
    ui->feedback_label = NULL;
    ui->time_label = NULL;

    return ui;
}


/* Create a Background Window for Every Monitor */
static void setup_background_windows(Config *config, UI *ui)
{
    GdkDisplay *display = gdk_display_get_default();
    ui->monitor_count = gdk_display_get_n_monitors(display);
    ui->background_windows = malloc((uint) ui->monitor_count * sizeof (GtkWindow *));
    for (int m = 0; m < ui->monitor_count; m++) {
        GdkMonitor *monitor = gdk_display_get_monitor(display, m);
        if (monitor == NULL) {
            break;
        }

        GtkWindow *background_window = new_background_window(monitor);
        ui->background_windows[m] = background_window;

        gboolean show_background_image =
            (gdk_monitor_is_primary(monitor) || config->show_image_on_all_monitors) &&
            (strcmp(config->background_image, "\"\"") != 0);
        if (show_background_image) {
            GtkStyleContext *style_context =
                gtk_widget_get_style_context(GTK_WIDGET(background_window));
            gtk_style_context_add_class(style_context, "with-image");
        }
    }
}


/* Create & Configure a Background Window for a Monitor */
static GtkWindow *new_background_window(GdkMonitor *monitor)
{
    GtkWindow *background_window = GTK_WINDOW(gtk_window_new(
        GTK_WINDOW_TOPLEVEL));
    gtk_window_set_type_hint(background_window, GDK_WINDOW_TYPE_HINT_DESKTOP);
    gtk_window_set_keep_below(background_window, TRUE);
    gtk_widget_set_name(GTK_WIDGET(background_window), "background");

    // Set Window Size to Monitor Size
    set_window_to_monitor_size(monitor, background_window);

    g_signal_connect(background_window, "realize", G_CALLBACK(hide_mouse_cursor),
                     NULL);
    // TODO: is this needed?
    g_signal_connect(background_window, "destroy", G_CALLBACK(gtk_main_quit),
                     NULL);

    return background_window;
}


/* Set the Window's Minimum Size to the Default Screen's Size */
static void set_window_to_monitor_size(GdkMonitor *monitor, GtkWindow *window)
{
    GdkRectangle geometry;
    gdk_monitor_get_geometry(monitor, &geometry);
    gtk_widget_set_size_request(
        GTK_WIDGET(window),
        geometry.width,
        geometry.height
    );
    gtk_window_move(window, geometry.x, geometry.y);
    gtk_window_set_resizable(window, FALSE);
}


/* Hide the mouse cursor when it is hovered over the given widget.
 *
 * Note: This has no effect when used with a GtkEntry widget.
 */
static void hide_mouse_cursor(GtkWidget *widget, gpointer user_data)
{
    GdkDisplay *display = gdk_display_get_default();
    GdkCursor *blank_cursor = gdk_cursor_new_for_display(display, GDK_BLANK_CURSOR);
    GdkWindow *window = gtk_widget_get_window(widget);
    if (window != NULL) {
        gdk_window_set_cursor(window, blank_cursor);
    }
}


/* Move the mouse cursor to the upper-left corner of the primary screen.
 *
 * This is necessary for hiding the mouse cursor because we cannot hide the
 * mouse cursor when it is hovered over the GtkEntry password input. Instead,
 * we hide the cursor when it is over the background windows and then move the
 * mouse to the corner of the screen where it should hover over the background
 * window or main window instead.
 */
static void move_mouse_to_background_window(void)
{
    GdkDisplay *display = gdk_display_get_default();
    GdkDevice *mouse = gdk_seat_get_pointer(gdk_display_get_default_seat(display));
    GdkScreen *screen = gdk_display_get_default_screen(display);

    gdk_device_warp(mouse, screen, 0, 0);
}


/* Create & Configure the Main Window */
static void setup_main_window(Config *config, UI *ui)
{
    GtkWindow *main_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));

    gtk_container_set_border_width(GTK_CONTAINER(main_window), config->layout_spacing);
    gtk_widget_set_name(GTK_WIDGET(main_window), "main");

    g_signal_connect(main_window, "show", G_CALLBACK(place_main_window), NULL);
    g_signal_connect(main_window, "realize", G_CALLBACK(hide_mouse_cursor), NULL);
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    ui->main_window = main_window;
}


/* Move the Main Window to the Center of the Primary Monitor
 *
 * This is done after the main window is shown(via the "show" signal) so that
 * the width of the window is properly calculated. Otherwise the returned size
 * will not include the size of the password label text.
 */
static void place_main_window(GtkWidget *main_window, gpointer user_data)
{
    // Get the Geometry of the Primary Monitor
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *primary_monitor = gdk_display_get_primary_monitor(display);
    GdkRectangle primary_monitor_geometry;
    gdk_monitor_get_geometry(primary_monitor, &primary_monitor_geometry);

    // Get the Geometry of the Window
    gint window_width, window_height;
    gtk_window_get_size(GTK_WINDOW(main_window), &window_width, &window_height);

    gtk_window_move(
        GTK_WINDOW(main_window),
        primary_monitor_geometry.x + primary_monitor_geometry.width / 2 - window_width / 2,
        primary_monitor_geometry.y + primary_monitor_geometry.height / 2 - window_height / 2);
}


/* Add a Layout Container for All Displayed Widgets */
static void create_and_attach_layout_container(UI *ui)
{
    ui->layout_container = GTK_GRID(gtk_grid_new());
    gtk_grid_set_column_spacing(ui->layout_container, 5);
    gtk_grid_set_row_spacing(ui->layout_container, 5);

    gtk_container_add(GTK_CONTAINER(ui->main_window),
                      GTK_WIDGET(ui->layout_container));
}


/* Add a label & entry field for the user's password.
 *
 * If the `show_password_label` member of `config` is FALSE,
 * `ui->password_label` is left as NULL.
 */
static void create_and_attach_password_field(Config *config, UI *ui)
{
    ui->password_input = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(ui->password_input), FALSE);
    gtk_entry_set_alignment(GTK_ENTRY(ui->password_input),
                            (gfloat) config->password_alignment);
    // TODO: The width is usually a little shorter than we specify. Is there a
    // way to force this exact character width?
    // Maybe use 2 GtkBoxes instead of a GtkGrid?
    gtk_entry_set_width_chars(GTK_ENTRY(ui->password_input),
                              config->password_input_width);
    gtk_widget_set_name(GTK_WIDGET(ui->password_input), "password");
    gtk_grid_attach(ui->layout_container, ui->password_input, 0, 0, 1, 1);

    if (config->show_password_label) {
        ui->password_label = gtk_label_new(config->password_label_text);
        gtk_label_set_justify(GTK_LABEL(ui->password_label), GTK_JUSTIFY_RIGHT);
        gtk_grid_attach_next_to(ui->layout_container, ui->password_label,
                                ui->password_input, GTK_POS_LEFT, 1, 1);
    }
}


/* Add a label for feedback to the user */
static void create_and_attach_feedback_label(UI *ui)
{
    ui->feedback_label = gtk_label_new("");
    gtk_label_set_justify(GTK_LABEL(ui->feedback_label), GTK_JUSTIFY_CENTER);
    gtk_widget_set_no_show_all(ui->feedback_label, TRUE);
    gtk_widget_set_name(GTK_WIDGET(ui->feedback_label), "error");

    GtkWidget *attachment_point;
    gint width;
    if (ui->password_label == NULL)
        width = 1;
    else
        width = 2;

    attachment_point = ui->time_label;
    gtk_grid_attach_next_to(ui->layout_container, ui->feedback_label,
                            attachment_point, GTK_POS_BOTTOM, width, 1);
}


static void create_and_attach_time_label(UI *ui)
{
    ui->time_label = gtk_label_new("");
	timer_handler(ui);
    gtk_label_set_justify(GTK_LABEL(ui->time_label), GTK_JUSTIFY_CENTER);

    GtkWidget *attachment_point;
    gint width;
    if (ui->password_label == NULL) {
        attachment_point = ui->password_input;
        width = 1;
    } else {
        attachment_point = ui->password_label;
        width = 2;
    }

    gtk_grid_attach_next_to(ui->layout_container, ui->time_label,
                            attachment_point, GTK_POS_BOTTOM, width, 1);
}

gboolean timer_handler(UI *ui){
    GDateTime *date_time;
    gchar *dt_format;

    date_time = g_date_time_new_now_local();

    if (ui->password_label == NULL) {
        dt_format = g_date_time_format(date_time, "%H:%M:%S");
    } else {
        dt_format = g_date_time_format(date_time, "%a, %d-%m-%g %H:%M:%S");
    }

    gtk_label_set_text(GTK_LABEL(ui->time_label), dt_format);
    g_free(dt_format);
    
    return TRUE;
}


/* Attach a style provider to the screen, using color options from config */
static void attach_config_colors_to_screen(Config *config)
{
    GtkCssProvider* provider = gtk_css_provider_new();

    GdkRGBA *caret_color;
    if (config->show_input_cursor) {
        caret_color = config->password_color;
    } else {
        caret_color = config->password_background_color;
    }

    char *css;
    int css_string_length = asprintf(&css,
        "* {\n"
            "font-family: %s;\n"
            "font-size: %s;\n"
            "font-weight: %s;\n"
            "font-style: %s;\n"
        "}\n"
        "label {\n"
            "color: %s;\n"
        "}\n"
        "label#error {\n"
            "color: %s;\n"
        "}\n"
        "#background {\n"
            "background-color: %s;\n"
        "}\n"
        "#background.with-image {\n"
            "background-image: image(url(%s), %s);\n"
            "background-repeat: no-repeat;\n"
            "background-position: center;\n"
        "}\n"
        "#main, #password {\n"
            "border-width: %s;\n"
            "border-color: %s;\n"
            "border-style: solid;\n"
        "}\n"
        "#main {\n"
            "background-color: %s;\n"
        "}\n"
        "#password {\n"
            "color: %s;\n"
            "caret-color: %s;\n"
            "background-color: %s;\n"
            "border-width: %s;\n"
            "border-color: %s;\n"
            "background-image: none;\n"
            "box-shadow: none;\n"
            "border-image-width: 0;\n"
        "}\n"
        // *
        , config->font
        , config->font_size
        , config->font_weight
        , config->font_style
        // label
        , gdk_rgba_to_string(config->text_color)
        // label#error
        , gdk_rgba_to_string(config->error_color)
        // #background
        , gdk_rgba_to_string(config->background_color)
        // #background.image-background
        , config->background_image
        , gdk_rgba_to_string(config->background_color)
        // #main, #password
        , config->border_width
        , gdk_rgba_to_string(config->border_color)
        // #main
        , gdk_rgba_to_string(config->window_color)
        // #password
        , gdk_rgba_to_string(config->password_color)
        , gdk_rgba_to_string(caret_color)
        , gdk_rgba_to_string(config->password_background_color)
        , config->password_border_width
        , gdk_rgba_to_string(config->password_border_color)
    );

    if (css_string_length >= 0) {
        gtk_css_provider_load_from_data(provider, css, -1, NULL);

        GdkScreen *screen = gdk_screen_get_default();
        gtk_style_context_add_provider_for_screen(
            screen, GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER + 1);
    }


    g_object_unref(provider);
}
