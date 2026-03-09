#include "pbg/Lzss.hpp"
#include "pbg/PbgMemory.hpp"

#define LZSS_LOOKAHEAD_SIZE ((1 << LZSS_LENGTH_BITS) + 2)
#define LZSS_DICTSIZE_MASK (LZSS_DICTSIZE - 1)
#define LZSS_DICTPOS_MOD(pos, amount) ((pos + amount) & LZSS_DICTSIZE_MASK)

namespace th08
{
Lzss::TreeNode Lzss::tree[LZSS_DICTSIZE + 1];
u8 Lzss::dict[LZSS_DICTSIZE];

#define ENC_NEXT_BIT()                                                                                                 \
    inBitMask >>= 1;                                                                                                   \
    if (inBitMask == 0)                                                                                                \
    {                                                                                                                  \
        *outCursor++ = currByte;                                                                                       \
        checksum += currByte;                                                                                          \
        currByte = 0;                                                                                                  \
        inBitMask = 0x80;                                                                                              \
    }

#define ENC_WRITE_FLAG_BIT(bit)                                                                                        \
    if (bit)                                                                                                           \
    {                                                                                                                  \
        currByte |= inBitMask;                                                                                         \
    }                                                                                                                  \
    ENC_NEXT_BIT();

#define ENC_WRITE_BITS(bitCount, condition)                                                                            \
    bitfieldMask = 0x1 << (bitCount - 1);                                                                              \
    while (bitfieldMask != 0)                                                                                          \
    {                                                                                                                  \
        if (condition)                                                                                                 \
        {                                                                                                              \
            currByte |= inBitMask;                                                                                     \
        }                                                                                                              \
        ENC_NEXT_BIT();                                                                                                \
        bitfieldMask >>= 1;                                                                                            \
    }

#pragma var_order(currByte, out, outCursor, matchOffset, i, bytesToCopyToDict, inBitMask, inCursor, matchLength,       \
                  checksum, lookAheadBytes, dictValue, dictHead, bitfieldMask)
LPBYTE Lzss::Encode(LPBYTE in, i32 uncompressedSize, i32 *compressedSize)
{
    i32 i;
    i32 bytesToCopyToDict;
    i32 lookAheadBytes;
    i32 dictValue;
    u32 bitfieldMask;

    u8 inBitMask = 0x80;
    u32 currByte = 0;
    u32 checksum = 0;

    LPBYTE out = (LPBYTE)MemAlloc(uncompressedSize * 2);
    if (out == NULL)
    {
        return NULL;
    }

    LPBYTE inCursor = in;
    LPBYTE outCursor = out;
    *compressedSize = 0;

    InitEncoderState();

    u32 dictHead = 1;
    for (i = 0; i < LZSS_LOOKAHEAD_SIZE; i++)
    {
        if (inCursor - in >= uncompressedSize)
        {
            dictValue = -1;
        }
        else
        {
            dictValue = *inCursor++;
        }

        if (dictValue == -1)
        {
            break;
        }

        dict[dictHead + i] = dictValue;
    }

    lookAheadBytes = i;
    InitTree(dictHead);
    i32 matchLength = 0;
    i32 matchOffset = 0;

    while (lookAheadBytes > 0)
    {
        if (matchLength > lookAheadBytes)
        {
            matchLength = lookAheadBytes;
        }

        if (matchLength <= 2)
        {
            bytesToCopyToDict = 1;

            ENC_WRITE_FLAG_BIT(1);
            ENC_WRITE_BITS(8, (bitfieldMask & dict[dictHead]) != 0);
        }
        else
        {
            ENC_WRITE_FLAG_BIT(0);
            ENC_WRITE_BITS(LZSS_OFFSET_BITS, (bitfieldMask & matchOffset) != 0);
            ENC_WRITE_BITS(LZSS_LENGTH_BITS, (bitfieldMask & (matchLength - 3)) != 0);

            bytesToCopyToDict = matchLength;
        }

        for (i = 0; i < bytesToCopyToDict; i++)
        {
            DeleteString(LZSS_DICTPOS_MOD(dictHead, LZSS_LOOKAHEAD_SIZE));

            if (inCursor - in >= uncompressedSize)
            {
                dictValue = -1;
            }
            else
            {
                dictValue = *inCursor++;
            }

            if (dictValue == -1)
            {
                lookAheadBytes--;
            }
            else
            {
                dict[LZSS_DICTPOS_MOD(dictHead, LZSS_LOOKAHEAD_SIZE)] = dictValue;
            }

            dictHead = LZSS_DICTPOS_MOD(dictHead, 1);

            if (lookAheadBytes != 0)
            {
                matchLength = AddString(dictHead, &matchOffset);
            }
        }
    }

    ENC_WRITE_FLAG_BIT(0);
    ENC_WRITE_BITS(LZSS_OFFSET_BITS, FALSE);

    *compressedSize = outCursor - out;
    return out;
}

#define DEC_NEXT_BIT()                                                                                                 \
    inBitMask >>= 1;                                                                                                   \
    if (inBitMask == 0)                                                                                                \
    {                                                                                                                  \
        inBitMask = 0x80;                                                                                              \
    }

#define DEC_WRITE_BYTE(data)                                                                                           \
    *outCursor++ = data;                                                                                               \
    dict[dictHead] = data;                                                                                           \
    dictHead = LZSS_DICTPOS_MOD(dictHead, 1);

#define DEC_HANDLE_FETCH_NEW_BYTE()                                                                                    \
    if (inBitMask == 0x80)                                                                                             \
    {                                                                                                                  \
        currByte = *inCursor;                                                                                          \
        if (inCursor - in >= size)                                                                                     \
        {                                                                                                              \
            currByte = 0;                                                                                              \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            inCursor++;                                                                                                \
        }                                                                                                              \
        checksum += currByte;                                                                                          \
    }

#define DEC_READ_FLAG_BIT()                                                                                            \
    DEC_HANDLE_FETCH_NEW_BYTE();                                                                                       \
    inBits = currByte & inBitMask;                                                                                     \
    DEC_NEXT_BIT();

#define DEC_READ_BITS(bitsCount)                                                                                       \
    outBitMask = 0x01 << (bitsCount - 1);                                                                              \
    inBits = 0;                                                                                                        \
    while (outBitMask != 0)                                                                                            \
    {                                                                                                                  \
        DEC_HANDLE_FETCH_NEW_BYTE();                                                                                   \
        if ((currByte & inBitMask) != 0)                                                                               \
        {                                                                                                              \
            inBits |= outBitMask;                                                                                      \
        }                                                                                                              \
                                                                                                                       \
        outBitMask >>= 1;                                                                                              \
        DEC_NEXT_BIT();                                                                                                \
    }

#pragma var_order(currByte, outCursor, matchOffset, i, inBitMask, inCursor, inBits, size, matchLength, checksum,       \
                  dictValue, outBitMask, dictHead)
LPBYTE Lzss::Decode(LPBYTE in, i32 compressedSize, LPBYTE out, i32 decompressedSize)
{
    i32 i;
    u32 matchOffset;
    u32 inBits;
    i32 matchLength;
    u32 dictValue;
    u32 outBitMask;

    u8 inBitMask = 0x80;
    u32 currByte = 0;
    u32 checksum = 0;
    i32 size = compressedSize;

    if (out == NULL)
    {
        out = (u8 *)MemAlloc(decompressedSize);
        if (out == NULL)
        {
            return NULL;
        }
    }

    LPBYTE inCursor = in;
    LPBYTE outCursor = out;
    u32 dictHead = 1;

    for (;;)
    {
        DEC_READ_FLAG_BIT();

        // Read literal byte from next 8 bits
        if (inBits != 0)
        {
            DEC_READ_BITS(8);
            DEC_WRITE_BYTE(inBits);
        }
        // Copy from dictionary, 13 bit offset, then 4 bit length
        else
        {
            DEC_READ_BITS(13);

            matchOffset = inBits;
            if (matchOffset == 0)
            {
                break;
            }

            DEC_READ_BITS(4);

            // Value encoded in 4 bit length is 3 less than the actual length
            matchLength = inBits + 2;
            for (i = 0; i <= matchLength; i++)
            {
                dictValue = dict[LZSS_DICTPOS_MOD(matchOffset, i)];
                DEC_WRITE_BYTE(dictValue);
            }
        }
    }

    // Read any trailing bits in the data
    while (inBitMask != 0x80)
    {
        DEC_READ_FLAG_BIT();
    }

    return out;
}

void Lzss::InitTree(i32 root)
{
    tree[LZSS_DICTSIZE].right = root;
    tree[root].parent = LZSS_DICTSIZE;
    tree[root].right = 0;
    tree[root].left = 0;
}

void Lzss::InitEncoderState()
{
    i32 i;

    for (i = 0; i < LZSS_DICTSIZE; i++)
    {
        dict[i] = 0;
    }
    for (i = 0; i < LZSS_DICTSIZE + 1; i++)
    {
        tree[i].parent = 0;
        tree[i].left = 0;
        tree[i].right = 0;
    }
}

#pragma var_order(i, child, testNode, matchLength, delta)
i32 Lzss::AddString(i32 newNode, i32 *matchPosition)
{
    i32 i;
    i32 *child;
    i32 delta;

    if (newNode == 0)
    {
        return 0;
    }

    i32 testNode = tree[LZSS_DICTSIZE].right;
    i32 matchLength = 0;

    for (;;)
    {
        for (i = 0; i < LZSS_LOOKAHEAD_SIZE; i++)
        {
            delta = dict[LZSS_DICTPOS_MOD(newNode, i)] - dict[LZSS_DICTPOS_MOD(testNode, i)];

            if (delta != 0)
            {
                break;
            }
        }

        if (i >= matchLength)
        {
            matchLength = i;
            *matchPosition = testNode;

            if (matchLength >= LZSS_LOOKAHEAD_SIZE)
            {
                ReplaceNode(testNode, newNode);
                return matchLength;
            }
        }

        if (delta >= 0)
        {
            child = &tree[testNode].right;
        }
        else
        {
            child = &tree[testNode].left;
        }

        if (*child == 0)
        {
            *child = newNode;
            tree[newNode].parent = testNode;
            tree[newNode].right = 0;
            tree[newNode].left = 0;
            return matchLength;
        }

        testNode = *child;
    }
}

void Lzss::DeleteString(i32 p)
{
    if (tree[p].parent == 0)
    {
        return;
    }

    if (tree[p].right == 0)
    {
        ContractNode(p, tree[p].left);
    }
    else if (tree[p].left == 0)
    {
        ContractNode(p, tree[p].right);
    }
    else
    {
        i32 replacement = FindNextNode(p);
        DeleteString(replacement);
        ReplaceNode(p, replacement);
    }
}

void Lzss::ContractNode(i32 oldNode, i32 newNode)
{
    tree[newNode].parent = tree[oldNode].parent;

    if (tree[tree[oldNode].parent].right == oldNode)
    {
        tree[tree[oldNode].parent].right = newNode;
    }
    else
    {
        tree[tree[oldNode].parent].left = newNode;
    }
    tree[oldNode].parent = 0;
}

void Lzss::ReplaceNode(i32 oldNode, i32 newNode)
{
    i32 parent = tree[oldNode].parent;

    if (tree[parent].left == oldNode)
    {
        tree[parent].left = newNode;
    }
    else
    {
        tree[parent].right = newNode;
    }
    tree[newNode] = tree[oldNode];
    tree[tree[newNode].left].parent = newNode;
    tree[tree[newNode].right].parent = newNode;
    tree[oldNode].parent = 0;
}

i32 Lzss::FindNextNode(i32 node)
{
    i32 next = tree[node].left;

    while (tree[next].right != 0)
    {
        next = tree[next].right;
    }
    return next;
}
}; // namespace th08
