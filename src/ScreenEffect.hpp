#pragma once

#include "inttypes.hpp"
#include "utils.hpp"
#include <d3dx8.h>

namespace th08
{
struct ScreenEffect
{
    static void SetViewport(D3DCOLOR clearColor);

    unknown_fields(0x0, 0x34);
};
}; // Namespace th08
