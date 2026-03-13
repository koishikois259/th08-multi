#pragma once

#include "Global.hpp"
#include "ZunResult.hpp"
#include "diffbuild.hpp"
#include "inttypes.hpp"
#include "utils.hpp"

namespace th08
{

struct Ending
{
    static ZunResult RegisterChain();

    unknown_fields(0x0, 0x2ab8);
};

C_ASSERT(sizeof(Ending) == 0x2ab8);

} // namespace th08
