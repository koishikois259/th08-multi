#include "SoundPlayer.hpp"

namespace th08
{
DIFFABLE_STATIC_ARRAY_ASSIGN(SoundBufferIdxVolume, 46, g_SoundBufferIdxVol) = {
    {0, -1900, 0},   {0, -2100, 0},   {1, -1200, 5},    {1, -1500, 5},   {2, -1100, 100}, {3, -700, 100},
    {4, -700, 100},  {5, -1900, 50},  {6, -2200, 50},   {7, -2400, 50},  {8, -1100, 100}, {9, -1100, 100},
    {10, -1500, 10}, {11, -1500, 10}, {12, -1000, 100}, {5, -1100, 50},  {13, -1300, 50}, {14, -1400, 50},
    {15, -900, 100}, {16, -400, 100}, {17, -880, 0},    {18, -1500, 0},  {5, -300, 20},   {6, -1800, 20},
    {7, -1800, 20},  {19, -1100, 50}, {20, -1300, 50},  {21, -1500, 50}, {22, -500, 140}, {23, -500, 100},
    {24, -1100, 20}, {25, -800, 90},  {24, -1200, 20},  {19, -500, 50},  {26, -800, 100}, {27, -800, 100},
    {28, -800, 100}, {29, -700, 0},   {30, -300, 100},  {31, -800, 100}, {32, -800, 100}, {33, -200, 100},
    {34, 0, 100},    {34, -600, 100}, {35, -800, 0},    {8, -100, 100},
};
DIFFABLE_STATIC_ARRAY_ASSIGN(char *, 36, g_SFXList) = {
    "se_plst00.wav", "se_enep00.wav",   "se_pldead00.wav", "se_power0.wav",
    "se_power1.wav", "se_tan00.wav",    "se_tan01.wav",    "se_tan02.wav",
    "se_ok00.wav",   "se_cancel00.wav", "se_select00.wav", "se_gun00.wav",
    "se_cat00.wav",  "se_lazer00.wav",  "se_lazer01.wav",  "se_enep01.wav",
    "se_nep00.wav",  "se_damage00.wav", "se_item00.wav",   "se_kira00.wav",
    "se_kira01.wav", "se_kira02.wav",   "se_extend.wav",   "se_timeout.wav",
    "se_graze.wav",  "se_powerup.wav",  "se_pause.wav",    "se_cardget.wav",
    "se_option.wav", "se_damage01.wav", "se_timeout2.wav", "se_opshow.wav",
    "se_ophide.wav", "se_invalid.wav",  "se_slash.wav",    "se_item01.wav",
};
DIFFABLE_STATIC(SoundPlayer, g_SoundPlayer)

};
