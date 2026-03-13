#include "ZunResult.hpp"
#include "diffbuild.hpp"
#include "inttypes.hpp"

namespace th08
{

struct AsciiManager
{
    static ZunResult RegisterChain();
};

DIFFABLE_EXTERN(AsciiManager, g_AsciiManager);

} // namespace th08