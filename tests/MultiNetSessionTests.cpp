#include <winsock.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "MultiNetSession.hpp"

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

class TestPeer
{
  public:
    SOCKET socketHandle;
    u16 port;
    bool winsockStarted;

    TestPeer() : socketHandle(INVALID_SOCKET), port(0), winsockStarted(false)
    {
        WSADATA data;
        sockaddr_in address;
        int addressSize = sizeof(address);
        u_long nonBlocking = 1;
        if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
            return;
        winsockStarted = true;
        socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socketHandle == INVALID_SOCKET)
            return;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr("127.0.0.1");
        if (bind(socketHandle, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0 ||
            getsockname(socketHandle, reinterpret_cast<sockaddr *>(&address), &addressSize) != 0 ||
            ioctlsocket(socketHandle, FIONBIO, &nonBlocking) != 0)
            return;
        port = ntohs(address.sin_port);
    }

    ~TestPeer()
    {
        if (socketHandle != INVALID_SOCKET)
            closesocket(socketHandle);
        if (winsockStarted)
            WSACleanup();
    }

    bool Send(const u8 *wire, u32 size, u16 destinationPort)
    {
        sockaddr_in destination;
        memset(&destination, 0, sizeof(destination));
        destination.sin_family = AF_INET;
        destination.sin_addr.s_addr = inet_addr("127.0.0.1");
        destination.sin_port = htons(destinationPort);
        return sendto(socketHandle, reinterpret_cast<const char *>(wire), size, 0,
                      reinterpret_cast<const sockaddr *>(&destination), sizeof(destination)) ==
               static_cast<int>(size);
    }

    bool Receive(MultiNetPacketType wantedType, u8 *wire, u32 *size, u16 *senderPort)
    {
        for (i32 attempt = 0; attempt < 100; ++attempt)
        {
            sockaddr_in sender;
            int senderSize = sizeof(sender);
            int received = recvfrom(socketHandle, reinterpret_cast<char *>(wire), 512, 0,
                                    reinterpret_cast<sockaddr *>(&sender), &senderSize);
            if (received != SOCKET_ERROR)
            {
                MultiNetPacketType type;
                if (GetMultiNetPacketType(wire, received, &type) && type == wantedType)
                {
                    *size = received;
                    *senderPort = ntohs(sender.sin_port);
                    return true;
                }
            }
            else if (WSAGetLastError() != WSAEWOULDBLOCK)
                return false;
            Sleep(1);
        }
        return false;
    }
};

u16 OpenTestHost(MultiNetSession *host, u8 slowMode)
{
    MultiNetWelcomePacket::SaveProgress progress[MULTI_NET_SAVE_SHOT_COUNT] = { 0 };
    u16 firstPort = static_cast<u16>(24000 + GetCurrentProcessId() % 10000);
    for (u16 offset = 0; offset < 64; ++offset)
    {
        u16 port = firstPort + offset;
        if (host->OpenHost(port, "127.0.0.1", 0, 3, 0x40003, 0x1111, 0x2222,
                           2, slowMode, progress))
            return port;
    }
    return 0;
}

MultiNetInputPacket InputPacket(u32 sessionId, u8 slot, u32 revision, u32 frame)
{
    MultiNetInputPacket input = { 0 };
    input.sessionId = sessionId;
    input.senderSlot = slot;
    input.settingsRevision = revision;
    input.latestFrame = frame;
    input.stateHashFrame = MULTI_NET_INVALID_FRAME;
    input.sampleCount = 1;
    input.samples[0].frame = frame;
    return input;
}

bool SendInput(TestPeer *peer, u16 destinationPort, const MultiNetInputPacket &input)
{
    u8 wire[512];
    u32 size;
    return EncodeMultiNetInputPacket(input, wire, sizeof(wire), &size) &&
           peer->Send(wire, size, destinationPort);
}

void TestBothHostSlowModes()
{
    for (u8 slowMode = 0; slowMode <= 1; ++slowMode)
    {
        MultiNetSession host;
        MultiNetSession guest;
        u16 port = OpenTestHost(&host, slowMode);
        Expect(port != 0, "loopback host opens");
        if (port == 0)
            return;
        Expect(guest.OpenGuest(0, "127.0.0.1", "127.0.0.1", port,
                               0, 3, 0x40003, 0x3333), "loopback guest opens");
        for (u32 tick = 1000; tick < 1100 && guest.GetState() != MULTI_NET_STATE_CONNECTED; ++tick)
        {
            host.Pump(tick);
            guest.Pump(tick);
            Sleep(1);
        }
        Expect(guest.GetState() == MULTI_NET_STATE_CONNECTED, "loopback guest connects");
        Expect(guest.GetSlowMode() == slowMode, "guest uses host slow mode in both directions");
        Expect(!host.AreGameplaySettingsConfirmed() && !guest.AreGameplaySettingsConfirmed(),
               "welcome alone does not satisfy the bilateral settings barrier");
        Expect(host.CaptureLocalInput(0, 0, MULTI_NET_INVALID_FRAME, 0), "host sends settings revision");
        Expect(guest.CaptureLocalInput(0, 0, MULTI_NET_INVALID_FRAME, 0), "guest acknowledges revision");
        host.Pump(1100);
        guest.Pump(1100);
        Expect(host.AreGameplaySettingsConfirmed() && guest.AreGameplaySettingsConfirmed(),
               "both peers confirm the same settings revision");
        u32 oldRevision = host.GetSaveRevision();
        host.UpdateHostGameplaySettings(2, 1 - slowMode);
        Expect(host.GetSaveRevision() == oldRevision + 1 && !host.AreGameplaySettingsConfirmed(),
               "changing only slow mode invalidates the previous confirmation");
        guest.Pump(1110);
        Expect(guest.GetSlowMode() == 1 - slowMode && !guest.AreGameplaySettingsConfirmed(),
               "guest applies the new host setting before confirming it");
        host.CaptureLocalInput(1, 0, MULTI_NET_INVALID_FRAME, 0);
        guest.CaptureLocalInput(1, 0, MULTI_NET_INVALID_FRAME, 0);
        host.Pump(1120);
        guest.Pump(1120);
        Expect(host.AreGameplaySettingsConfirmed() && guest.AreGameplaySettingsConfirmed(),
               "changed setting is confirmed by both peers");
        oldRevision = host.GetSaveRevision();
        host.UpdateHostGameplaySettings(2, 2);
        Expect(host.GetSaveRevision() == oldRevision && host.GetSlowMode() == 1 - slowMode,
               "invalid slow-mode update does not mutate the session");
        guest.UpdateHostGameplaySettings(0, slowMode);
        Expect(guest.GetSaveRevision() == oldRevision && guest.GetSlowMode() == 1 - slowMode,
               "guest cannot replace authoritative host settings");
    }
}

void TestLostWelcomeIsResent()
{
    TestPeer peer;
    MultiNetSession host;
    u16 hostPort = OpenTestHost(&host, 0);
    u8 wire[512];
    u32 size;
    u16 senderPort;
    MultiNetHelloPacket hello = { 0 };
    MultiNetWelcomePacket welcome = { 0 };
    Expect(peer.port != 0 && hostPort != 0, "resend test binds loopback sockets");
    if (peer.port == 0 || hostPort == 0)
        return;
    hello.buildFingerprint = 0x40003;
    hello.clientNonce = 0x3333;
    hello.requestedInputDelay = 3;
    Expect(EncodeMultiNetHelloPacket(hello, wire, sizeof(wire), &size) &&
           peer.Send(wire, size, hostPort), "test peer sends hello");
    host.Pump(1000);
    if (!peer.Receive(MULTI_NET_PACKET_WELCOME, wire, &size, &senderPort))
    {
        Expect(false, "test peer receives initial welcome");
        return;
    }
    DecodeMultiNetWelcomePacket(wire, size, &welcome);
    Expect(SendInput(&peer, hostPort, InputPacket(welcome.sessionId, 1, welcome.saveRevision, 0)),
           "test peer acknowledges initial settings");
    host.Pump(1010);
    Expect(host.AreGameplaySettingsConfirmed(), "initial settings are acknowledged");
    host.UpdateHostGameplaySettings(2, 1);
    Expect(peer.Receive(MULTI_NET_PACKET_WELCOME, wire, &size, &senderPort),
           "first settings update is deliberately discarded");
    host.Pump(1261);
    Expect(peer.Receive(MULTI_NET_PACKET_WELCOME, wire, &size, &senderPort) &&
           DecodeMultiNetWelcomePacket(wire, size, &welcome) && welcome.slowMode == 1 &&
           welcome.saveRevision == 2, "unacknowledged settings are retransmitted after packet loss");
    Expect(!host.AreGameplaySettingsConfirmed(), "resending alone cannot acknowledge settings");
    SendInput(&peer, hostPort, InputPacket(host.GetSessionId(), 1, 2, 1));
    host.Pump(1270);
    Expect(host.AreGameplaySettingsConfirmed(), "current revision acknowledges retransmitted settings");
    SendInput(&peer, hostPort, InputPacket(host.GetSessionId(), 1, 1, 0));
    host.Pump(1280);
    Expect(host.AreGameplaySettingsConfirmed(), "old reordered input cannot roll back confirmation");
}

void TestGuestRejectsStaleOrConflictingSnapshots()
{
    TestPeer peer;
    MultiNetSession guest;
    u8 wire[512];
    u32 size;
    u16 guestPort;
    MultiNetHelloPacket hello = { 0 };
    MultiNetWelcomePacket welcome = { 0 };
    Expect(peer.port != 0 && guest.OpenGuest(0, "127.0.0.1", "127.0.0.1", peer.port,
                                           0, 3, 0x40003, 0x3333), "snapshot test guest opens");
    if (!peer.Receive(MULTI_NET_PACKET_HELLO, wire, &size, &guestPort))
    {
        Expect(false, "snapshot test receives guest hello");
        return;
    }
    DecodeMultiNetHelloPacket(wire, size, &hello);
    welcome.sessionId = 0x11223344;
    welcome.hostNonce = 0x1111;
    welcome.echoedClientNonce = hello.clientNonce;
    welcome.randomSeed = 0x2222;
    welcome.inputDelay = 3;
    welcome.assignedSlot = 1;
    welcome.initialLives = 5;
    welcome.slowMode = 1;
    welcome.saveRevision = 2;
    EncodeMultiNetWelcomePacket(welcome, wire, sizeof(wire), &size);
    peer.Send(wire, size, guestPort);
    guest.Pump(1000);
    Expect(guest.GetState() == MULTI_NET_STATE_CONNECTED && guest.GetSlowMode() == 1,
           "guest accepts authoritative initial snapshot");
    SendInput(&peer, guestPort, InputPacket(welcome.sessionId, 0, 2, 0));
    guest.Pump(1010);
    Expect(guest.AreGameplaySettingsConfirmed(), "guest confirms current host revision");
    welcome.saveRevision = 1;
    welcome.initialLives = 1;
    welcome.slowMode = 0;
    EncodeMultiNetWelcomePacket(welcome, wire, sizeof(wire), &size);
    peer.Send(wire, size, guestPort);
    guest.Pump(1020);
    Expect(guest.GetSaveRevision() == 2 && guest.GetInitialLives() == 5 && guest.GetSlowMode() == 1,
           "old welcome cannot roll back lives or slow mode");
    welcome.saveRevision = 2;
    EncodeMultiNetWelcomePacket(welcome, wire, sizeof(wire), &size);
    peer.Send(wire, size, guestPort);
    guest.Pump(1030);
    Expect(guest.GetInitialLives() == 5 && guest.GetSlowMode() == 1,
           "same-revision conflicting settings are rejected");
    welcome.initialLives = 5;
    welcome.slowMode = 1;
    EncodeMultiNetWelcomePacket(welcome, wire, sizeof(wire), &size);
    peer.Send(wire, size, guestPort);
    guest.Pump(5000);
    guest.Pump(6500);
    Expect(guest.GetState() == MULTI_NET_STATE_CONNECTED,
           "identical retransmitted snapshot refreshes the receive timer");
    welcome.saveRevision = 3;
    welcome.slowMode = 0;
    EncodeMultiNetWelcomePacket(welcome, wire, sizeof(wire), &size);
    peer.Send(wire, size, guestPort);
    guest.Pump(6510);
    Expect(guest.GetSlowMode() == 0 && !guest.AreGameplaySettingsConfirmed(),
           "new snapshot is applied but waits for the new revision confirmation");
}
} // namespace

int main()
{
    TestBothHostSlowModes();
    TestLostWelcomeIsResent();
    TestGuestRejectsStaleOrConflictingSnapshots();
    if (failures == 0)
        printf("multiplayer session settings tests passed\n");
    return failures != 0 ? 1 : 0;
}
