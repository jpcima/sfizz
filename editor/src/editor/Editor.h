// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <memory>

class Editor {
public:
    static constexpr int fixedWidth = 640;
    static constexpr int fixedHeight = 480;

    Editor();
    ~Editor();

    bool open(void* parentWindowId);
    void close();
    bool isOpen() const;

    void show();
    void hide();

    void processEvents();

    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
};
