#pragma once

#include <windows.h>
#include "inttypes.hpp"
#include "utils.hpp"
#include "ZunResult.hpp"

namespace th08
{
    struct ResultSysInf
    {
        static ZunResult RegisterChain(u32 unk); 

        unknown_fields(0x0, 0x477b0);
    };
    C_ASSERT(sizeof(ResultSysInf) == 0x477b0);
};
