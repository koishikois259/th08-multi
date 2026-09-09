#include "MultiNetProtocol.hpp"
#include <stdio.h>

using namespace th08;

namespace
{

int failures = 0;

void Expect(bool condition, const char *message)
{
    if (!condition)
    {
        ++failures;
        printf("FAIL: %s\n", message);
    }
}

MultiNetInputPacket CreatePacket()
{
    MultiNetInputPacket packet = { 0 };
    packet.sessionId = 0x10203040;
    packet.latestFrame = 100;
    packet.acknowledgedFrame = 96;
    packet.stateHashFrame = 90;
    packet.stateHash = 0xAABBCCDD;
    packet.senderSlot = 1;
    packet.sampleCount = 3;
    packet.samples[0].frame = 100;
    packet.samples[0].buttons = 0x0001;
    packet.samples[1].frame = 99;
    packet.samples[1].buttons = 0x0050;
    packet.samples[2].frame = 98;
    packet.samples[2].buttons = 0x0000;
    return packet;
}

void TestRoundTrip()
{
    MultiNetInputPacket input = CreatePacket();
    MultiNetInputPacket output = { 0 };
    u8 wire[256];
    u32 wireSize = 0;

    Expect(EncodeMultiNetInputPacket(input, wire, sizeof(wire), &wireSize), "input packet encodes");
    Expect(wireSize == MultiNetInputPacketWireSize(3), "wire size is canonical");
    Expect(DecodeMultiNetInputPacket(wire, wireSize, &output), "input packet decodes");
    Expect(output.sessionId == input.sessionId, "session id round-trips");
    Expect(output.latestFrame == input.latestFrame, "latest frame round-trips");
    Expect(output.acknowledgedFrame == input.acknowledgedFrame, "ack frame round-trips");
    Expect(output.stateHashFrame == input.stateHashFrame, "hash frame round-trips");
    Expect(output.stateHash == input.stateHash, "state hash round-trips");
    Expect(output.senderSlot == input.senderSlot, "sender slot round-trips");
    Expect(output.sampleCount == input.sampleCount, "sample count round-trips");
    Expect(output.samples[1].frame == 99, "sample frame round-trips");
    Expect(output.samples[1].buttons == 0x0050, "sample buttons round-trip");
}

void TestRejectsMalformedPackets()
{
    MultiNetInputPacket packet = CreatePacket();
    MultiNetInputPacket decoded = { 0 };
    u8 wire[256];
    u32 wireSize = 0;

    packet.sampleCount = MULTI_NET_MAX_REDUNDANT_INPUTS + 1;
    Expect(!EncodeMultiNetInputPacket(packet, wire, sizeof(wire), &wireSize), "too many samples are rejected");
    packet = CreatePacket();
    Expect(!EncodeMultiNetInputPacket(packet, wire, 12, &wireSize), "short output buffer is rejected");
    Expect(EncodeMultiNetInputPacket(packet, wire, sizeof(wire), &wireSize), "valid packet encodes for mutation");

    wire[0] ^= 1;
    Expect(!DecodeMultiNetInputPacket(wire, wireSize, &decoded), "bad magic is rejected");
    wire[0] ^= 1;
    wire[9] = 2;
    Expect(!DecodeMultiNetInputPacket(wire, wireSize, &decoded), "invalid player slot is rejected");
    wire[9] = 1;
    Expect(!DecodeMultiNetInputPacket(wire, wireSize - 1, &decoded), "truncated packet is rejected");
}

void TestHistoryAndRedundancy()
{
    MultiNetInputHistory history;
    MultiNetInputSample samples[MULTI_NET_MAX_REDUNDANT_INPUTS];
    u16 buttons = 0;
    u32 frame;
    u32 count;

    for (frame = 86; frame <= 100; ++frame)
        Expect(history.Insert(frame, static_cast<u16>(frame)), "new history input is accepted");
    count = history.BuildRedundantSamples(100, samples);
    Expect(count == MULTI_NET_MAX_REDUNDANT_INPUTS, "history emits 15 redundant frames");
    Expect(samples[0].frame == 100 && samples[14].frame == 86, "history is newest first");
    Expect(history.TryGet(93, &buttons) && buttons == 93, "history retrieves a frame");

    Expect(!history.Insert(100, 0x7777), "conflicting duplicate input is rejected");
    Expect(history.Insert(100, 100), "identical duplicate input is accepted");
    Expect(history.Insert(93 + MULTI_NET_INPUT_HISTORY_SIZE, 0x7777), "new ring generation is accepted");
    Expect(!history.TryGet(93, &buttons), "ring overwrite rejects a stale frame id");
    Expect(
        history.TryGet(93 + MULTI_NET_INPUT_HISTORY_SIZE, &buttons) && buttons == 0x7777,
        "ring retrieves the replacement frame");
}

void TestDroppedPacketRecovery()
{
    MultiNetInputHistory sender;
    MultiNetInputHistory receiver;
    MultiNetInputSample samples[MULTI_NET_MAX_REDUNDANT_INPUTS];
    u32 frame;
    u32 i;
    u32 count;
    u16 buttons;

    for (frame = 0; frame < 20; ++frame)
        Expect(sender.Insert(frame, static_cast<u16>(frame * 3)), "sender history accepts input");

    // Pretend packets for frames 5 through 9 were lost. The frame-10 packet
    // still contains all of them in its redundant history.
    count = sender.BuildRedundantSamples(10, samples);
    for (i = 0; i < count; ++i)
        Expect(receiver.Insert(samples[i].frame, samples[i].buttons), "receiver accepts redundant input");
    for (frame = 5; frame <= 10; ++frame)
        Expect(
            receiver.TryGet(frame, &buttons) && buttons == static_cast<u16>(frame * 3),
            "redundancy recovers a dropped recent input");
}

} // namespace

int main()
{
    TestRoundTrip();
    TestRejectsMalformedPackets();
    TestHistoryAndRedundancy();
    TestDroppedPacketRecovery();

    if (failures != 0)
    {
        printf("%d multiplayer network test(s) failed\n", failures);
        return 1;
    }
    printf("multiplayer network tests passed\n");
    return 0;
}
