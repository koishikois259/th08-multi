#ifdef TH08_MULTI_STANDALONE_TEST
#include <stddef.h>
#else
#include "th_pch.h"
#endif
#include "MultiNetProtocol.hpp"

namespace th08
{

namespace
{

const u32 MULTI_NET_MAGIC = 0x54483038; // "TH08" on the wire.
const u32 MULTI_NET_INPUT_HEADER_SIZE = 32;
const u32 MULTI_NET_INPUT_SAMPLE_SIZE = 6;

void WriteU16(u8 *output, u16 value)
{
    output[0] = static_cast<u8>(value >> 8);
    output[1] = static_cast<u8>(value);
}

void WriteU32(u8 *output, u32 value)
{
    output[0] = static_cast<u8>(value >> 24);
    output[1] = static_cast<u8>(value >> 16);
    output[2] = static_cast<u8>(value >> 8);
    output[3] = static_cast<u8>(value);
}

u16 ReadU16(const u8 *input)
{
    return static_cast<u16>((static_cast<u16>(input[0]) << 8) | input[1]);
}

u32 ReadU32(const u8 *input)
{
    return (static_cast<u32>(input[0]) << 24) |
           (static_cast<u32>(input[1]) << 16) |
           (static_cast<u32>(input[2]) << 8) |
           static_cast<u32>(input[3]);
}

} // namespace

MultiNetInputHistory::MultiNetInputHistory()
{
    Reset();
}

void MultiNetInputHistory::Reset()
{
    u32 i;
    for (i = 0; i < MULTI_NET_INPUT_HISTORY_SIZE; ++i)
    {
        entries[i].frame = 0;
        entries[i].buttons = 0;
        entries[i].occupied = false;
    }
}

bool MultiNetInputHistory::Insert(u32 frame, u16 buttons)
{
    Entry &entry = entries[frame % MULTI_NET_INPUT_HISTORY_SIZE];
    if (entry.occupied && entry.frame == frame)
        return entry.buttons == buttons;
    entry.frame = frame;
    entry.buttons = buttons;
    entry.occupied = true;
    return true;
}

bool MultiNetInputHistory::TryGet(u32 frame, u16 *buttons) const
{
    const Entry &entry = entries[frame % MULTI_NET_INPUT_HISTORY_SIZE];
    if (!entry.occupied || entry.frame != frame)
        return false;
    if (buttons != NULL)
        *buttons = entry.buttons;
    return true;
}

u32 MultiNetInputHistory::BuildRedundantSamples(
    u32 latestFrame,
    MultiNetInputSample samples[MULTI_NET_MAX_REDUNDANT_INPUTS]) const
{
    u32 count = 0;
    u32 offset;
    u16 buttons;

    for (offset = 0; offset < MULTI_NET_MAX_REDUNDANT_INPUTS; ++offset)
    {
        u32 frame;
        if (offset > latestFrame)
            break;
        frame = latestFrame - offset;
        if (TryGet(frame, &buttons))
        {
            samples[count].frame = frame;
            samples[count].buttons = buttons;
            ++count;
        }
    }
    return count;
}

u32 MultiNetInputPacketWireSize(u32 sampleCount)
{
    if (sampleCount > MULTI_NET_MAX_REDUNDANT_INPUTS)
        return 0;
    return MULTI_NET_INPUT_HEADER_SIZE + sampleCount * MULTI_NET_INPUT_SAMPLE_SIZE;
}

bool EncodeMultiNetInputPacket(
    const MultiNetInputPacket &packet,
    u8 *output,
    u32 outputCapacity,
    u32 *outputSize)
{
    u32 wireSize = MultiNetInputPacketWireSize(packet.sampleCount);
    u32 i;
    u32 cursor;

    if (wireSize == 0 || output == NULL || outputSize == NULL || outputCapacity < wireSize)
        return false;

    WriteU32(output + 0, MULTI_NET_MAGIC);
    WriteU32(output + 4, MULTI_NET_PROTOCOL_VERSION);
    output[8] = MULTI_NET_PACKET_INPUT;
    output[9] = packet.senderSlot;
    WriteU16(output + 10, static_cast<u16>(wireSize - 12));
    WriteU32(output + 12, packet.sessionId);
    WriteU32(output + 16, packet.latestFrame);
    WriteU32(output + 20, packet.acknowledgedFrame);
    WriteU32(output + 24, packet.stateHashFrame);
    WriteU32(output + 28, packet.stateHash);

    cursor = MULTI_NET_INPUT_HEADER_SIZE;
    for (i = 0; i < packet.sampleCount; ++i)
    {
        WriteU32(output + cursor, packet.samples[i].frame);
        WriteU16(output + cursor + 4, packet.samples[i].buttons);
        cursor += MULTI_NET_INPUT_SAMPLE_SIZE;
    }
    *outputSize = wireSize;
    return true;
}

bool DecodeMultiNetInputPacket(
    const u8 *data,
    u32 dataSize,
    MultiNetInputPacket *packet)
{
    u32 payloadSize;
    u32 sampleBytes;
    u32 sampleCount;
    u32 cursor;
    u32 i;

    if (data == NULL || packet == NULL || dataSize < MULTI_NET_INPUT_HEADER_SIZE)
        return false;
    if (ReadU32(data + 0) != MULTI_NET_MAGIC ||
        ReadU32(data + 4) != MULTI_NET_PROTOCOL_VERSION ||
        data[8] != MULTI_NET_PACKET_INPUT)
        return false;

    payloadSize = ReadU16(data + 10);
    if (payloadSize + 12 != dataSize)
        return false;
    sampleBytes = dataSize - MULTI_NET_INPUT_HEADER_SIZE;
    if (sampleBytes % MULTI_NET_INPUT_SAMPLE_SIZE != 0)
        return false;
    sampleCount = sampleBytes / MULTI_NET_INPUT_SAMPLE_SIZE;
    if (sampleCount > MULTI_NET_MAX_REDUNDANT_INPUTS)
        return false;

    packet->senderSlot = data[9];
    if (packet->senderSlot >= 2)
        return false;
    packet->sampleCount = static_cast<u8>(sampleCount);
    packet->sessionId = ReadU32(data + 12);
    packet->latestFrame = ReadU32(data + 16);
    packet->acknowledgedFrame = ReadU32(data + 20);
    packet->stateHashFrame = ReadU32(data + 24);
    packet->stateHash = ReadU32(data + 28);

    cursor = MULTI_NET_INPUT_HEADER_SIZE;
    for (i = 0; i < sampleCount; ++i)
    {
        packet->samples[i].frame = ReadU32(data + cursor);
        packet->samples[i].buttons = ReadU16(data + cursor + 4);
        cursor += MULTI_NET_INPUT_SAMPLE_SIZE;
    }
    return true;
}

} // namespace th08
