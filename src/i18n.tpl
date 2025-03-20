#pragma once

#define TH_LANG TH_JP

// JP
#define TH_JP_ERR_MIDI_FAILED_TO_READ_FILE "error : MIDI File が読み込めない %s \rv\r\n"
#define TH_JP_ERR_MIDI_NOT_LOADED "error : まだMIDIが読み込まれていないのに再生しようとしている\n"
#define TH_JP_ERR_ARCFILE_CORRUPTED "ファイル %s のオープン中にエラーが発生しました"

#define TH_JP_ERR_SOUNDPLAYER_FAILED_TO_INITIALIZE_OBJECT "DirectSound オブジェクトの初期化が失敗したよ\r\n"
#define TH_JP_DBG_SOUNDPLAYER_INIT_SUCCESS "DirectSound は正常に初期化されました\r\n"

// EN
#define TH_EN_ERR_MIDI_FAILED_TO_READ_FILE "error : MIDI File %s could not be read.\n"
#define TH_EN_ERR_MIDI_NOT_LOADED "error : MIDI not loaded before being playback started.\n"
#define TH_EN_ERR_ARCFILE_CORRUPTED "Error opening file %s"

#define TH_EN_ERR_SOUNDPLAYER_FAILED_TO_INITIALIZE_OBJECT "DirectSound: Failed to initialize object\r\n"
#define TH_EN_DBG_SOUNDPLAYER_INIT_SUCCESS "DirectSound initialized successfully\r\n"

#define TH_CONCAT_HELPER(x, y) x##y

#define TH_MAKE_LANG_STR(lang, id) TH_CONCAT_HELPER(lang, id)

#define TH_ERR_MIDI_FAILED_TO_READ_FILE TH_MAKE_LANG_STR(TH_LANG, _ERR_MIDI_FAILED_TO_READ_FILE)
#define TH_ERR_MIDI_NOT_LOADED TH_MAKE_LANG_STR(TH_LANG, _ERR_MIDI_NOT_LOADED)
#define TH_ERR_ARCFILE_CORRUPTED TH_MAKE_LANG_STR(TH_LANG, _ERR_ARCFILE_CORRUPTED)

#define TH_ERR_SOUNDPLAYER_FAILED_TO_INITIALIZE_OBJECT TH_MAKE_LANG_STR(TH_LANG, _ERR_SOUNDPLAYER_FAILED_TO_INITIALIZE_OBJECT)
#define TH_DBG_SOUNDPLAYER_INIT_SUCCESS TH_MAKE_LANG_STR(TH_LANG, _DBG_SOUNDPLAYER_INIT_SUCCESS)

#define TH_FONT_NAME "ＭＳ ゴシック"
