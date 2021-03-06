#pragma once

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>

struct cached_atom {
  const char* name;
  size_t len;
  xcb_atom_t* atom;
};

static xcb_atom_t _NET_WM_NAME;
static xcb_atom_t _NET_WM_DESKTOP;
static xcb_atom_t _NET_WM_WINDOW_TYPE;
static xcb_atom_t _NET_WM_WINDOW_TYPE_DOCK;
static xcb_atom_t _NET_WM_WINDOW_TYPE_NORMAL;
static xcb_atom_t _NET_WM_PID;
static xcb_atom_t _NET_WM_STATE;
static xcb_atom_t _NET_WM_STATE_STICKY;
static xcb_atom_t _NET_WM_STATE_SKIP_TASKBAR;
static xcb_atom_t _NET_WM_STATE_ABOVE;
static xcb_atom_t _NET_WM_STATE_MAXIMIZED_VERT;
static xcb_atom_t _NET_WM_STRUT;
static xcb_atom_t _NET_WM_STRUT_PARTIAL;
static xcb_atom_t WM_PROTOCOLS;
static xcb_atom_t WM_DELETE_WINDOW;
static xcb_atom_t _XEMBED;
static xcb_atom_t _XEMBED_INFO;
static xcb_atom_t _NET_SYSTEM_TRAY_OPCODE;
static xcb_atom_t MANAGER;
static xcb_atom_t WM_STATE;
static xcb_atom_t _NET_SYSTEM_TRAY_ORIENTATION;
static xcb_atom_t WM_TAKE_FOCUS;
static xcb_atom_t Backlight;
static xcb_atom_t BACKLIGHT;

// clang-format off
static cached_atom ATOMS[24] = {
    {"_NET_WM_NAME", sizeof("_NET_WM_NAME") - 1, &_NET_WM_NAME},
    {"_NET_WM_DESKTOP", sizeof("_NET_WM_DESKTOP") - 1, &_NET_WM_DESKTOP},
    {"_NET_WM_WINDOW_TYPE", sizeof("_NET_WM_WINDOW_TYPE") - 1, &_NET_WM_WINDOW_TYPE},
    {"_NET_WM_WINDOW_TYPE_DOCK", sizeof("_NET_WM_WINDOW_TYPE_DOCK") - 1, &_NET_WM_WINDOW_TYPE_DOCK},
    {"_NET_WM_WINDOW_TYPE_NORMAL", sizeof("_NET_WM_WINDOW_TYPE_NORMAL") - 1, &_NET_WM_WINDOW_TYPE_NORMAL},
    {"_NET_WM_PID", sizeof("_NET_WM_PID") - 1, &_NET_WM_PID},
    {"_NET_WM_STATE", sizeof("_NET_WM_STATE") - 1, &_NET_WM_STATE},
    {"_NET_WM_STATE_STICKY", sizeof("_NET_WM_STATE_STICKY") - 1, &_NET_WM_STATE_STICKY},
    {"_NET_WM_STATE_SKIP_TASKBAR", sizeof("_NET_WM_STATE_SKIP_TASKBAR") - 1, &_NET_WM_STATE_SKIP_TASKBAR},
    {"_NET_WM_STATE_ABOVE", sizeof("_NET_WM_STATE_ABOVE") - 1, &_NET_WM_STATE_ABOVE},
    {"_NET_WM_STATE_MAXIMIZED_VERT", sizeof("_NET_WM_STATE_MAXIMIZED_VERT") - 1, &_NET_WM_STATE_MAXIMIZED_VERT},
    {"_NET_WM_STRUT", sizeof("_NET_WM_STRUT") - 1, &_NET_WM_STRUT},
    {"_NET_WM_STRUT_PARTIAL", sizeof("_NET_WM_STRUT_PARTIAL") - 1, &_NET_WM_STRUT_PARTIAL},
    {"WM_PROTOCOLS", sizeof("WM_PROTOCOLS") - 1, &WM_PROTOCOLS},
    {"WM_DELETE_WINDOW", sizeof("WM_DELETE_WINDOW") - 1, &WM_DELETE_WINDOW},
    {"_XEMBED", sizeof("_XEMBED") - 1, &_XEMBED},
    {"_XEMBED_INFO", sizeof("_XEMBED_INFO") - 1, &_XEMBED_INFO},
    {"_NET_SYSTEM_TRAY_OPCODE", sizeof("_NET_SYSTEM_TRAY_OPCODE") - 1, &_NET_SYSTEM_TRAY_OPCODE},
    {"MANAGER", sizeof("MANAGER") - 1, &MANAGER},
    {"WM_STATE", sizeof("WM_STATE") - 1, &WM_STATE},
    {"_NET_SYSTEM_TRAY_ORIENTATION", sizeof("_NET_SYSTEM_TRAY_ORIENTATION") - 1, &_NET_SYSTEM_TRAY_ORIENTATION},
    {"WM_TAKE_FOCUS", sizeof("WM_TAKE_FOCUS") - 1, &WM_TAKE_FOCUS},
    {"Backlight", sizeof("Backlight") - 1, &Backlight},
    {"BACKLIGHT", sizeof("BACKLIGHT") - 1, &BACKLIGHT},
};
// clang-format on
