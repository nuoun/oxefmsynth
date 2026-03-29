cmake_minimum_required(VERSION 3.25.0)

# Ninja is a single-config generator: --config at build time is ignored.
# Default to Release so the SDK's fdebug.h gets NDEBUG and doesn't error out.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

project(oxefmsynth
    VERSION 1.3.6
    DESCRIPTION "Oxe FM Synth"
    HOMEPAGE_URL "http://www.oxesoft.com"
)

set(vst3sdk_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vst3sdk")

# Validator tool requires WinMain and doesn't build cleanly under MinGW.
# It's only useful for CI testing of the plugin; disable by default.
option(SMTG_RUN_VST_VALIDATOR "Run VST validator on VST3 Plug-ins" OFF)

list(APPEND CMAKE_MODULE_PATH "${vst3sdk_SOURCE_DIR}/cmake/modules")

include(SMTG_VST3_SDK)

smtg_setup_platform_toolset()
smtg_setup_symbol_visibility()

set(SDK_ROOT "${vst3sdk_SOURCE_DIR}")
set(public_sdk_SOURCE_DIR "${SDK_ROOT}/public.sdk")
set(pluginterfaces_SOURCE_DIR "${SDK_ROOT}/pluginterfaces")

add_subdirectory(${vst3sdk_SOURCE_DIR} vst3sdk EXCLUDE_FROM_ALL)

# MinGW on Windows exposes _aligned_malloc but not std::aligned_alloc.
# The SDK's alignedalloc.h only guards for _MSC_VER, missing MinGW.
# Fix without touching the submodule: generate a polyfill header and
# force-include it into every SDK compilation unit that needs it.
if(MINGW)
    set(_MINGW_COMPAT "${CMAKE_BINARY_DIR}/mingw_compat.h")
    file(WRITE "${_MINGW_COMPAT}" [=[
#pragma once
#if defined(__MINGW32__) || defined(__MINGW64__)
#include <malloc.h>
namespace std {
  // std::aligned_alloc(alignment, size) -- note argument order matches the C standard
  inline void* aligned_alloc(size_t alignment, size_t size) {
    return ::_aligned_malloc(size, alignment);
  }
}
#endif
]=])
    foreach(_t sdk sdk_common sdk_hosting)
        if(TARGET ${_t})
            target_compile_options(${_t} PRIVATE "-include${_MINGW_COMPAT}")
        endif()
    endforeach()
endif()

include_directories(${SDK_ROOT})

smtg_add_vst3plugin(oxefmsynth
    "${CMAKE_CURRENT_SOURCE_DIR}/src/vst3/oxevst3factory.cpp"
)

target_sources(oxefmsynth PRIVATE
    # VST3 wrapper — SingleComponentEffect is not part of libsdk; add it directly
    "${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/vstsinglecomponenteffect.cpp"
    src/vst3/oxevst3.cpp
    src/vst3/oxevst3.h
    src/vst3/oxevst3editor.cpp
    src/vst3/oxevst3editor.h
    src/vst3/vsthostinterface3.cpp
    src/vst3/vsthostinterface3.h

    # Synthesizer engine
    src/synth/buffers.cpp
    src/synth/delay.cpp
    src/synth/envelop.cpp
    src/synth/filter.cpp
    src/synth/noise.cpp
    src/synth/note.cpp
    src/synth/oscillator.cpp
    src/synth/persist.cpp
    src/synth/programs.cpp
    src/synth/reverb.cpp
    src/synth/synthesizer.cpp

    # GUI
    src/gui/button.cpp
    src/gui/channels.cpp
    src/gui/editor.cpp
    src/gui/key.cpp
    src/gui/knob.cpp
    src/gui/lcd.cpp
    src/gui/mapper.cpp
)

target_include_directories(oxefmsynth PRIVATE
    src/vst3
    src/synth
    src/gui
    src/toolkits
    src/windows
)

target_link_libraries(oxefmsynth PRIVATE
    sdk
    base
    pluginterfaces
)

if(WIN32)
    target_sources(oxefmsynth PRIVATE
        src/toolkits/windowstoolkit.cpp
        src/windows/resources.rc
    )
    # windres needs to find resources.h, constants.h, and the bitmap/icon files
    set_source_files_properties(src/windows/resources.rc PROPERTIES
        COMPILE_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/src/windows -I${CMAKE_CURRENT_SOURCE_DIR}/src/synth -I${CMAKE_CURRENT_SOURCE_DIR}/skins/default"
    )
    target_link_libraries(oxefmsynth PRIVATE gdi32)
elseif(APPLE)
    target_sources(oxefmsynth PRIVATE
        src/toolkits/cocoatoolkit.cpp
        src/toolkits/cocoatoolkit.m
        src/toolkits/opengltoolkit.cpp
    )
    target_link_libraries(oxefmsynth PRIVATE
        "-framework Cocoa"
        "-framework OpenGL"
    )
elseif(UNIX)
    target_sources(oxefmsynth PRIVATE
        src/toolkits/xlibtoolkit.cpp
    )
    target_link_libraries(oxefmsynth PRIVATE X11 pthread dl)
    target_compile_options(oxefmsynth PRIVATE -fPIC -D__cdecl=)

    # Embed skin bitmaps into the binary on Linux (matches old Makefile.vstlinux behaviour)
    find_program(EMBEDRESOURCES embedresources PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src")
    if(EMBEDRESOURCES)
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/bitmaps.cpp"
            COMMAND ${EMBEDRESOURCES}
                    "${CMAKE_CURRENT_SOURCE_DIR}/skins/default"
                    "${CMAKE_CURRENT_BINARY_DIR}/bitmaps.cpp"
            COMMENT "Embedding skin bitmaps"
        )
        target_sources(oxefmsynth PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/bitmaps.cpp")
    endif()
endif()

smtg_target_add_plugin_resources(oxefmsynth
    RESOURCES
        "skins/default/bg.bmp"
        "skins/default/chars.bmp"
        "skins/default/buttons.bmp"
        "skins/default/key.bmp"
        "skins/default/knob.bmp"
        "skins/default/knob2.bmp"
        "skins/default/knob3.bmp"
        "skins/default/ops.bmp"
)

smtg_target_configure_version_file(oxefmsynth)
