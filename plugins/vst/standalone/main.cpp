// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "JACKWrapper.h"
#include "GtkEditorWrapper.h"
#include <cstdio>
#include <cassert>

extern bool InitModule();
extern bool DeinitModule();

int main(int argc, char* argv[])
{
    if (!InitModule())
        assert(false);

    JACKWrapper wrapper;

    if (!wrapper.init()) {
        fprintf(stderr, "Cannot start a JACK client.\n");
        return 1;
    }

    GtkEditorWrapper editorWrapper(wrapper.getController());
    editorWrapper.open();

    wrapper.activate();

    editorWrapper.exec();

    editorWrapper.close();

    DeinitModule();

    return 0;
}
