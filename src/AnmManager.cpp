#include "AnmManager.hpp"
#include "i18n.hpp"
#include "utils.hpp"

namespace th08
{
DIFFABLE_STATIC(AnmManager *, g_AnmManager);

D3DFORMAT g_TextureFormatD3D8Mapping[] = {D3DFMT_UNKNOWN, D3DFMT_A8R8G8B8, D3DFMT_A1R5G5B5,
                                          D3DFMT_R5G6B5,  D3DFMT_R8G8B8,   D3DFMT_A4R4G4B4};

u32 g_TextureFormatBytesPerPixel[] = {4, 4, 2, 2, 3, 2};

void AnmManager::ExecuteScriptOnVmArray(AnmVm *sprite, int count)
{
    while (count != 0)
    {
        if (sprite->scriptIndex >= 0)
        {
            g_AnmManager->ExecuteScript(sprite);
        }
        sprite++;
        count--;
    }
}

// STUB: th08 0x465070
AnmManager::AnmManager()
{
    memset((void *)this, 0, sizeof(AnmManager));
}

// STUB: th08 0x465250
void AnmManager::SetupVertexBuffer()
{
}

static i32 GetAnmFormat(i32 format)
{
    if (g_Supervisor.Is16bitColorMode() != 0)
    {
        if ((g_TextureFormatD3D8Mapping[format] == D3DFMT_A8R8G8B8) ||
            (g_TextureFormatD3D8Mapping[format] == D3DFMT_UNKNOWN))
        {
            format = 5;
        }
        else if (g_TextureFormatD3D8Mapping[format] == D3DFMT_R8G8B8)
        {
            format = 3;
        }
    }

    return format;
}

// STUB: th08 0x465570
ZunResult AnmManager::CreateTextureFromFile(IDirect3DTexture8 **outTexture, i32 format, i32 colorKey)
{
    return ZUN_ERROR;
}

#pragma var_order(surface, textureSurfaceLevel, header, lockedRect, currentY, textureSrc, textureDest)
ZunResult AnmManager::CreateTextureFromAnm(IDirect3DTexture8 **outTexture, AnmTextureHeader *textureHeader, i32 format)
{
    IDirect3DSurface8 *surface;
    IDirect3DSurface8 *textureSurfaceLevel;
    AnmTextureHeader *header;
    const void *textureSrc;
    void *textureDest;
    D3DLOCKED_RECT lockedRect;
    int currentY;

    surface = NULL;
    textureSurfaceLevel = NULL;
    format = GetAnmFormat(format);
    header = textureHeader;

    g_Supervisor.d3dDevice->CreateImageSurface(header->width, header->height,
                                               g_TextureFormatD3D8Mapping[header->format], &surface);

    surface->LockRect(&lockedRect, NULL, 0);

    for (currentY = 0; currentY < header->height; currentY++)
    {
        textureDest = (u8 *)lockedRect.pBits + currentY * lockedRect.Pitch;
        textureSrc = ((u8 *)textureHeader) + sizeof(AnmTextureHeader) +
                     (currentY * header->width * g_TextureFormatBytesPerPixel[header->format]);
        memcpy(textureDest, textureSrc, header->width * g_TextureFormatBytesPerPixel[header->format]);
    }

    surface->UnlockRect();

    if (D3DXCreateTexture(g_Supervisor.d3dDevice, header->width, header->height, 1, 0,
                          g_TextureFormatD3D8Mapping[format], D3DPOOL_MANAGED, outTexture) != D3D_OK)
    {
        goto err;
    }

    (*outTexture)->GetSurfaceLevel(0, &textureSurfaceLevel);

    if (D3DXLoadSurfaceFromSurface(textureSurfaceLevel, NULL, NULL, surface, NULL, NULL, 3, 0) != D3D_OK)
    {
        goto err;
    }

    if (surface != NULL)
    {
        surface->Release();
        surface = NULL;
    }
    if (textureSurfaceLevel != NULL)
    {
        textureSurfaceLevel->Release();
        textureSurfaceLevel = NULL;
    }

    return ZUN_SUCCESS;

err:
    if (surface != NULL)
    {
        surface->Release();
        surface = NULL;
    }
    if (textureSurfaceLevel != NULL)
    {
        textureSurfaceLevel->Release();
        textureSurfaceLevel = NULL;
    }

    return ZUN_ERROR;
}

ZunResult AnmManager::CreateEmptyTexture(IDirect3DTexture8 **outTexture, i32 width, i32 height, i32 format)
{
    D3DXCreateTexture(g_Supervisor.d3dDevice, width, height, 1, 0, g_TextureFormatD3D8Mapping[format], D3DPOOL_MANAGED,
                      outTexture);

    return ZUN_SUCCESS;
}

AnmFileDesc *AnmManager::LoadAnm(i32 anmIdx, const char *filename)
{
    utils::DebugPrint("::loadAnim : %s\n", filename);
    AnmFileDesc *fileDesc = this->ReadAnmEntries(anmIdx, filename);
    if (fileDesc != NULL)
    {
        fileDesc->numberEntriesToBeLoaded = 1;

        /* ZUN bug: no NULL check! */
        while (fileDesc->numberEntriesToBeLoaded != 0)
        {
            fileDesc = this->PostloadAnmEntry(fileDesc);
        }
    }

    return fileDesc;
}

#pragma var_order(curEntryNum, totalSprites, totalEntries, fileDesc, entry, result, totalScripts, curEntry)
AnmFileDesc *AnmManager::ReadAnmEntries(int anmIdx, const char *filename)
{
    i32 result;

    utils::DebugPrint("::preloadAnim : %s\n", filename);

    if (anmIdx >= 25)
    {
        g_GameErrorContext.Fatal(TH_ERR_ANMMANAGER_NO_TEXTURE_STORAGE);
        return NULL;
    }

    this->ReleaseAnm(anmIdx);

    AnmRawEntry *entry = (AnmRawEntry *)FileSystem::OpenFile(filename, NULL, 0);
    i32 totalEntries = 0;
    i32 totalScripts = 0;
    i32 totalSprites = 0;
    i32 curEntryNum = 0;

    AnmFileDesc *fileDesc = this->anmFiles + anmIdx;
    if (entry == NULL)
    {
        return NULL;
    }

    fileDesc->anmIdx = anmIdx;
    fileDesc->rawData = entry;
    AnmRawEntry *curEntry = entry;

    while (true)
    {
        totalEntries++;
        totalScripts += curEntry->numScripts;
        totalSprites += curEntry->numSprites;

        if (curEntry->nextOffset == 0)
        {
            break;
        }

        curEntry = (AnmRawEntry *)(((u8 *)curEntry) + curEntry->nextOffset);
    }

    fileDesc->totalEntries = totalEntries;

    fileDesc->textures = (AnmEntry *)g_ZunMemory.Alloc(totalEntries * sizeof(AnmEntry));
    memset(fileDesc->textures, 0, sizeof(AnmEntry) * totalEntries);
    fileDesc->sprites = (AnmLoadedSprite *)g_ZunMemory.Alloc(totalSprites * sizeof(AnmLoadedSprite));
    fileDesc->scripts = (AnmRawInstr **)g_ZunMemory.Alloc(totalScripts * sizeof(void *));

    curEntry = entry;
    totalEntries = 0;
    totalSprites = 0;
    totalScripts = 0;

    while (true)
    {
        result = this->LoadExternalTextureData(fileDesc, curEntryNum, &totalSprites, &totalScripts, curEntry);
        if (result < ZUN_SUCCESS)
        {
            return NULL;
        }

        curEntryNum++;

        if (curEntry->nextOffset == 0)
        {
            break;
        }

        curEntry = (AnmRawEntry *)(((u8 *)curEntry) + curEntry->nextOffset);
    }

    return fileDesc;
}

AnmFileDesc *AnmManager::PreloadAnm(i32 anmIdx, const char *filename)
{
    AnmFileDesc *fileDesc = this->ReadAnmEntries(anmIdx, filename);
    if (fileDesc == NULL)
    {
        return NULL;
    }

    fileDesc->numberEntriesToBeLoaded = 1;
    while (fileDesc->numberEntriesToBeLoaded != 0 && !g_Supervisor.subthreadCloseRequestActive)
    {
        Sleep(1);
    }
    utils::DebugPrint("::preloadAnimEnd : %s\n", filename);

    return g_Supervisor.subthreadCloseRequestActive ? NULL : fileDesc;
}

i32 AnmManager::LoadExternalTextureData(AnmFileDesc *fileDesc, i32 entryNumber, i32 *sprites, i32 *scripts,
                                        AnmRawEntry *rawEntry)
{
    return 0;
}

#pragma var_order(currentEntryNumber, currentNumSprites, entryLoadNumber, data, result, currentNumScripts, rawEntry)
AnmFileDesc *AnmManager::PostloadAnmEntry(AnmFileDesc *fileDesc)
{
    i32 result;

    utils::DebugPrint("::postloadAnim : %d, %d\n", fileDesc->anmIdx, fileDesc->numberEntriesToBeLoaded);

    AnmRawEntry *rawData = fileDesc->rawData;

    i32 entryLoadNumber = 0;
    i32 currentNumScripts = 0;
    i32 currentNumSprites = 0;
    i32 currentEntryNumber = 0;

    /* ??? */
    fileDesc->rawData = rawData;
    AnmRawEntry *rawEntry = rawData;

    while (true)
    {
        if (entryLoadNumber == fileDesc->numberEntriesToBeLoaded - 1 &&
            (result = this->LoadTextureData(fileDesc, currentEntryNumber, currentNumSprites, currentNumScripts,
                                            rawEntry)) < ZUN_SUCCESS)
        {
            fileDesc->numberEntriesToBeLoaded = 0;
            return NULL;
        }

        currentNumSprites += rawEntry->numSprites;
        currentNumScripts += rawEntry->numScripts;
        currentEntryNumber++;

        if (rawEntry->nextOffset == 0)
        {
            break;
        }

        rawEntry = (AnmRawEntry *)(((u8 *)rawEntry) + rawEntry->nextOffset);
        entryLoadNumber++;

        if (entryLoadNumber == fileDesc->numberEntriesToBeLoaded)
        {
            fileDesc->numberEntriesToBeLoaded++;
            return fileDesc;
        }
    }

    fileDesc->numberEntriesToBeLoaded = 0;

    return fileDesc;
}

#pragma var_order(result, startOfEntry, surfaceDesc, path, rawSprite, i, currentOffset, loadedSprite)
int AnmManager::LoadTextureData(AnmFileDesc *fileDesc, i32 entryNumber, i32 currentSpriteNumber,
                                i32 currentScriptNumber, AnmRawEntry *rawEntry)
{
    int result = 0;
    AnmLoadedSprite loadedSprite;
    int i;
    const char *path;

    if (rawEntry == NULL)
    {
        g_GameErrorContext.Fatal(TH_ERR_ANMMANAGER_ANIMATION_CORRUPTED);
        return ZUN_ERROR;
    }

    AnmRawEntry *startOfEntry = rawEntry;

    if (startOfEntry->version != 3)
    {
        g_GameErrorContext.Fatal(TH_ERR_ANMMANAGER_ANIMATION_WRONG_VERSION);
        return ZUN_ERROR;
    }

    if (!startOfEntry->hasData)
    {
        path = (const char *)(((u8 *)startOfEntry) + startOfEntry->nameOffset);

        if (path[0] == '@')
        {
            this->CreateEmptyTexture(&fileDesc->textures[entryNumber].texture, startOfEntry->width,
                                     startOfEntry->height, startOfEntry->format);
        }
        else
        {
            if (this->CreateTextureFromFile(&fileDesc->textures[entryNumber].texture, startOfEntry->format,
                                            startOfEntry->colorKey) != ZUN_SUCCESS)
            {
                g_GameErrorContext.Fatal(TH_ERR_ANMMANAGER_EXTERN_TEXTURE_CORRUPTED, path);
                return ZUN_ERROR;
            }
        }
    }
    else
    {
        if (this->CreateTextureFromAnm(&fileDesc->textures[entryNumber].texture,
                                       (AnmTextureHeader *)(((u8 *)startOfEntry) + startOfEntry->textureOffset),
                                       startOfEntry->format) != ZUN_SUCCESS)
        {
            g_GameErrorContext.Fatal(TH_ERR_ANMMANAGER_TEXTURE_CORRUPTED);
            return ZUN_ERROR;
        }
    }

    fileDesc->textures[entryNumber].texture->SetPriority(startOfEntry->priority);
    fileDesc->textures[entryNumber].texture->PreLoad();

    D3DSURFACE_DESC surfaceDesc;

    fileDesc->textures[entryNumber].texture->GetLevelDesc(0, &surfaceDesc);

    u32 *currentOffset = (u32 *)((u8 *)startOfEntry + sizeof(AnmRawEntry));

    AnmRawSprite *rawSprite;

    for (i = 0; i < startOfEntry->numSprites; i++, currentOffset++)
    {
        rawSprite = (AnmRawSprite *)((u8 *)startOfEntry + *currentOffset);

        loadedSprite.anmIdx = fileDesc->anmIdx;
        loadedSprite.texture = fileDesc->textures[entryNumber].texture;
        loadedSprite.scaleFactor.x = surfaceDesc.Width / (float)startOfEntry->width;
        loadedSprite.scaleFactor.y = surfaceDesc.Height / (float)startOfEntry->height;

        loadedSprite.startPixelInclusive.x = rawSprite->x * loadedSprite.scaleFactor.x;
        loadedSprite.startPixelInclusive.y = rawSprite->y * loadedSprite.scaleFactor.y;
        loadedSprite.endPixelInclusive.x = (rawSprite->x + rawSprite->width) * loadedSprite.scaleFactor.x;
        loadedSprite.endPixelInclusive.y = (rawSprite->y + rawSprite->height) * loadedSprite.scaleFactor.y;
        loadedSprite.width = surfaceDesc.Width;
        loadedSprite.height = surfaceDesc.Height;

        fileDesc->LoadSprite(currentSpriteNumber, &loadedSprite);

        currentSpriteNumber++;
    }

    for (i = 0; i < startOfEntry->numScripts; i++, currentOffset += 2)
    {
        fileDesc->scripts[currentScriptNumber] = (AnmRawInstr *)(((u8 *)startOfEntry) + currentOffset[1]);
        currentScriptNumber++;
    }

    return result + 1;
}

ZunResult AnmManager::ServicePreloadedAnims()
{
    for (int i = 0; i < ARRAY_SIZE(this->anmFiles); i++)
    {
        if (this->anmFiles[i].numberEntriesToBeLoaded != 0 && this->PostloadAnmEntry(this->anmFiles + i) == NULL)
        {
            return ZUN_ERROR;
        }
    }

    return ZUN_SUCCESS;
}

void AnmManager::ReleaseAnm(i32 anmIdx)
{
    if (anmIdx < 0 || anmIdx >= ARRAY_SIZE(this->anmFiles))
    {
        return;
    }

    if (this->anmFiles[anmIdx].rawData != NULL)
    {
        for (int i = 0; i < this->anmFiles[anmIdx].totalEntries; i++)
        {
            this->ReleaseAnmEntry(&this->anmFiles[anmIdx].textures[i]);
        }

        g_ZunMemory.Free(this->anmFiles[anmIdx].textures);
        g_ZunMemory.Free(this->anmFiles[anmIdx].sprites);
        g_ZunMemory.Free(this->anmFiles[anmIdx].scripts);
        g_ZunMemory.Free(this->anmFiles[anmIdx].rawData);

        memset(&this->anmFiles[anmIdx], 0, sizeof(AnmFileDesc));
    }
}

void AnmManager::ReleaseAnmEntry(AnmEntry *entry)
{
    if (entry->texture != NULL)
    {
        entry->texture->Release();
        entry->texture = NULL;
    }
    if (entry->rawData != NULL)
    {
        g_ZunMemory.Free(entry->rawData);
        /* there should be a entry->rawData = NULL */
    }
}

void AnmFileDesc::LoadSprite(i32 spriteIdx, AnmLoadedSprite *loadedSprite)
{
    this->sprites[spriteIdx] = *loadedSprite;

    this->sprites[spriteIdx].uvStart.x =
        this->sprites[spriteIdx].startPixelInclusive.x / (this->sprites[spriteIdx].width);
    this->sprites[spriteIdx].uvEnd.x = this->sprites[spriteIdx].endPixelInclusive.x / (this->sprites[spriteIdx].width);
    this->sprites[spriteIdx].uvStart.y =
        this->sprites[spriteIdx].startPixelInclusive.y / (this->sprites[spriteIdx].height);
    this->sprites[spriteIdx].uvEnd.y = this->sprites[spriteIdx].endPixelInclusive.y / (this->sprites[spriteIdx].height);
    this->sprites[spriteIdx].widthPx =
        (this->sprites[spriteIdx].endPixelInclusive.x - this->sprites[spriteIdx].startPixelInclusive.x) /
        (loadedSprite->scaleFactor.x);
    this->sprites[spriteIdx].heightPx =
        (this->sprites[spriteIdx].endPixelInclusive.y - this->sprites[spriteIdx].startPixelInclusive.y) /
        (loadedSprite->scaleFactor.y);
}

#pragma var_order(surface, fileSize, fileData)
ZunResult AnmManager::LoadSurface(i32 surfaceIdx, const char *filename)
{
    u8 *fileData;
    i32 fileSize;
    IDirect3DSurface8 *surface;

    if (this->surfaces[surfaceIdx] != NULL)
    {
        this->ReleaseSurface(surfaceIdx);
    }

    if (surfaceData[surfaceIdx] == NULL)
    {
        fileData = FileSystem::OpenFile(filename, &fileSize, 0);
        if (fileData == NULL)
        {
            g_GameErrorContext.Fatal(TH_ERR_CANNOT_BE_LOADED, filename);
            return ZUN_ERROR;
        }
    }
    else
    {
        fileData = this->surfaceData[surfaceIdx];
        fileSize = this->surfaceDataSizes[surfaceIdx];
        this->surfaceData[surfaceIdx] = NULL;
    }

    if (g_Supervisor.d3dDevice->CreateImageSurface(640, 1024, g_Supervisor.presentParameters.BackBufferFormat,
                                                   &surface) != D3D_OK)
    {
        return ZUN_ERROR;
    }

    if (D3DXLoadSurfaceFromFileInMemory(surface, NULL, NULL, fileData, fileSize, NULL, 1, 0,
                                        (D3DXIMAGE_INFO *)&surfaceInfo[surfaceIdx]) != D3D_OK)
    {
        goto err;
    }

    if (g_Supervisor.d3dDevice->CreateRenderTarget(this->surfaceInfo[surfaceIdx].Width, surfaceInfo[surfaceIdx].Height,
                                                   g_Supervisor.presentParameters.BackBufferFormat, D3DMULTISAMPLE_NONE,
                                                   1, &this->surfaces[surfaceIdx]) != D3D_OK)
    {
        if (g_Supervisor.d3dDevice->CreateImageSurface(
                this->surfaceInfo[surfaceIdx].Width, this->surfaceInfo[surfaceIdx].Height,
                g_Supervisor.presentParameters.BackBufferFormat, &this->surfaces[surfaceIdx]) != D3D_OK)
        {
            goto err;
        }
    }

    if (g_Supervisor.d3dDevice->CreateImageSurface(
            this->surfaceInfo[surfaceIdx].Width, this->surfaceInfo[surfaceIdx].Height,
            g_Supervisor.presentParameters.BackBufferFormat, &this->surfacesBis[surfaceIdx]) != D3D_OK)
    {
        goto err;
    }

    if (D3DXLoadSurfaceFromSurface(this->surfaces[surfaceIdx], NULL, NULL, surface, NULL, NULL, D3DX_FILTER_NONE, 0) !=
        D3D_OK)
    {
        goto err;
    }

    if (D3DXLoadSurfaceFromSurface(this->surfacesBis[surfaceIdx], NULL, NULL, surface, NULL, NULL, D3DX_FILTER_NONE,
                                   0) != D3D_OK)
    {
        goto err;
    }

    if (surface != NULL)
    {
        surface->Release();
        surface = NULL;
    }
    g_ZunMemory.Free(fileData);

    return ZUN_SUCCESS;
err:
    if (surface != NULL)
    {
        surface->Release();
        surface = NULL;
    }
    g_ZunMemory.Free(fileData);

    return ZUN_ERROR;
}

void AnmManager::ReleaseSurface(i32 surfaceIdx)
{
    if (this->surfaces[surfaceIdx] != NULL)
    {
        this->surfaces[surfaceIdx]->Release();
        this->surfaces[surfaceIdx] = NULL;
    }
    if (this->surfacesBis[surfaceIdx] != NULL)
    {
        this->surfacesBis[surfaceIdx]->Release();
        this->surfacesBis[surfaceIdx] = NULL;
    }
    if (this->surfaceData[surfaceIdx] != NULL)
    {
        g_ZunMemory.Free(this->surfaceData[surfaceIdx]);
    }
    this->surfaceData[surfaceIdx] = NULL;
}

/* completely identical to EoSD. */
void AnmManager::CopySurfaceToBackbuffer(i32 surfaceIdx, i32 left, i32 top, i32 x, i32 y)
{
    if (this->surfacesBis[surfaceIdx] == NULL)
    {
        return;
    }

    IDirect3DSurface8 *destSurface;
    if (g_Supervisor.d3dDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &destSurface) != D3D_OK)
    {
        return;
    }
    if (this->surfaces[surfaceIdx] == NULL)
    {
        if (g_Supervisor.d3dDevice->CreateRenderTarget(
                this->surfaceInfo[surfaceIdx].Width, this->surfaceInfo[surfaceIdx].Height,
                g_Supervisor.presentParameters.BackBufferFormat, D3DMULTISAMPLE_NONE, TRUE,
                &this->surfaces[surfaceIdx]) != D3D_OK)
        {
            if (g_Supervisor.d3dDevice->CreateImageSurface(
                    this->surfaceInfo[surfaceIdx].Width, this->surfaceInfo[surfaceIdx].Height,
                    g_Supervisor.presentParameters.BackBufferFormat, &this->surfaces[surfaceIdx]) != D3D_OK)
            {
                destSurface->Release();
                return;
            }
        }
        if (D3DXLoadSurfaceFromSurface(this->surfaces[surfaceIdx], NULL, NULL, this->surfacesBis[surfaceIdx], NULL,
                                       NULL, D3DX_FILTER_NONE, 0) != D3D_OK)
        {
            destSurface->Release();
            return;
        }
    }

    RECT sourceRect;
    POINT destPoint;
    sourceRect.left = left;
    sourceRect.top = top;
    sourceRect.right = this->surfaceInfo[surfaceIdx].Width;
    sourceRect.bottom = this->surfaceInfo[surfaceIdx].Height;
    destPoint.x = x;
    destPoint.y = y;
    g_Supervisor.d3dDevice->CopyRects(this->surfaces[surfaceIdx], &sourceRect, 1, destSurface, &destPoint);
    destSurface->Release();
}
}; // Namespace th08
