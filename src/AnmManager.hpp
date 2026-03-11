#pragma once
#include <d3d8.h>
#include <d3dx8math.h>
#include "diffbuild.hpp"
#include "Global.hpp"
#include "inttypes.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "ZunColor.hpp"
#include "utils.hpp"

#define GAME_WINDOW_WIDTH 640
#define GAME_WINDOW_HEIGHT 480

namespace th08
{
struct VertexDiffuseXyzrhw
{
    D3DXVECTOR3 pos;
    f32 w;
    D3DCOLOR diffuse;
};

struct VertexTex1DiffuseXyzrhw
{
    D3DXVECTOR3 pos;
    f32 w;
    D3DCOLOR diffuse;
    D3DXVECTOR2 textureUV;
};

// Touhou 8 uses DirectX 8.1, but evidently Zun used some mismatched DirectX 8 headers as well
// D3DXIMAGE_INFO changed from 20 to 28 bytes between DX8 and DX8.1, but somehow IN uses the the DX8 version
// This struct is a redefinition of the DX8 D3DXIMAGE_INFO for that
// The only reason this ABI mismatch doesn't cause issues is because no surface indices are ever loaded other than 0
struct ZunImageInfo
{
    u32 Width;
    u32 Height;
    u32 Depth;
    u32 MipLevels;
    D3DFORMAT Format;
};
C_ASSERT(sizeof(ZunImageInfo) == 0x14);

enum AnmBlendMode
{
    AnmBlendMode_Unset = -1
};

enum AnmColorOp
{
    AnmColorOp_Unset = -1
};

enum AnmVertexShader
{
    AnmVertexShader_Unset = -1
};

enum AnmCameraMode
{
    AnmCameraMode_Unset = -1
};

enum AnmZWriteMode
{
    AnmZWriteMode_Unset = -1
};

struct AnmEntry
{
    IDirect3DTexture8 *texture;
    u8 *rawData;
    i32 size;
};

C_ASSERT(sizeof(AnmEntry) == 0xc);

struct AnmRawEntry
{
    i32 numSprites;
    i32 numScripts;
    u32 textureIdx;
    i32 width;
    i32 height;
    u32 format;
    u32 colorKey;
    u32 nameOffset;
    u32 spriteIdxOffset;
    u32 mipmapNameOffset;
    u32 version;
    u32 priority;
    u32 textureOffset;
    u8 hasData;
    /* 3 bytes pad for alignment */
    u32 nextOffset;
    u32 unk2;
};

C_ASSERT(sizeof(AnmRawEntry) == 0x40);

struct AnmTextureHeader
{
    char magic[4];      /* THTX */
    u16 unk0x4;
    i16 format;
    i16 width;
    i16 height;
    u16 unk0x14;
    u16 unk0x18;
};

struct AnmLoadedSprite
{
    u32 anmIdx;
    IDirect3DTexture8 *texture;
    ZunVec2 startPixelInclusive;
    ZunVec2 endPixelInclusive;
    float height;
    float width;
    ZunVec2 uvStart;
    ZunVec2 uvEnd;
    float heightPx;
    float widthPx;
    ZunVec2 scaleFactor;
    u32 unk0x40;
};

C_ASSERT(sizeof(AnmLoadedSprite) == 0x44);


struct AnmInterpData
{
    D3DXVECTOR3 posInitial;
    D3DXVECTOR3 posFinal;
    D3DXVECTOR3 rotateInitial;
    D3DXVECTOR3 rotateFinal;
    D3DXVECTOR2 scaleInitial;
    D3DXVECTOR2 scaleFinal;
    D3DCOLOR color1Initial;
    D3DCOLOR color1Final;
    D3DCOLOR color2Initial;
    D3DCOLOR color2Final;
};

C_ASSERT(sizeof(AnmInterpData) == 0x50);

struct AnmInterpTimers
{
    ZunTimer pos;
    ZunTimer rgb1;
    ZunTimer alpha1;
    ZunTimer rotate;
    ZunTimer scale;
    ZunTimer rgb2;
    ZunTimer alpha2;
};

C_ASSERT(sizeof(AnmInterpTimers) == 0x54);

struct AnmInterpModes
{
    u8 pos;
    u8 rgb1;
    u8 alpha1;
    u8 rotate;
    u8 scale;
    u8 rgb2;
    u8 alpha2;
    u8 unk0x7;
};

struct AnmRawInstr
{
};

struct AnmPrefix
{
    D3DXVECTOR3 rotation;
    D3DXVECTOR3 angleVel;
    D3DXVECTOR2 scale;
    D3DXVECTOR2 scaleGrowth;
    D3DXVECTOR2 spriteSize;
    D3DXVECTOR2 uvScrollPos;
    ZunTimer currentTimeInScript;
    ZunTimer waitTimer;
    AnmInterpTimers interpCurrentTImes;
    AnmInterpTimers interpEndTimes;
    AnmInterpModes interpModes;
    i32 intVars[4];
    f32 floatVars[4];
    i32 intVar8;
    i32 intVar9;
    D3DXVECTOR2 uvScrollVel;
    D3DXMATRIX matrix1;
    D3DXMATRIX matrix2;
    D3DXMATRIX matrix3;
    ZunColor color1;
    ZunColor color2;
    u32 flags;
    i16 type;
    i16 pendingInterrupt;
    i32 playerBulletHitAnimationType;
    AnmFileDesc *anmFile;
};

C_ASSERT(sizeof(AnmPrefix) == 0x208);

struct AnmVm
{
    AnmPrefix prefix;
    D3DXVECTOR3 pos;
    i16 activeSpriteIndex;
    i16 anmFileIndex;
    i16 baseSpriteIndex;
    i16 scriptIndex;
    AnmRawInstr *beginningOfScript;
    AnmRawInstr *currentInstruction;
    AnmLoadedSprite *loadedSprite;
    ZunTimer interruptReturnTime;
    AnmRawInstr *interruptReturnInstruction;
    AnmInterpData interpData;
    D3DXVECTOR3 pos2;
    i32 timeOfLastSpriteSet;
    u8 fontWidth;
    u8 fontHeight;
    unknown_fields(0x29a, 0xa);

    AnmVm::AnmVm()
    {
        memset(this, 0, sizeof(AnmVm));
        this->activeSpriteIndex = -1;
    }

    void Initialize()
    {
        this->prefix.scale.x = 1.0f;
        this->prefix.scale.y = 1.0f;
        this->prefix.color1 = COLOR_WHITE;
    }
};

C_ASSERT(sizeof(AnmVm) == 0x2a4);

struct AnmFileDesc
{
    i32 anmIdx;
    AnmRawEntry *rawData;
    i32 totalEntries;
    AnmLoadedSprite *sprites;
    AnmRawInstr **scripts;
    AnmEntry *textures;
    int numberEntriesToBeLoaded;

    void LoadSprite(i32 spriteIdx, AnmLoadedSprite *loadedSprite);

    void ExecuteAnmIdx(AnmVm *vm, int scriptIdx)
    {
        vm->scriptIndex = scriptIdx;

        vm->pos = D3DXVECTOR3(0, 0, 0);
        vm->pos2 = D3DXVECTOR3(0, 0, 0);

        vm->fontHeight = 15;
        vm->fontWidth = 15;

        this->SetAndExecuteScript(vm, this->scripts[scriptIdx]);
    }

    void SetAndExecuteScript(AnmVm *vm, AnmRawInstr *beginningOfScript)
    {
        if (beginningOfScript == NULL || (this->numberEntriesToBeLoaded != 0))
        {
            memset(vm, 0, sizeof(AnmVm));
        }
        else
        {
            vm->Initialize();
            vm->prefix.anmFile = this;
            vm->prefix.flags &= 0xfffff9ff;
            vm->beginningOfScript = beginningOfScript;
            vm->currentInstruction = vm->beginningOfScript;
            vm->prefix.currentTimeInScript.SetCurrent(0);
            vm->prefix.flags &= 0xfffffffe;
            /* TODO: circular dependency! */
            #if 0
            g_AnmManager->ExecuteScript(vm);
            #endif
        }
    }
};

C_ASSERT(sizeof(AnmFileDesc) == 0x1c);

struct AnmRawSprite
{
    u32 id;
    float x;
    float y;
    float width;
    float height;
};

struct AnmRawScript
{
    u32 id;
    u32 offset;
};

struct AnmManager
{
    AnmManager();
    void SetupVertexBuffer();
    ~AnmManager() {}
    u32 ExecuteScript(AnmVm *sprite);
    void ExecuteScriptOnVmArray(AnmVm *sprites, int count);
    ZunResult CreateTextureFromFile(IDirect3DTexture8 **outTexture, i32 format, i32 colorKey);
    ZunResult CreateTextureFromAnm(IDirect3DTexture8 **outTexture, AnmTextureHeader *textureData, i32 format);
    ZunResult CreateEmptyTexture(IDirect3DTexture8 **outTexture, i32 width, i32 height, i32 format);
    AnmFileDesc *LoadAnm(i32 anmIdx, const char *filename);
    AnmFileDesc *ReadAnmEntries(i32 anmIdx, const char *filename);
    AnmFileDesc *PreloadAnm(i32 anmIdx, const char *filename);
    i32 LoadExternalTextureData(AnmFileDesc *fileDesc, i32 entryNumber, i32 *sprites, i32 *scripts, AnmRawEntry *rawEntry);
    AnmFileDesc *PostloadAnmEntry(AnmFileDesc *anm);
    BOOL LoadTextureData(AnmFileDesc *fileDesc, i32 entryNumber, i32 sprites, i32 scripts, AnmRawEntry *rawEntry);
    ZunResult ServicePreloadedAnims();
    void ReleaseAnm(i32 anmIdx);
    void ReleaseAnmEntry(AnmEntry *anmEntry);
    ZunResult LoadSurface(i32 surfaceIdx, const char *path);
    void ReleaseSurface(i32 surfaceIdx);
    void CopySurfaceToBackbuffer(int surfaceIdx, int left, int top, int x, int y);

    void ClearBlendMode()
    {
        this->currentBlendMode = 3;
    }

    void ClearColorOp()
    {
        this->currentColorOp = AnmColorOp_Unset;
    }

    void ClearSprite()
    {
        this->currentSprite = NULL;
    }

    void ClearVertexShader()
    {
        this->currentVertexShader = AnmVertexShader_Unset;
    }

    void ClearTexture()
    {
        this->currentTexture = NULL;
    }

    void ClearCameraSettings()
    {
        this->cameraMode = AnmCameraMode_Unset;
    }

    void ClearZWrite()
    {
        this->disableZWrite = AnmZWriteMode_Unset;
    }

    void ResetSomeStuff()
    {
        this->flushesThisFrame = 0;
        unk0x14 = 0;
        unk0xc = 0;
        unk0x18 = 0;
    }

    void ReleaseSurfaces()
    {
        i32 i;

        for (i = 0; i < ARRAY_SIZE_SIGNED(this->surfaces); i++)
        {
            if (this->surfaces[i] != NULL)
            {
                this->surfaces[i]->Release();
                this->surfaces[i] = NULL;
            }
        }
    }

    void TakeScreencaptures()
    {
        if (this->captureAnmIdx >= 0)
        {
            CaptureToTexture(this->captureAnmIdx, this->textureCaptureSrcX, this->textureCaptureSrcY, this->textureCaptureSrcW, this->textureCaptureSrcH,
                            this->textureCaptureDstX, this->textureCaptureDstY, this->textureCaptureDstW, this->textureCaptureDstH);
            this->captureAnmIdx = -1;
        }

        if (this->captureSurfaceIdx >= 0)
        {
            CaptureToSurface(this->captureSurfaceIdx, this->surfaceCaptureSrcX, this->surfaceCaptureSrcY, this->surfaceCaptureSrcW, this->surfaceCaptureSrcH,
                            this->surfaceCaptureDstX, this->surfaceCaptureDstY, this->surfaceCaptureDstW, this->surfaceCaptureDstH);
            this->captureSurfaceIdx = -1;
        }
    }

    void ResetMoreStuff()
    {
        unk0x4 = 0;
        this->color = 0x80808080;
    }

    void CaptureToTexture (i32 captureAnmIdx, i32 srcX, i32 srcY, i32 srcW, i32 srcH, i32 dstX, i32 dstY, i32 dstW, i32 dstH);
    void CaptureToSurface (i32 captureSurfaceIdx, i32 srcX, i32 srcY, i32 srcW, i32 srcH, i32 dstX, i32 dstY, i32 dstW, i32 dstH);

    void ClearVertexBuffer();
    void FlushVertexBuffer();

    D3DCOLOR color;
    i32 unk0x4;
    i32 captureSurfaceIdx;
    u32 unk0xc;
    u32 flushesThisFrame;
    u32 unk0x14;
    u32 unk0x18;
    D3DXVECTOR2 unk0x1c;
    AnmFileDesc anmFiles[256];
    D3DXVECTOR3 unk0x1c24;
    unknown_fields(0x1c30, 0x34);
    AnmVm unk0x1c64;
    unknown_fields(0x1f08, 0x130);

    IDirect3DSurface8 *surfaces[32];
    IDirect3DSurface8 *surfacesBis[32];
    u8 *surfaceData[32];
    u32 surfaceDataSizes[32];
    ZunImageInfo surfaceInfo[32];

    unknown_fields(0x24b8, 0x4);

    IDirect3DBaseTexture8 *currentTexture;
    u8 currentBlendMode;
    u8 currentColorOp;
    u8 currentVertexShader;
    u8 disableZWrite;
    u8 cameraMode;
    unknown_fields(0x24c5, 3); // Padding?
    void *currentSprite;
    unknown_fields(0x24cc, 4);
    VertexDiffuseXyzrhw untexturedVector[4];
    u32 spritesToDraw;
    VertexTex1DiffuseXyzrhw vertexBuffer[0x18000];
    VertexTex1DiffuseXyzrhw *vertexBufferEndPtr;
    VertexTex1DiffuseXyzrhw *vertexBufferStartPtr;
    i32 captureAnmIdx;
    i32 textureCaptureSrcX;
    i32 textureCaptureSrcY;
    i32 textureCaptureSrcW;
    i32 textureCaptureSrcH;
    i32 textureCaptureDstX;
    i32 textureCaptureDstY;
    i32 textureCaptureDstW;
    i32 textureCaptureDstH;
    i32 surfaceCaptureSrcX;
    i32 surfaceCaptureSrcY;
    i32 surfaceCaptureSrcW;
    i32 surfaceCaptureSrcH;
    i32 surfaceCaptureDstX;
    i32 surfaceCaptureDstY;
    i32 surfaceCaptureDstW;
    i32 surfaceCaptureDstH;
};
C_ASSERT(sizeof(AnmManager) == 0x2a2570);

DIFFABLE_EXTERN(AnmManager *, g_AnmManager);

}; // namespace th08
