#include "diffbuild.hpp"
#include "inttypes.hpp"
#include "ZunResult.hpp"

namespace th08
{

struct AsciiManager
{
    static ZunResult RegisterChain();
};

DIFFABLE_EXTERN(AsciiManager, g_AsciiManager);

}