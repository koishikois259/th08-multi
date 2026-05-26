#pragma once

#include "inttypes.hpp"

#define COLOR_BLACK 0xff000000
#define COLOR_LIGHT_GREY 0xffa0a0a0
#define COLOR_WHITE 0xffffffff

#define COLOR_TEXT_WHITE    0xffffff

union ZunColor {
    u32 d3dColor;
    struct
    {
        u8 b;
        u8 g;
        u8 r;
        u8 a;
    };
};
