#pragma once

#include "inttypes.hpp"
#include "ZunBool.hpp"
#include "diffbuild.hpp"


/* TODO: this should be an enum with all the spell cards... */

#define SPELLCARD_LAST_WORD_START                  205

#define SPELLCARD_COUNT_IN_GAME_SPELLCARDS         205
#define SPELLCARD_COUNT_SPELLCARDS                 222
#define SPELLCARD_COUNT_LAST_WORD_SPELLCARDS      (SPELLCARD_COUNT_SPELLCARDS - SPELLCARD_COUNT_IN_GAME_SPELLCARDS)

namespace th08
{

struct Spellcard
{
    static i32 GetDifficultyFromSpellCard(i32 spellcardNumber);
};

DIFFABLE_EXTERN_ARRAY(i32 *, 6, g_SpellcardNumbersPerDifficulty);
DIFFABLE_EXTERN_ARRAY(i32, 6, g_SpellcardCountsPerDifficulty);
DIFFABLE_EXTERN_ARRAY(i32, 43, g_LastSpellNumbers);
DIFFABLE_EXTERN(i32, g_LastSpellCount);
DIFFABLE_EXTERN_ARRAY(i32 *, 10, g_SpellcardNumbersPerStage)
DIFFABLE_EXTERN_ARRAY(i32, 10, g_SpellcardCountPerStage)

} /* namespace th08 */