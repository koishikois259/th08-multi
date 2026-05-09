#pragma once

#include "GameManager.hpp"
#include "utils.hpp"

#define SCORE_DAT_VERSION           1

namespace th08
{

struct ScoreListNode
{
    ScoreListNode *prev;
    ScoreListNode *next;
    Hscr *data;
};

C_ASSERT(sizeof(ScoreListNode) == 0xc);

struct ScoreDat
{
    static i32 LinkScore(ScoreListNode *prevNode, Hscr *newScore);
    static void FreeAllScores(ScoreListNode *score);
    static ScoreDat *OpenScore(const char *filename);
    static u32 GetHighScore(ScoreDat *score, ScoreListNode *node, u32 character, u32 difficulty, u8 *continuesUsed);
    static i32 ParseCATK(ScoreDat *score, Catk *outCatk);
    static i32 ParseLSNM(ScoreDat *score, Lsnm *outLsnm);
    static i32 ParseFLSP(ScoreDat *score, Flsp *outFlsp);
    static i32 ParseCLRD(ScoreDat *score, Clrd *outClrd);
    static i32 ParsePSCR(ScoreDat *score, Pscr *outPscr);
    static i32 ParsePLST(ScoreDat *score, Plst *outPlst);
    static void ReleaseScore(ScoreDat *score);

    u16 unk0x0;
    u16 checksum;
    u16 version;
    u32 headerSize;
    ScoreListNode *scores;
    u32 decompressedFileSize;
    u32 decompressedFileSizeMinusHeader;
    u32 compressedFileSize;
};

C_ASSERT(sizeof(ScoreDat) == 0x1c);

} /* namespace th08 */
