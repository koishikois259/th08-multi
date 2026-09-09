#pragma once

#include "inttypes.hpp"

namespace th08
{

enum MultiNetProtocolConstant
{
    MULTI_NET_PROTOCOL_VERSION = 0x00010000,
    MULTI_NET_MAX_REDUNDANT_INPUTS = 15,
    MULTI_NET_INPUT_HISTORY_SIZE = 256,
    MULTI_NET_INVALID_FRAME = 0xFFFFFFFF,
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
bool EncodeMultiNetInputPacket(
    const MultiNetInputPacket &packet,
    u8 *output,
    u32 outputCapacity,
    u32 *outputSize);
bool DecodeMultiNetInputPacket(
    const u8 *data,
    u32 dataSize,
    MultiNetInputPacket *packet);

} // namespace th08
