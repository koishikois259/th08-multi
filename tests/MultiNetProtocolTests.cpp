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

    packet = CreatePacket();
    packet.sessionId = 0;
    Expect(!EncodeMultiNetInputPacket(packet, wire, sizeof(wire), &wireSize),
           "zero session id is rejected");
    packet = CreatePacket();
    packet.samples[1].frame = packet.samples[0].frame;
    Expect(!EncodeMultiNetInputPacket(packet, wire, sizeof(wire), &wireSize),
           "non-descending redundant frames are rejected");
}

void TestHandshakeAndDisconnectGuards()
{
    MultiNetHelloPacket hello = { 0 };
    MultiNetWelcomePacket welcome = { 0 };
    MultiNetDisconnectPacket disconnect = { 0 };
    MultiNetHelloPacket decodedHello = { 0 };
    MultiNetWelcomePacket decodedWelcome = { 0 };
    MultiNetDisconnectPacket decodedDisconnect = { 0 };
    u8 wire[256];
    u32 wireSize;

    hello.buildFingerprint = 0x00020026;
    hello.clientNonce = 0x12345678;
    hello.requestedInputDelay = 3;
    hello.selectedTeam = 2;
    Expect(EncodeMultiNetHelloPacket(hello, wire, sizeof(wire), &wireSize),
           "valid hello encodes");
    Expect(DecodeMultiNetHelloPacket(wire, wireSize, &decodedHello),
           "valid hello decodes");
    wire[27] = 1;
    Expect(!DecodeMultiNetHelloPacket(wire, wireSize, &decodedHello),
           "nonzero hello reserved byte is rejected");
    hello.clientNonce = 0;
    Expect(!EncodeMultiNetHelloPacket(hello, wire, sizeof(wire), &wireSize),
           "zero client nonce is rejected");

    welcome.sessionId = 0x11223344;
    welcome.hostNonce = 0x22334455;
    welcome.echoedClientNonce = 0x12345678;
    welcome.randomSeed = 0x33445566;
    welcome.inputDelay = 3;
    welcome.hostTeam = 1;
    welcome.guestTeam = 2;
    welcome.assignedSlot = 1;
    Expect(EncodeMultiNetWelcomePacket(welcome, wire, sizeof(wire), &wireSize),
           "valid welcome encodes");
    Expect(DecodeMultiNetWelcomePacket(wire, wireSize, &decodedWelcome),
           "valid welcome decodes");
    wire[35] = 1;
    Expect(!DecodeMultiNetWelcomePacket(wire, wireSize, &decodedWelcome),
           "nonzero welcome reserved byte is rejected");

    disconnect.sessionId = 0x11223344;
    disconnect.reason = MULTI_NET_DISCONNECT_USER;
    disconnect.senderSlot = 1;
    Expect(EncodeMultiNetDisconnectPacket(disconnect, wire, sizeof(wire), &wireSize),
           "valid disconnect encodes");
    Expect(DecodeMultiNetDisconnectPacket(wire, wireSize, &decodedDisconnect),
           "valid disconnect decodes");
    wire[19] = 1;
    Expect(!DecodeMultiNetDisconnectPacket(wire, wireSize, &decodedDisconnect),
           "nonzero disconnect reserved byte is rejected");
    disconnect.reason = 0xFFFF;
    Expect(!EncodeMultiNetDisconnectPacket(disconnect, wire, sizeof(wire), &wireSize),
           "unknown disconnect reason is rejected");
}

unsigned long NextMutation(unsigned long *state)
{
    unsigned long value = *state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    return value;
}

void ExerciseAllDecoders(const u8 *data, u32 size)
{
    MultiNetPacketType type;
    MultiNetHelloPacket hello;
    MultiNetWelcomePacket welcome;
    MultiNetInputPacket input;
    MultiNetDisconnectPacket disconnect;

    GetMultiNetPacketType(data, size, &type);
    DecodeMultiNetHelloPacket(data, size, &hello);
    DecodeMultiNetWelcomePacket(data, size, &welcome);
    DecodeMultiNetInputPacket(data, size, &input);
    DecodeMultiNetDisconnectPacket(data, size, &disconnect);
}

void TestDeterministicMalformedCorpus()
{
    u8 data[256];
    unsigned long state = 0x54483038UL;
    u32 size;
    u32 iteration;
    u32 index;

    ExerciseAllDecoders(NULL, 0);
    for (size = 0; size <= sizeof(data); ++size)
    {
        for (iteration = 0; iteration < 32; ++iteration)
        {
            for (index = 0; index < size; ++index)
                data[index] = static_cast<u8>(NextMutation(&state));
            ExerciseAllDecoders(data, size);
        }
    }
    Expect(true, "deterministic malformed corpus completed without a crash");
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
    TestHandshakeAndDisconnectGuards();
    TestHistoryAndRedundancy();
    TestDroppedPacketRecovery();
    TestDeterministicMalformedCorpus();

    if (failures != 0)
    {
        printf("%d multiplayer network test(s) failed\n", failures);
        return 1;
    }
    printf("multiplayer network tests passed\n");
    return 0;
}
