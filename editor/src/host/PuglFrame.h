// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "PuglHelpers.h"

class PuglFrame {
public:
    void initDefaultSize(int w, int h);
    bool open(void *parentWindowId);
    void close();
    bool isOpen() const;
    void show();
    void hide();
    void repaint();
    void processEvents();

private:
    PuglStatus handleEvent(const PuglEvent *event);

private:
    int defaultWidth_ = 0;
    int defaultHeight_ = 0;
    PuglWorld_u world_;
    PuglView_u view_;
};
