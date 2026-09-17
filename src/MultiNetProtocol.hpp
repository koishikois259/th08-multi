#pragma once

#include "inttypes.hpp"

namespace th08
{

enum MultiNetProtocolConstant
{
    MULTI_NET_PROTOCOL_VERSION = 0x00010001,
    MULTI_NET_MIN_INPUT_DELAY = 1,
    MULTI_NET_MAX_INPUT_DELAY = 12,
    MULTI_NET_MAX_REDUNDANT_INPUTS = 15,
    MULTI_NET_INPUT_HISTORY_SIZE = 256,
    MULTI_NET_INVALID_FRAME = 0xFFFFFFFF,
    MULTI_NET_SAVE_SHOT_COUNT = 13,
    MULTI_NET_SAVE_DIFFICULTY_COUNT = 5,
    MULTI_NET_SAVE_SNAPSHOT_VERSION = 1,
};

enum MultiNetDisconnectReason
{
    MULTI_NET_DISCONNECT_NONE = 0,
    MULTI_NET_DISCONNECT_USER = 1,
    MULTI_NET_DISCONNECT_TIMEOUT = 2,
    MULTI_NET_DISCONNECT_PROTOCOL = 3,
    MULTI_NET_DISCONNECT_BUILD_MISMATCH = 4,
};

struct MultiNetHelloPacket
{
    u32 buildFingerprint;
    u32 clientNonce;
    u32 capabilities;
    u16 requestedInputDelay;
    u8 selectedTeam;
};

struct MultiNetWelcomePacket
{
    u32 sessionId;
    u32 hostNonce;
    u32 echoedClientNonce;
    u32 randomSeed;
    u16 inputDelay;
    u8 hostTeam;
    u8 guestTeam;
    u8 assignedSlot;
    u32 saveRevision;
    struct SaveProgress
    {
        u16 clearedWithoutRetries[MULTI_NET_SAVE_DIFFICULTY_COUNT];
        u16 clearedWithRetries[MULTI_NET_SAVE_DIFFICULTY_COUNT];
        u8 pendingEndingSkip;
    } saveProgress[MULTI_NET_SAVE_SHOT_COUNT];
};

struct MultiNetDisconnectPacket
{
    u32 sessionId;
    u16 reason;
    u8 senderSlot;
};

enum MultiNetPacketType
{
    MULTI_NET_PACKET_HELLO = 1,
    MULTI_NET_PACKET_WELCOME = 2,
    MULTI_NET_PACKET_INPUT = 3,
    MULTI_NET_PACKET_DISCONNECT = 4,
};

struct MultiNetInputSample
{
    u32 frame;
    u16 buttons;
};

struct MultiNetInputPacket
{
    u32 sessionId;
    u32 latestFrame;
    u32 acknowledgedFrame;
    u32 stateHashFrame;
    u32 stateHash;
    u8 senderSlot;
    u8 sampleCount;
    MultiNetInputSample samples[MULTI_NET_MAX_REDUNDANT_INPUTS];
};

class MultiNetInputHistory
{
  public:
    MultiNetInputHistory();

    void Reset();
    bool Insert(u32 frame, u16 buttons);
    bool TryGet(u32 frame, u16 *buttons) const;
    u32 BuildRedundantSamples(
        u32 latestFrame,
        MultiNetInputSample samples[MULTI_NET_MAX_REDUNDANT_INPUTS]) const;

  private:
    struct Entry
    {
        u32 frame;
        u16 buttons;
        bool occupied;
    };

    Entry entries[MULTI_NET_INPUT_HISTORY_SIZE];
};

u32 MultiNetInputPacketWireSize(u32 sampleCount);
bool GetMultiNetPacketType(const u8 *data, u32 dataSize, MultiNetPacketType *type);
bool EncodeMultiNetHelloPacket(
    const MultiNetHelloPacket &packet, u8 *output, u32 outputCapacity, u32 *outputSize);
bool DecodeMultiNetHelloPacket(
    const u8 *data, u32 dataSize, MultiNetHelloPacket *packet);
bool EncodeMultiNetWelcomePacket(
    const MultiNetWelcomePacket &packet, u8 *output, u32 outputCapacity, u32 *outputSize);
bool DecodeMultiNetWelcomePacket(
    const u8 *data, u32 dataSize, MultiNetWelcomePacket *packet);
bool EncodeMultiNetInputPacket(
    const MultiNetInputPacket &packet,
    u8 *output,
    u32 outputCapacity,
    u32 *outputSize);
bool DecodeMultiNetInputPacket(
    const u8 *data,
    u32 dataSize,
    MultiNetInputPacket *packet);
bool EncodeMultiNetDisconnectPacket(
    const MultiNetDisconnectPacket &packet, u8 *output, u32 outputCapacity, u32 *outputSize);
bool DecodeMultiNetDisconnectPacket(
    const u8 *data, u32 dataSize, MultiNetDisconnectPacket *packet);

} // namespace th08
