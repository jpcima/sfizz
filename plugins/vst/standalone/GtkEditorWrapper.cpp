// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "GtkEditorWrapper.h"

GtkEditorWrapper::GtkEditorWrapper(Vst::IEditController* controller)
    : BaseEditorWrapper(controller)
{
    gtk_init(nullptr, nullptr);
}

GtkEditorWrapper::~GtkEditorWrapper()
{
}

bool GtkEditorWrapper::open()
{
    close();

    int w = 400;
    int h = 400;

    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_resize(GTK_WINDOW(window), w, h);

    g_signal_connect(
        window, "destroy",
        G_CALLBACK(+[](GtkWidget*, GtkEditorWrapper* self) {
            gtk_main_quit();
        }), this);

    GtkWidget* socket = gtk_socket_new();
    gtk_container_add(GTK_CONTAINER(window), socket);

    gtk_widget_show_all(window);

    void* parentWindow = reinterpret_cast<void*>(gtk_socket_get_id(GTK_SOCKET(socket)));
    if (!_open(parentWindow))
        return false;

    return true;
}

void GtkEditorWrapper::close()
{
    GtkWidget* window = window_;
    if (!window)
        return;

    gtk_widget_destroy(GTK_WIDGET(window));
    window_ = nullptr;
}

void GtkEditorWrapper::exec()
{
    gtk_main();
}
