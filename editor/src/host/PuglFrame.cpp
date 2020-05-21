// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "PuglFrame.h"
#include <pugl/pugl_cairo.h>
#include <cstdio>

void PuglFrame::initDefaultSize(int w, int h)
{
    defaultWidth_ = w;
    defaultHeight_ = h;
}

bool PuglFrame::open(void *parentWindowId)
{
    close();

    PuglWorld *world = puglNewWorld(PUGL_MODULE, PUGL_WORLD_THREADS);
    if (!world) {
        close();
        return false;
    }
    world_.reset(world);
    puglSetWorldHandle(world, this);

    PuglView *view = puglNewView(world);
    if (!view) {
        close();
        return false;
    }
    view_.reset(view);
    puglSetHandle(view, this);

    auto eventFunc = +[](PuglView *view, const PuglEvent *event) -> PuglStatus {
        PuglFrame *self = reinterpret_cast<PuglFrame *>(puglGetHandle(view));
        return self->handleEvent(event);
    };

    if (puglSetEventFunc(view, eventFunc) != PUGL_SUCCESS) {
        close();
        return false;
    }

    if (puglSetBackend(view, puglCairoBackend()) != PUGL_SUCCESS) {
        close();
        return false;
    }

    if (puglSetDefaultSize(view, defaultWidth_, defaultHeight_) != PUGL_SUCCESS) {
        close();
        return false;
    }

    PuglRect childFrame {};
    childFrame.width = defaultWidth_;
    childFrame.height = defaultHeight_;
    if (puglSetFrame(view, childFrame) != PUGL_SUCCESS) {
        close();
        return false;
    }

    if (puglSetParentWindow(view, reinterpret_cast<PuglNativeView>(parentWindowId)) != PUGL_SUCCESS) {
        close();
        return false;
    }

    if (puglRealize(view) != PUGL_SUCCESS) {
        close();
        return false;
    }

    puglShowWindow(view);

    return true;
}

void PuglFrame::close()
{
    view_.reset();
    world_.reset();
}

bool PuglFrame::isOpen() const
{
    return view_ != nullptr;
}

void PuglFrame::show()
{
    if (!view_)
        return;

    puglShowWindow(view_.get());
}

void PuglFrame::hide()
{
    if (!view_)
        return;

    puglHideWindow(view_.get());
}

void PuglFrame::repaint()
{
    if (!view_)
        return;

    puglPostRedisplay(view_.get());
}

void PuglFrame::processEvents()
{
    if (!view_)
        return;

    puglUpdate(world_.get(), 0.0);
}

PuglStatus PuglFrame::handleEvent(const PuglEvent *event)
{
    //TODO handle event

    switch (event->type) {
    }

    return PUGL_SUCCESS;
}
