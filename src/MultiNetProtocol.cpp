#ifndef TH08_MULTI_PROTOCOL_STANDALONE
#include "th_pch.h"
#else
#include <stddef.h>
#endif

#include "MultiNetProtocol.hpp"

namespace th08
{

namespace
{

const u32 MULTI_NET_MAGIC = 0x54483038; // "TH08" on the wire.
const u32 MULTI_NET_COMMON_HEADER_SIZE = 12;
const u32 MULTI_NET_HELLO_SIZE = 28;
const u32 MULTI_NET_SAVE_PROGRESS_SIZE = 21;
const u32 MULTI_NET_WELCOME_PROGRESS_OFFSET = 44;
const u32 MULTI_NET_WELCOME_SIZE =
    MULTI_NET_WELCOME_PROGRESS_OFFSET +
    MULTI_NET_SAVE_SHOT_COUNT * MULTI_NET_SAVE_PROGRESS_SIZE;
const u32 MULTI_NET_INPUT_HEADER_SIZE = 32;
const u32 MULTI_NET_INPUT_SAMPLE_SIZE = 6;
const u32 MULTI_NET_DISCONNECT_SIZE = 20;

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

bool ValidateHeader(const u8 *data, u32 dataSize, MultiNetPacketType type)
{
    if (data == NULL || dataSize < MULTI_NET_COMMON_HEADER_SIZE)
        return false;
    return ReadU32(data + 0) == MULTI_NET_MAGIC &&
           ReadU32(data + 4) == MULTI_NET_PROTOCOL_VERSION &&
           data[8] == type && ReadU16(data + 10) + MULTI_NET_COMMON_HEADER_SIZE == dataSize;
}

void WriteHeader(u8 *output, MultiNetPacketType type, u8 senderSlot, u32 wireSize)
{
    WriteU32(output + 0, MULTI_NET_MAGIC);
    WriteU32(output + 4, MULTI_NET_PROTOCOL_VERSION);
    output[8] = static_cast<u8>(type);
    output[9] = senderSlot;
    WriteU16(output + 10, static_cast<u16>(wireSize - MULTI_NET_COMMON_HEADER_SIZE));
}

} // namespace

bool GetMultiNetPacketType(const u8 *data, u32 dataSize, MultiNetPacketType *type)
{
    if (data == NULL || type == NULL || dataSize < MULTI_NET_COMMON_HEADER_SIZE ||
        ReadU32(data + 0) != MULTI_NET_MAGIC ||
        ReadU32(data + 4) != MULTI_NET_PROTOCOL_VERSION ||
        ReadU16(data + 10) + MULTI_NET_COMMON_HEADER_SIZE != dataSize ||
        data[8] < MULTI_NET_PACKET_HELLO || data[8] > MULTI_NET_PACKET_DISCONNECT)
        return false;
    *type = static_cast<MultiNetPacketType>(data[8]);
    return true;
}

bool EncodeMultiNetHelloPacket(
    const MultiNetHelloPacket &packet, u8 *output, u32 outputCapacity, u32 *outputSize)
{
    if (output == NULL || outputSize == NULL || outputCapacity < MULTI_NET_HELLO_SIZE ||
        packet.clientNonce == 0 || packet.capabilities != 0 ||
        packet.selectedTeam >= 4 || packet.requestedInputDelay < MULTI_NET_MIN_INPUT_DELAY ||
        packet.requestedInputDelay > MULTI_NET_MAX_INPUT_DELAY)
        return false;
    WriteHeader(output, MULTI_NET_PACKET_HELLO, 1, MULTI_NET_HELLO_SIZE);
    WriteU32(output + 12, packet.buildFingerprint);
    WriteU32(output + 16, packet.clientNonce);
    WriteU32(output + 20, packet.capabilities);
    WriteU16(output + 24, packet.requestedInputDelay);
    output[26] = packet.selectedTeam;
    output[27] = 0;
    *outputSize = MULTI_NET_HELLO_SIZE;
    return true;
}

bool DecodeMultiNetHelloPacket(const u8 *data, u32 dataSize, MultiNetHelloPacket *packet)
{
    if (packet == NULL || dataSize != MULTI_NET_HELLO_SIZE ||
        !ValidateHeader(data, dataSize, MULTI_NET_PACKET_HELLO) || data[9] != 1)
        return false;
    packet->buildFingerprint = ReadU32(data + 12);
    packet->clientNonce = ReadU32(data + 16);
    packet->capabilities = ReadU32(data + 20);
    packet->requestedInputDelay = ReadU16(data + 24);
    packet->selectedTeam = data[26];
    return packet->clientNonce != 0 && packet->capabilities == 0 && data[27] == 0 &&
           packet->selectedTeam < 4 &&
           packet->requestedInputDelay >= MULTI_NET_MIN_INPUT_DELAY &&
           packet->requestedInputDelay <= MULTI_NET_MAX_INPUT_DELAY;
}

bool EncodeMultiNetWelcomePacket(
    const MultiNetWelcomePacket &packet, u8 *output, u32 outputCapacity, u32 *outputSize)
{
    u32 shot;
    u32 difficulty;
    u32 cursor;
    if (output == NULL || outputSize == NULL || outputCapacity < MULTI_NET_WELCOME_SIZE ||
        packet.sessionId == 0 || packet.hostNonce == 0 || packet.randomSeed == 0 ||
        packet.saveRevision == 0 ||
        packet.hostTeam >= 4 || packet.guestTeam >= 4 || packet.assignedSlot != 1 ||
        packet.initialLives > MULTI_NET_MAX_INITIAL_LIVES ||
        packet.inputDelay < MULTI_NET_MIN_INPUT_DELAY || packet.inputDelay > MULTI_NET_MAX_INPUT_DELAY)
        return false;
    for (shot = 0; shot < MULTI_NET_SAVE_SHOT_COUNT; ++shot)
    {
        if (packet.saveProgress[shot].pendingEndingSkip > 1)
            return false;
    }
    WriteHeader(output, MULTI_NET_PACKET_WELCOME, 0, MULTI_NET_WELCOME_SIZE);
    WriteU32(output + 12, packet.sessionId);
    WriteU32(output + 16, packet.hostNonce);
    WriteU32(output + 20, packet.echoedClientNonce);
    WriteU32(output + 24, packet.randomSeed);
    WriteU16(output + 28, packet.inputDelay);
    output[30] = packet.hostTeam;
    output[31] = packet.guestTeam;
    output[32] = packet.assignedSlot;
    output[33] = MULTI_NET_SAVE_SNAPSHOT_VERSION;
    output[34] = MULTI_NET_SAVE_SHOT_COUNT;
    output[35] = MULTI_NET_SAVE_DIFFICULTY_COUNT;
    WriteU32(output + 36, packet.saveRevision);
    output[40] = packet.initialLives;
    output[41] = output[42] = output[43] = 0;
    cursor = MULTI_NET_WELCOME_PROGRESS_OFFSET;
    for (shot = 0; shot < MULTI_NET_SAVE_SHOT_COUNT; ++shot)
    {
        for (difficulty = 0; difficulty < MULTI_NET_SAVE_DIFFICULTY_COUNT; ++difficulty)
        {
            WriteU16(output + cursor, packet.saveProgress[shot].clearedWithoutRetries[difficulty]);
            cursor += 2;
        }
        for (difficulty = 0; difficulty < MULTI_NET_SAVE_DIFFICULTY_COUNT; ++difficulty)
        {
            WriteU16(output + cursor, packet.saveProgress[shot].clearedWithRetries[difficulty]);
            cursor += 2;
        }
        output[cursor++] = packet.saveProgress[shot].pendingEndingSkip;
    }
    *outputSize = MULTI_NET_WELCOME_SIZE;
    return true;
}

bool DecodeMultiNetWelcomePacket(const u8 *data, u32 dataSize, MultiNetWelcomePacket *packet)
{
    u32 shot;
    u32 difficulty;
    u32 cursor;
    if (packet == NULL || dataSize != MULTI_NET_WELCOME_SIZE ||
        !ValidateHeader(data, dataSize, MULTI_NET_PACKET_WELCOME) || data[9] != 0)
        return false;
    packet->sessionId = ReadU32(data + 12);
    packet->hostNonce = ReadU32(data + 16);
    packet->echoedClientNonce = ReadU32(data + 20);
    packet->randomSeed = ReadU32(data + 24);
    packet->inputDelay = ReadU16(data + 28);
    packet->hostTeam = data[30];
    packet->guestTeam = data[31];
    packet->assignedSlot = data[32];
    packet->saveRevision = ReadU32(data + 36);
    packet->initialLives = data[40];
    if (data[33] != MULTI_NET_SAVE_SNAPSHOT_VERSION ||
        data[34] != MULTI_NET_SAVE_SHOT_COUNT ||
        data[35] != MULTI_NET_SAVE_DIFFICULTY_COUNT || packet->saveRevision == 0 ||
        packet->initialLives > MULTI_NET_MAX_INITIAL_LIVES ||
        data[41] != 0 || data[42] != 0 || data[43] != 0)
        return false;
    cursor = MULTI_NET_WELCOME_PROGRESS_OFFSET;
    for (shot = 0; shot < MULTI_NET_SAVE_SHOT_COUNT; ++shot)
    {
        for (difficulty = 0; difficulty < MULTI_NET_SAVE_DIFFICULTY_COUNT; ++difficulty)
        {
            packet->saveProgress[shot].clearedWithoutRetries[difficulty] = ReadU16(data + cursor);
            cursor += 2;
        }
        for (difficulty = 0; difficulty < MULTI_NET_SAVE_DIFFICULTY_COUNT; ++difficulty)
        {
            packet->saveProgress[shot].clearedWithRetries[difficulty] = ReadU16(data + cursor);
            cursor += 2;
        }
        packet->saveProgress[shot].pendingEndingSkip = data[cursor++];
        if (packet->saveProgress[shot].pendingEndingSkip > 1)
            return false;
    }
    return packet->sessionId != 0 && packet->hostNonce != 0 && packet->randomSeed != 0 &&
           packet->hostTeam < 4 && packet->guestTeam < 4 &&
           packet->assignedSlot == 1 && packet->inputDelay >= MULTI_NET_MIN_INPUT_DELAY &&
           packet->inputDelay <= MULTI_NET_MAX_INPUT_DELAY;
}

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
    if (sampleCount == 0 || sampleCount > MULTI_NET_MAX_REDUNDANT_INPUTS)
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

    if (wireSize == 0 || output == NULL || outputSize == NULL || outputCapacity < wireSize ||
        packet.sessionId == 0 || packet.senderSlot >= 2 ||
        packet.samples[0].frame != packet.latestFrame)
        return false;
    for (i = 1; i < packet.sampleCount; ++i)
    {
        if (packet.samples[i].frame >= packet.samples[i - 1].frame)
            return false;
    }

    WriteHeader(output, MULTI_NET_PACKET_INPUT, packet.senderSlot, wireSize);
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
    if (!ValidateHeader(data, dataSize, MULTI_NET_PACKET_INPUT))
        return false;

    payloadSize = ReadU16(data + 10);
    if (payloadSize + 12 != dataSize)
        return false;
    sampleBytes = dataSize - MULTI_NET_INPUT_HEADER_SIZE;
    if (sampleBytes % MULTI_NET_INPUT_SAMPLE_SIZE != 0)
        return false;
    sampleCount = sampleBytes / MULTI_NET_INPUT_SAMPLE_SIZE;
    if (sampleCount == 0 || sampleCount > MULTI_NET_MAX_REDUNDANT_INPUTS)
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
    if (packet->sessionId == 0 || packet->samples[0].frame != packet->latestFrame)
        return false;
    for (i = 1; i < sampleCount; ++i)
    {
        if (packet->samples[i].frame >= packet->samples[i - 1].frame)
            return false;
    }
    return true;
}

bool EncodeMultiNetDisconnectPacket(
    const MultiNetDisconnectPacket &packet, u8 *output, u32 outputCapacity, u32 *outputSize)
{
    if (output == NULL || outputSize == NULL || outputCapacity < MULTI_NET_DISCONNECT_SIZE ||
        packet.sessionId == 0 || packet.senderSlot >= 2 ||
        packet.reason < MULTI_NET_DISCONNECT_USER ||
        packet.reason > MULTI_NET_DISCONNECT_BUILD_MISMATCH)
        return false;
    WriteHeader(output, MULTI_NET_PACKET_DISCONNECT, packet.senderSlot, MULTI_NET_DISCONNECT_SIZE);
    WriteU32(output + 12, packet.sessionId);
    WriteU16(output + 16, packet.reason);
    output[18] = packet.senderSlot;
    output[19] = 0;
    *outputSize = MULTI_NET_DISCONNECT_SIZE;
    return true;
}

bool DecodeMultiNetDisconnectPacket(
    const u8 *data, u32 dataSize, MultiNetDisconnectPacket *packet)
{
    if (packet == NULL || dataSize != MULTI_NET_DISCONNECT_SIZE ||
        !ValidateHeader(data, dataSize, MULTI_NET_PACKET_DISCONNECT) || data[9] >= 2 || data[18] != data[9])
        return false;
    packet->sessionId = ReadU32(data + 12);
    packet->reason = ReadU16(data + 16);
    packet->senderSlot = data[18];
    return packet->sessionId != 0 && data[19] == 0 &&
           packet->reason >= MULTI_NET_DISCONNECT_USER &&
           packet->reason <= MULTI_NET_DISCONNECT_BUILD_MISMATCH;
}

} // namespace th08
