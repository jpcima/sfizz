// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <pugl/pugl.h>
#include <memory>

struct PuglWorldDeleter {
    void operator()(PuglWorld *x) const { puglFreeWorld(x); }
};

struct PuglViewDeleter {
    void operator()(PuglView *x) const { puglFreeView(x); }
};

typedef std::unique_ptr<PuglWorld, PuglWorldDeleter> PuglWorld_u;
typedef std::unique_ptr<PuglView, PuglViewDeleter> PuglView_u;
