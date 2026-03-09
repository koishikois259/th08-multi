#include "pbg/PbgArchive.hpp"
#include "Global.hpp"
#include "i18n.hpp"
#include "pbg/Lzss.hpp"
#include "utils.hpp"

namespace th08
{
PbgArchive::PbgArchive()
{
    this->entries = NULL;
    this->numOfEntries = 0;
    this->filename = NULL;
    this->fileAbstraction = NULL;
}

PbgArchive::~PbgArchive()
{
    Release();
}

bool PbgArchive::Load(LPCSTR filename)
{
    Release();
    utils::DebugPrint("info : %s open arcfile\r\n", filename);

    this->fileAbstraction = NewEx(CPbgFile());
    if (this->fileAbstraction == NULL)
    {
        return false;
    }

    if (ParseHeader(filename))
    {
        this->filename = CopyFileName(filename);
        if (this->filename != NULL)
        {
            return true;
        }
    }

    utils::DebugPrint("info : %s not found\r\n", filename);
    Release();
    return false;
}

void PbgArchive::Release()
{
    if (this->filename != NULL)
    {
        utils::DebugPrint("info : %s close arcfile\r\n", this->filename);
    }
    MemFree(this->filename);
    DeleteArray(this->entries);
    DeleteEx(this->fileAbstraction);
    this->numOfEntries = 0;
}

#pragma var_order(entry, decompressedSize, decompressedData, compressedData, compressedSize)
LPBYTE PbgArchive::ReadDecompressEntry(LPCSTR filename, LPBYTE outBuffer)
{
    LPBYTE decompressedData;
    LPBYTE compressedData = NULL;
    u32 compressedSize;
    u32 decompressedSize;

    if (this->fileAbstraction == NULL)
    {
        return NULL;
    }

    PbgArchiveEntry *entry = FindEntry(filename);
    if (entry == NULL)
    {
        goto entry_read_error;
    }

    if (this->fileAbstraction->Open(this->filename, g_PbgFileOpenModes[0]) == false)
    {
        goto entry_read_error;
    }

    compressedSize = entry[1].dataOffset - entry->dataOffset;
    decompressedSize = entry->decompressedSize;

    compressedData = (LPBYTE)MemAlloc(compressedSize);
    if (compressedData == NULL)
    {
        goto entry_read_error;
    }
    if (this->fileAbstraction->Seek(entry->dataOffset, g_PbgFileSeekModes[0]) == false)
    {
        goto entry_read_error;
    }
    if (this->fileAbstraction->Read(compressedData, compressedSize) == 0)
    {
        goto entry_read_error;
    }

    decompressedData = Lzss::Decode(compressedData, compressedSize, outBuffer, decompressedSize);
    MemFree(compressedData);
    return decompressedData;

entry_read_error:
    utils::DebugPrint("info : %s error\r\n", this->filename);
    MemFree(compressedData);
    return NULL;
}

DWORD PbgArchive::GetEntryDecompressedSize(LPCSTR filename)
{
    PbgArchiveEntry *entry = FindEntry(filename);
    if (entry != NULL)
        return entry->decompressedSize;
    return 0;
}

PbgArchiveEntry *PbgArchive::FindEntry(LPCSTR filename)
{
    if (this->entries == NULL)
    {
        return NULL;
    }

    PbgArchiveEntry *entry = this->entries;
    for (i32 i = this->numOfEntries; i > 0; i--, entry++ /* PBG why */)
    {
        if (_stricmp(filename, entry->filename) == 0)
            return entry;
    }
    return NULL;
}

#pragma var_order(entryBuffer, decompressedSize, magic, size, fileTableOffset, fileTableBuffer, header,                \
                  decryptedHeader, decryptedFileTable)
bool PbgArchive::ParseHeader(LPCSTR filename)
{
    LPBYTE entryBuffer;
    i32 decompressedSize;
    i32 magic;
    DWORD size;
    i32 fileTableOffset;
    union {
        PbgArchiveHeader asStruct;
        BYTE asBytes[sizeof(PbgArchiveHeader)];
    } header;
    LPBYTE fileTableBuffer;
    LPBYTE decryptedHeader;
    LPBYTE decryptedFileTable;

    fileTableBuffer = NULL;
    entryBuffer = NULL;

    if (this->fileAbstraction == NULL)
    {
        return false;
    }
    if (!this->fileAbstraction->Open(filename, g_PbgFileOpenModes[0]))
    {
        goto parse_error;
    }
    if (this->fileAbstraction->ReadInt(&magic) == 0)
    {
        goto parse_error;
    }
    if (magic != 'ZGBP')
    {
        goto parse_error;
    }
    if (this->fileAbstraction->Read(header.asBytes, sizeof(header)) == 0)
    {
        goto parse_error;
    }

    decryptedHeader = FileSystem::Decrypt(header.asBytes, sizeof(header), 0x1b, 0x37, sizeof(header), 0x400);
    memcpy(&header.asStruct, decryptedHeader, sizeof(header));
    g_ZunMemory.Free(decryptedHeader);

    this->numOfEntries = header.asStruct.numOfEntries - 123456;
    fileTableOffset = header.asStruct.fileTableOffset - 345678;
    decompressedSize = header.asStruct.unk - 567891;

    if (this->numOfEntries <= 0)
    {
        goto parse_error;
    }

    size = this->fileAbstraction->GetSize();
    if (fileTableOffset >= size)
    {
        goto parse_error;
    }
    size -= fileTableOffset;

    this->fileAbstraction->Seek(fileTableOffset, g_PbgFileSeekModes[0]);

    fileTableBuffer = (LPBYTE)MemAlloc(size);
    if (fileTableBuffer == NULL)
    {
        goto parse_error;
    }
    if (this->fileAbstraction->Read(fileTableBuffer, size) == 0)
    {
        goto parse_error;
    }

    decryptedFileTable = FileSystem::Decrypt(fileTableBuffer, size, 0x3e, 0x9b, 0x80, 0x400);
    MemFree(fileTableBuffer);
    fileTableBuffer = decryptedFileTable;

    entryBuffer = Lzss::Decode(fileTableBuffer, size, NULL, decompressedSize);
    if (entryBuffer == NULL)
    {
        goto parse_error;
    }

    this->entries = AllocEntries(entryBuffer, this->numOfEntries, fileTableOffset);
    if (this->entries == NULL)
    {
        goto parse_error;
    }

    g_ZunMemory.Free(fileTableBuffer);
    MemFree(entryBuffer);
    return true;

parse_error:
    g_ZunMemory.Free(fileTableBuffer);
    MemFree(entryBuffer);
    DeleteEx(this->fileAbstraction);
    utils::DebugPrint(TH_ERR_ARCFILE_CORRUPTED, filename);

    while (false)
        ; // Yes this is correct. No, I don't get it either.

    return false;
}

#pragma var_order(entryData, i, buffer)
PbgArchiveEntry *PbgArchive::AllocEntries(LPVOID entryBuffer, i32 count, u32 dataOffset)
{
    LPVOID entryData;
    int i;
    PbgArchiveEntry *buffer = NULL;

    buffer = new PbgArchiveEntry[count + 1]();
    if (buffer == NULL)
    {
        goto buffer_alloc_error;
    }

    entryData = entryBuffer;
    for (i = 0; i < count; i++)
    {
        buffer[i].filename = CopyFileName((char *)entryData);
        SeekPastString(&entryData);
        buffer[i].dataOffset = *(u32 *)entryData;
        SeekPastInt(&entryData);
        buffer[i].decompressedSize = *(u32 *)entryData;
        SeekPastInt(&entryData);
        buffer[i].unk = *(u32 *)entryData;
        SeekPastInt(&entryData);
    }

    buffer[count].dataOffset = dataOffset;
    buffer[count].decompressedSize = 0;
    return buffer;

buffer_alloc_error:
    DeleteArray(buffer);
    return NULL;
}

char *PbgArchive::CopyFileName(LPCSTR filename)
{
    char *mem = (char *)MemAlloc(strlen(filename) + 1);
    if (mem != NULL)
    {
        strcpy(mem, filename);
    }
    return mem;
}

i32 PbgArchive::SeekPastInt(LPVOID *ptr)
{
    *ptr = (i32 *)*ptr + 1;
    return *(i32 *)*ptr;
}

LPVOID PbgArchive::SeekPastString(LPVOID *ptr)
{
    *ptr = (char *)*ptr + (strlen((char *)*ptr) + 1);
    return *ptr;
}
}; // namespace th08
