#
if(NOT WIN32 OR MINGW)
    find_package(PkgConfig REQUIRED)
    if(WIN32)
        pkg_check_modules(CAIRO "cairo-win32")
    elseif(APPLE)
        pkg_check_modules(CAIRO "cairo-quartz")
    else()
        pkg_check_modules(CAIRO "cairo-xlib")
    endif()
    add_library(sfizz-cairo INTERFACE)
    target_include_directories(sfizz-cairo INTERFACE ${CAIRO_INCLUDE_DIRS})
    target_link_libraries(sfizz-cairo INTERFACE ${CAIRO_LIBRARIES})
else()
    # TODO...
endif()

#
add_library(sfizz-pugl STATIC EXCLUDE_FROM_ALL
    external/pugl/pugl/detail/implementation.c)
target_include_directories(sfizz-pugl PUBLIC
    external/pugl)
if(WIN32)
    target_sources(sfizz-pugl PRIVATE
        external/pugl/pugl/detail/win.c
        external/pugl/pugl/detail/win_cairo.c)
elseif(APPLE)
    target_sources(sfizz-pugl PRIVATE
        external/pugl/pugl/detail/mac.m
        external/pugl/pugl/detail/mac_cairo.m
        external/pugl/pugl/detail/mac_stub.m)
else()
    target_sources(sfizz-pugl PRIVATE
        external/pugl/pugl/detail/x11.c
        external/pugl/pugl/detail/x11_cairo.c)
endif()
target_link_libraries(sfizz-pugl PUBLIC sfizz-cairo)

if(WIN32)
elseif(APPLE)
    find_library(APPLE_COCOA_LIBRARY "Cocoa")
    target_link_libraries(sfizz-pugl PRIVATE "${APPLE_COCOA_LIBRARY}")
else()
    find_package(X11 REQUIRED)
    target_include_directories(sfizz-pugl PRIVATE "${X11_INCLUDE_DIR}")
    target_link_libraries(sfizz-pugl PRIVATE ${X11_LIBRARIES})
endif()

#
add_library(sfizz_editor STATIC EXCLUDE_FROM_ALL
    src/editor/Editor.cpp
    src/host/PuglFrame.cpp)
target_include_directories(sfizz_editor PUBLIC src)
target_link_libraries(sfizz_editor PUBLIC sfizz-pugl)
