#include "ScoreDat.hpp"

namespace th08
{

// STUB: th08 0x45a500
i32 ScoreDat::LinkScore(ScoreListNode *prevNode, Hscr *newScore)
{
    return 0;
}

// STUB: th08 0x45a5a0
void ScoreDat::FreeAllScores(ScoreDat *score)
{
}

// STUB: th08 0x45a5e0
ScoreDat *ScoreDat::OpenScore(const char *filename)
{
    return NULL;
}

// STUB: th08 0x45a950
u32 ScoreDat::GetHighScore(ScoreDat *score, ScoreListNode *node, u32 character, u32 difficulty, u8 *continuesUsed)
{
    return 0;
}

// STUB: th08 0x45aac0
i32 ScoreDat::ParseCATK(ScoreDat *score, Catk *outCatk)
{
    return 0;
}

// STUB: th08 0x45ab70
i32 ScoreDat::ParseLSNM(ScoreDat *score, Lsnm *outLsnm)
{
    return 0;
}

// STUB: th08 0x45ac00
i32 ScoreDat::ParseFLSP(ScoreDat *score, Flsp *outFlsp)
{
    return 0;
}

// STUB: th08 0x45ac90
i32 ScoreDat::ParseCLRD(ScoreDat *score, Clrd *outClrd)
{
    return 0;
}

// STUB: th08 0x45ae00
i32 ScoreDat::ParsePSCR(ScoreDat *score, Pscr *outPscr)
{
    return 0;
}

// STUB: th08 0x45af30
i32 ScoreDat::ParsePLST(ScoreDat *score, Plst *outPlst)
{
    return 0;
}

// STUB: th08 0x45afc0
void ScoreDat::ReleaseScore(ScoreDat *score)
{
}

} /* namespace th08 */