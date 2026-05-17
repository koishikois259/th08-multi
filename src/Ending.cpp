#include "Ending.hpp"
#include "ZunColor.hpp"
#include "AnmManager.hpp"
#include "i18n.hpp"
#include <cstdlib>

namespace th08
{

Ending::Ending()
{
  memset(this, 0, sizeof(Ending));
  this->unk2a90 = 8;
  this->timer2 = 0;
  this->timer1 = 0;
  this->unk08 = 0;
  this->unk0c = 0;
  this->unk10 = 0;
}

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

#pragma var_order(local_8, textBuffer, index, i, scriptIdx, vmIndex, spriteIdx, firstRead, secondRead, cmdParam, fadeTime)
ZunResult Ending::ParseEndFile()
{
    char textBuffer[68];
    i32 index;
    i32 local_8;
    i32 i;

    i32 vmIndex;
    i32 scriptIdx;
    i32 spriteIdx;
    i32 firstRead;
    i32 secondRead;
    i32 cmdParam;
    f32 fadeTime;

    local_8 = false;
    index = 0;

    memset(textBuffer, 0, sizeof(textBuffer));

    if(this->timer3 > 0){
        this->timer3--;

        if (this->unk2a88 != 0){
            this->unk2a88--;
        }
        else {
            if((WAS_PRESSED(0x1001)) || ((this->canSkip != 0 && (IS_PRESSED(0x100) != 0)))){
              this->timer3 = 0;
            }
        }

        if (this->timer3 <= 0){
            for(i = 0; i < 15; i++){
                this->vms[i].SetInterrupt(2);
            }
            this->unk2a9c = 0;
        }
        else {
          goto end_of_parse;
        }
    }

    if (this->timer2 > 0) {
    this->timer2--;
    if (this->unk2a8c != 0) {
      this->unk2a8c--;

    } else {
      if ((WAS_PRESSED(0x1001)) || ((this->canSkip != 0 && (IS_PRESSED(0x100) != 0)))){
        this->timer2 = 0;
      }
    }
    goto end_of_parse; 
  }

        while(true){
            switch(*this->cursorPtr){
            case '@':
                this->cursorPtr++;
                switch(*this->cursorPtr){
                case 'b':
                    if(g_AnmManager->LoadSurface(0, (this->cursorPtr + 1))){
                        return ZUN_ERROR;
                    }
                    break;
                case 'a':
                    this->cursorPtr++;
                    vmIndex = this->ReadEndFileParameter();
                    scriptIdx = this->ReadEndFileParameter();
                    spriteIdx = this->ReadEndFileParameter();
                    this->anmFile->ExecuteAnmIdx(&this->vms[vmIndex], scriptIdx);
                    this->anmFile->SetSprite(&this->vms[vmIndex], spriteIdx);
                    break;
                case 'V':
                    this->cursorPtr++;
                    firstRead = this->ReadEndFileParameter();
                    secondRead = this->ReadEndFileParameter();
                    this->unk10 = (f32)firstRead / (f32)secondRead;
                    break;
                case 'v':
                    this->cursorPtr++;
                    this->unk0c = (f32)this->ReadEndFileParameter();
                    break;
                case 'F':
                    if(this->LoadEnding(this->cursorPtr + 1) != ZUN_SUCCESS){
                        return ZUN_ERROR;
                    }
                    index = 0;
                    local_8 = 0;
                    this->canSkip = this->unk2a5c;
                case 'R':
                    for(cmdParam = 0; cmdParam < 16; cmdParam++){
                        this->vms[cmdParam].scriptIndex = 0;
                    }
                    break;
                case 'm':
                    g_Supervisor.LoadMusic(0, this->cursorPtr + 1);
                    g_Supervisor.PlayMusic(0, 0);
                    break;
                case 'M':
                    this->cursorPtr++;
                    fadeTime = (f32)this->ReadEndFileParameter();
                    g_Supervisor.FadeOutMusic(fadeTime);
                    break;
                case 's':
                    this->cursorPtr++;
                    this->unk2a90 = this->ReadEndFileParameter();
                    this->unk2a94 = this->ReadEndFileParameter();
                    break;
                case 'c':
                    this->cursorPtr++;
                    this->unk2aa0 = this->ReadEndFileParameter();
                    break;
                case 'r':
                    this->cursorPtr++;
                    this->timer3 = this->ReadEndFileParameter();
                    this->unk2a88 = this->ReadEndFileParameter();
                    this->timer2 = 0;
                    this->unk2a8c = 0;
                    while(*this->cursorPtr != '\n' && *this->cursorPtr != '\r'){
                        this->cursorPtr++;
                    }
                    while(*this->cursorPtr == '\n' || *this->cursorPtr == '\r'){
                        this->cursorPtr++;
                    }
                    goto end_of_parse;
                case 'w':
                    this->cursorPtr++;
                    this->timer2 = this->ReadEndFileParameter();
                    this->unk2a8c = this->ReadEndFileParameter();
                    while(*this->cursorPtr != '\n' && *this->cursorPtr != '\r'){
                        this->cursorPtr++;
                    }
                    while(*this->cursorPtr == '\n' || *this->cursorPtr == '\r'){
                        this->cursorPtr++;
                    }
                    goto end_of_parse;

                case '0':
                    this->cursorPtr++;
                    this->fadeMode = 1;
                    this->fadeTimer = 0;
                    this->fadeDuration = this->ReadEndFileParameter();
                    break;
                case '1':
                    this->cursorPtr++;
                    this->fadeMode = 2;
                    this->fadeTimer = 0;
                    this->fadeDuration = this->ReadEndFileParameter();
                    break;
                case '2':
                    this->cursorPtr++;
                    this->fadeMode = 3;
                    this->fadeTimer = 0;
                    this->fadeDuration = this->ReadEndFileParameter();
                    break;
                case '3':
                    this->cursorPtr++;
                    this->fadeMode = 4;
                    this->fadeTimer = 0;
                    this->fadeDuration = this->ReadEndFileParameter();
                    break;                

                
                
                case 'z':
                    return ZUN_ERROR;
                }
                while(*this->cursorPtr != '\n' && *this->cursorPtr != '\r'){
                    this->cursorPtr++;
                }
                while(*this->cursorPtr == '\n' || *this->cursorPtr == '\r'){
                    this->cursorPtr++;
                }
                break;

            case '\0':
            case '\n':
            case '\r':
                if(index != 0){
                    g_AnmManager->DrawTextLeft(
                        &this->vms[this->unk2a9c],
                        this->unk2aa0,
                        COLOR_WHITE,
                        textBuffer
                    );
                    this->vms[this->unk2a9c].SetInterrupt(1);
                }
                    while(*this->cursorPtr == '\n' || *this->cursorPtr == '\0' || *this->cursorPtr == '\r'){
                        this->cursorPtr++;
                    }
                    if(IS_PRESSED(0x1001)){
                        this->timer2 = this->unk2a94;
                        this->unk2a8c = this->unk2a94;
                    }
                    else {
                        this->timer2 = this->unk2a90;
                        this->unk2a8c = this->unk2a94;
                    }
                    this->unk2a9c++;
                    goto end_of_parse;

            default:
                textBuffer[index] = *this->cursorPtr;
                textBuffer[index + 1] = *(this->cursorPtr + 1);
                index += 2;
                this->cursorPtr += 2;
                break;

            } // switch(*this->cursorPtr)
        } // while(true)
    
end_of_parse:
    this->timer1++;
    this->unk0c -= this->unk10;

    if(this->unk0c <= 0.0f){
        this->unk0c = 0.0f;
        this->unk10 = 0.0f;
    }
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
  this->unk2a90 = 8;
  this->timer2 = 0;
  this->timer1 = 0;

  if (prevFile != NULL){
    g_ZunMemory.Free(prevFile);
  }
  return ZUN_SUCCESS;
}

ZunResult Ending::RegisterChain()
{
    Ending *ending = ZUN_NEW(Ending, "EndingInf");

    ending->calcChain = g_Chain.CreateElem((ChainCallback)Ending::OnUpdate);
    ending->calcChain->arg = ending;
    ending->calcChain->addedCallback = (ChainLifetimeCallback)Ending::AddedCallback;
    ending->calcChain->deletedCallback = (ChainLifetimeCallback)Ending::DeletedCallback;

    if (g_Chain.AddToCalcChain(ending->calcChain, 5) != ZUN_SUCCESS) {
        return ZUN_ERROR;
      }

    ending->drawChain = g_Chain.CreateElem((ChainCallback)Ending::OnDraw);
    ending->drawChain->arg = ending;
    g_Chain.AddToDrawChain(ending->drawChain, 4);
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
            if (IS_PRESSED(0x100) != 0) {
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

ZunResult Ending::DeletedCallback(Ending *ending)
{
    g_AnmManager->ReleaseAnm(24);
    g_Supervisor.curState = SupervisorState_ResultScreenFromGame;
    g_AnmManager->ReleaseSurface(0);
    g_ZunMemory.Free(ending->fileData);
    g_Chain.Cut(ending->drawChain);
    ending->drawChain = NULL;
    ZUN_DELETE(ending);
    g_Supervisor.unk178 = 0;
    return ZUN_SUCCESS;
}

} /* namespace th08 */
