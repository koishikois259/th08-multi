#include "Ending.hpp"
#include "ZunColor.hpp"
#include "AnmManager.hpp"
#include "i18n.hpp"
#include <cstdlib>

namespace th08
{

ZunResult Ending::ReadEndFileParameter()
{
    DWORD param;

    param = atol(this->cursorPtr);

    while (*this->cursorPtr != '\0') {
        this->cursorPtr++;
    }
    while (*this->cursorPtr == '\0') {
        this->cursorPtr++;
    }

    return (ZunResult)param;
}

#pragma var_order(rect, alpha)
void Ending::FadingEffect()
{
    ZunRect rect;
    i32 alpha;

    rect.left = 0.0f;
    rect.top = 0.0f;
    rect.right = (f32)GAME_WINDOW_WIDTH;
    rect.bottom = (f32)GAME_WINDOW_HEIGHT;

    switch (this->fadeMode) {
    case 1:
        if (this->fadeTimer >= this->fadeDuration) {
            this->fadeMode = 0;
            this->fadeColor = 0;
            break;
        } else {
            alpha = 0xff - (this->fadeTimer * 0xff) / this->fadeDuration;
            this->fadeColor = COLOR_SET_ALPHA(0, alpha);
            this->fadeTimer++;
            break;
        }

    case 2:
        if (this->fadeTimer >= this->fadeDuration) {
            this->fadeColor = COLOR_BLACK;
            break;
        } else {
            alpha = (this->fadeTimer * 0xff) / this->fadeDuration;
            this->fadeColor = COLOR_SET_ALPHA(0, alpha);
            this->fadeTimer++;
            break;
        }

    case 3:
        if (this->fadeTimer >= this->fadeDuration) {
            this->fadeMode = 0;
            this->fadeColor = 0;
            break;
        } else {
            alpha = 0xff - (this->fadeTimer * 0xff) / this->fadeDuration;
            this->fadeColor = COLOR_SET_ALPHA3(COLOR_WHITE, alpha);
            this->fadeTimer++;
            break;
        }

    case 4:
        if (this->fadeTimer >= this->fadeDuration) {
            this->fadeColor = COLOR_WHITE;
            break;
        } else {
            alpha = (this->fadeTimer * 0xff) / this->fadeDuration;
            this->fadeColor = COLOR_SET_ALPHA3(COLOR_WHITE, alpha);
            this->fadeTimer++;
            break;
        }

    case 0:
        this->fadeColor = 0;
        break;
    }

    if ((this->fadeColor & COLOR_ALPHA_MASK) != 0) {
        ScreenEffect::DrawSquare(&rect, this->fadeColor);
    }
}

// STUB: th08 0x428b80
ZunResult Ending::ParseEndFile()
{
    return ZUN_SUCCESS;
}

ZunResult Ending::LoadEnding(const char *path)
{
  void *prevFile = this->fileData;
  this->fileData = FileSystem::OpenFile(path, NULL, 0);

  if (this->fileData == NULL) {
    this->fileData = prevFile;
    g_GameErrorContext.Log(TH_ERR_ENDING_FILE_UNREADABLE);
    return ZUN_ERROR;
  }

  this->cursorPtr = (char *)this->fileData;
  this->unknown_2a90 = 8;
  this->timer2 = 0;
  this->timer1 = 0;

  if (prevFile != NULL){
    g_ZunMemory.Free(prevFile);
  }
  return ZUN_SUCCESS;
}

// STUB: th08 0x4296a0
ZunResult Ending::RegisterChain()
{
    return ZUN_SUCCESS;
}

ChainCallbackResult Ending::OnUpdate(Ending *ending)
{
    i32 frameSkip = 0;

    for(;;) {
        if (ending->ParseEndFile() != ZUN_SUCCESS) {
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }

         for (i32 i = 0; i < 15; i++) {
            g_AnmManager->ExecuteScript(&ending->vms[i]);
        }

          if (ending->canSkip != 0) {
            if ((g_CurFrameInput & 0x100) != 0) {
                if (frameSkip < 8) {
                    frameSkip++;
                    continue;
                }
            }
        }
        break;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// STUB: th08 0x4298f0
ChainCallbackResult Ending::OnDraw(Ending *ending)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// STUB: th08 0x429980
ZunResult Ending::AddedCallback(Ending *ending)
{
    return ZUN_SUCCESS;
}

// STUB: th08 0x429d40
ZunResult Ending::DeletedCallback(Ending *ending)
{
    return ZUN_SUCCESS;
}

} /* namespace th08 */