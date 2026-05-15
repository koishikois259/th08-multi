#include "Ending.hpp"
#include "ZunColor.hpp"
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

// STUB: th08 0x4295e0
ZunResult Ending::LoadEnding(const char *path)
{
    return ZUN_SUCCESS;
}

// STUB: th08 0x4296a0
ZunResult Ending::RegisterChain()
{
    return ZUN_SUCCESS;
}

// STUB: th08 0x429860
ChainCallbackResult Ending::OnUpdate(Ending *ending)
{
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