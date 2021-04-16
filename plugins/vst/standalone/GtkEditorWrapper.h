// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "public.sdk/source/vst/basewrapper/basewrapper.h"
#include <gtk/gtk.h>
#include <gtk/gtkx.h>

using namespace Steinberg;

class GtkEditorWrapper final : public Vst::BaseEditorWrapper {
public:
    explicit GtkEditorWrapper(Vst::IEditController* controller);
    ~GtkEditorWrapper();
    bool open();
    void close();
    void exec();

private:
    GtkWidget* window_ = nullptr;
};
