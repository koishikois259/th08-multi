#include <winsock.h>
#include <string.h>

#include "th_pch.h"
#include "MultiNetSession.hpp"

namespace th08
{

namespace
{

const size_t MULTI_NET_INVALID_SOCKET_HANDLE = static_cast<size_t>(~static_cast<size_t>(0));
const u32 MULTI_NET_RESEND_INTERVAL_MS = 250;
const u32 MULTI_NET_INPUT_RESEND_INTERVAL_MS = 50;
const u32 MULTI_NET_CONNECT_TIMEOUT_MS = 30000;
const u32 MULTI_NET_CONNECTED_TIMEOUT_MS = 5000;
const u32 MULTI_NET_WIRE_BUFFER_SIZE = 256;
const u32 MULTI_NET_MAX_PACKETS_PER_PUMP = 64;

u16 ClampInputDelay(u16 delay)
{
    if (delay < MULTI_NET_MIN_INPUT_DELAY)
        return MULTI_NET_MIN_INPUT_DELAY;
    if (delay > MULTI_NET_MAX_INPUT_DELAY)
        return MULTI_NET_MAX_INPUT_DELAY;
    return delay;
}

u32 MakeSessionId(u32 hostNonce, u32 clientNonce, u32 randomSeed)
{
    u32 value = hostNonce ^ (clientNonce * 0x9E3779B9) ^ randomSeed ^ 0x54483038;
    if (value == 0)
        value = 1;
    return value;
}

} // namespace

MultiNetSession::MultiNetSession()
{
    socketHandle = MULTI_NET_INVALID_SOCKET_HANDLE;
    ResetState();
}

MultiNetSession::~MultiNetSession()
{
    Close(MULTI_NET_DISCONNECT_USER);
}

void MultiNetSession::ResetState()
{
    u32 i;
    remoteAddress = 0;
    remotePort = 0;
    role = MULTI_NET_ROLE_NONE;
    state = MULTI_NET_STATE_CLOSED;
    error = MULTI_NET_ERROR_NONE;
    buildFingerprint = 0;
    localNonce = 0;
    remoteNonce = 0;
    sessionId = 0;
    randomSeed = 0;
    requestedInputDelay = MULTI_NET_MIN_INPUT_DELAY;
    inputDelay = MULTI_NET_MIN_INPUT_DELAY;
    localTeam = 0;
    hostTeam = 0;
    guestTeam = 0;
    localSlot = 0;
    lastSendTime = 0;
    lastReceiveTime = 0;
    currentTime = 0;
    latestLocalFrame = MULTI_NET_INVALID_FRAME;
    latestRemoteFrame = MULTI_NET_INVALID_FRAME;
    latestAcknowledgedFrame = MULTI_NET_INVALID_FRAME;
    desyncFrame = MULTI_NET_INVALID_FRAME;
    latestStateHashFrame = MULTI_NET_INVALID_FRAME;
    latestStateHash = 0;
    pendingRemoteHashFrame = MULTI_NET_INVALID_FRAME;
    pendingRemoteHash = 0;
    peerConfirmed = false;
    localInputs.Reset();
    remoteInputs.Reset();
    for (i = 0; i < MULTI_NET_INPUT_HISTORY_SIZE; ++i)
    {
        localHashes[i].frame = 0;
        localHashes[i].hash = 0;
        localHashes[i].occupied = false;
    }
}

bool MultiNetSession::OpenSocket(u16 localPort, u32 bindAddress)
{
    WSADATA data;
    SOCKET sock;
    sockaddr_in local;
    u_long nonBlocking = 1;

    if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
        return false;
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET)
    {
        WSACleanup();
        return false;
    }
    local.sin_family = AF_INET;
    local.sin_port = htons(localPort);
    local.sin_addr.s_addr = bindAddress;
    memset(local.sin_zero, 0, sizeof(local.sin_zero));
    if (bind(sock, reinterpret_cast<const sockaddr *>(&local), sizeof(local)) == SOCKET_ERROR ||
        ioctlsocket(sock, FIONBIO, &nonBlocking) == SOCKET_ERROR)
    {
        closesocket(sock);
        WSACleanup();
        return false;
    }
    socketHandle = static_cast<size_t>(sock);
    return true;
}

bool MultiNetSession::OpenHost(u16 localPort, const char *bindAddress,
                               u8 selectedTeam, u16 delay,
                               u32 fingerprint, u32 hostNonce, u32 seed)
{
    u32 localAddress;
    Close(MULTI_NET_DISCONNECT_USER);
    ResetState();
    if (hostNonce == 0 || seed == 0)
    {
        state = MULTI_NET_STATE_ERROR;
        error = MULTI_NET_ERROR_ENTROPY;
        return false;
    }
    localAddress = bindAddress != NULL ? inet_addr(bindAddress) : INADDR_NONE;
    if (localAddress == INADDR_NONE)
    {
        state = MULTI_NET_STATE_ERROR;
        error = MULTI_NET_ERROR_ADDRESS;
        return false;
    }
    if (selectedTeam >= 4 || !OpenSocket(localPort, localAddress))
    {
        state = MULTI_NET_STATE_ERROR;
        error = selectedTeam >= 4 ? MULTI_NET_ERROR_PROTOCOL : MULTI_NET_ERROR_SOCKET;
        return false;
    }
    role = MULTI_NET_ROLE_HOST;
    state = MULTI_NET_STATE_LISTENING;
    buildFingerprint = fingerprint;
    localNonce = hostNonce;
    randomSeed = seed;
    requestedInputDelay = ClampInputDelay(delay);
    inputDelay = requestedInputDelay;
    localTeam = selectedTeam;
    hostTeam = selectedTeam;
    localSlot = 0;
    return true;
}

bool MultiNetSession::OpenGuest(u16 localPort, const char *bindAddress,
                                const char *hostAddress, u16 hostPort,
                                u8 selectedTeam, u16 delay, u32 fingerprint, u32 clientNonce)
{
    u32 address;
    u32 localAddress;
    Close(MULTI_NET_DISCONNECT_USER);
    ResetState();
    if (clientNonce == 0)
    {
        state = MULTI_NET_STATE_ERROR;
        error = MULTI_NET_ERROR_ENTROPY;
        return false;
    }
    if (hostAddress == NULL || bindAddress == NULL || selectedTeam >= 4)
    {
        state = MULTI_NET_STATE_ERROR;
        error = MULTI_NET_ERROR_ADDRESS;
        return false;
    }
    address = inet_addr(hostAddress);
    localAddress = inet_addr(bindAddress);
    if (address == INADDR_NONE || localAddress == INADDR_NONE)
    {
        state = MULTI_NET_STATE_ERROR;
        error = MULTI_NET_ERROR_ADDRESS;
        return false;
    }
    if (!OpenSocket(localPort, localAddress))
    {
        state = MULTI_NET_STATE_ERROR;
        error = MULTI_NET_ERROR_SOCKET;
        return false;
    }
    role = MULTI_NET_ROLE_GUEST;
    state = MULTI_NET_STATE_CONNECTING;
    remoteAddress = address;
    remotePort = htons(hostPort);
    buildFingerprint = fingerprint;
    localNonce = clientNonce;
    requestedInputDelay = ClampInputDelay(delay);
    inputDelay = requestedInputDelay;
    localTeam = selectedTeam;
    guestTeam = selectedTeam;
    localSlot = 1;
    SendHello();
    return true;
}

void MultiNetSession::Close(u16 reason)
{
    if (socketHandle != MULTI_NET_INVALID_SOCKET_HANDLE)
    {
        if (state == MULTI_NET_STATE_CONNECTED)
            SendDisconnect(reason);
        closesocket(static_cast<SOCKET>(socketHandle));
        WSACleanup();
        socketHandle = MULTI_NET_INVALID_SOCKET_HANDLE;
    }
    if (state != MULTI_NET_STATE_ERROR)
        state = MULTI_NET_STATE_CLOSED;
}

void MultiNetSession::SendHello()
{
    MultiNetHelloPacket packet;
    u8 data[MULTI_NET_WIRE_BUFFER_SIZE];
    u32 size;
    sockaddr_in remote;
    packet.buildFingerprint = buildFingerprint;
    packet.clientNonce = localNonce;
    packet.capabilities = 0;
    packet.requestedInputDelay = requestedInputDelay;
    packet.selectedTeam = localTeam;
    if (!EncodeMultiNetHelloPacket(packet, data, sizeof(data), &size))
        return;
    remote.sin_family = AF_INET;
    remote.sin_port = remotePort;
    remote.sin_addr.s_addr = remoteAddress;
    memset(remote.sin_zero, 0, sizeof(remote.sin_zero));
    if (sendto(static_cast<SOCKET>(socketHandle), reinterpret_cast<const char *>(data), size, 0,
               reinterpret_cast<const sockaddr *>(&remote), sizeof(remote)) == SOCKET_ERROR)
    {
        error = MULTI_NET_ERROR_SOCKET;
        state = MULTI_NET_STATE_ERROR;
    }
}

void MultiNetSession::SendWelcome()
{
    MultiNetWelcomePacket packet;
    u8 data[MULTI_NET_WIRE_BUFFER_SIZE];
    u32 size;
    sockaddr_in remote;
    packet.sessionId = sessionId;
    packet.hostNonce = localNonce;
    packet.echoedClientNonce = remoteNonce;
    packet.randomSeed = randomSeed;
    packet.inputDelay = inputDelay;
    packet.hostTeam = hostTeam;
    packet.guestTeam = guestTeam;
    packet.assignedSlot = 1;
    if (!EncodeMultiNetWelcomePacket(packet, data, sizeof(data), &size))
        return;
    remote.sin_family = AF_INET;
    remote.sin_port = remotePort;
    remote.sin_addr.s_addr = remoteAddress;
    memset(remote.sin_zero, 0, sizeof(remote.sin_zero));
    if (sendto(static_cast<SOCKET>(socketHandle), reinterpret_cast<const char *>(data), size, 0,
               reinterpret_cast<const sockaddr *>(&remote), sizeof(remote)) == SOCKET_ERROR)
    {
        error = MULTI_NET_ERROR_SOCKET;
        state = MULTI_NET_STATE_ERROR;
    }
}

void MultiNetSession::SendInput(u32 latestFrame, u32 hashFrame, u32 hash)
{
    MultiNetInputPacket packet;
    u8 data[MULTI_NET_WIRE_BUFFER_SIZE];
    u32 size;
    sockaddr_in remote;
    packet.sessionId = sessionId;
    packet.latestFrame = latestFrame;
    packet.acknowledgedFrame = latestRemoteFrame;
    packet.stateHashFrame = hashFrame;
    packet.stateHash = hash;
    packet.senderSlot = localSlot;
    packet.sampleCount = static_cast<u8>(localInputs.BuildRedundantSamples(latestFrame, packet.samples));
    if (!EncodeMultiNetInputPacket(packet, data, sizeof(data), &size))
        return;
    remote.sin_family = AF_INET;
    remote.sin_port = remotePort;
    remote.sin_addr.s_addr = remoteAddress;
    memset(remote.sin_zero, 0, sizeof(remote.sin_zero));
    if (sendto(static_cast<SOCKET>(socketHandle), reinterpret_cast<const char *>(data), size, 0,
               reinterpret_cast<const sockaddr *>(&remote), sizeof(remote)) == SOCKET_ERROR)
    {
        error = MULTI_NET_ERROR_SOCKET;
        state = MULTI_NET_STATE_ERROR;
    }
}

void MultiNetSession::SendDisconnect(u16 reason)
{
    MultiNetDisconnectPacket packet;
    u8 data[MULTI_NET_WIRE_BUFFER_SIZE];
    u32 size;
    sockaddr_in remote;
    packet.sessionId = sessionId;
    packet.reason = reason;
    packet.senderSlot = localSlot;
    if (!EncodeMultiNetDisconnectPacket(packet, data, sizeof(data), &size))
        return;
    remote.sin_family = AF_INET;
    remote.sin_port = remotePort;
    remote.sin_addr.s_addr = remoteAddress;
    memset(remote.sin_zero, 0, sizeof(remote.sin_zero));
    if (sendto(static_cast<SOCKET>(socketHandle), reinterpret_cast<const char *>(data), size, 0,
               reinterpret_cast<const sockaddr *>(&remote), sizeof(remote)) == SOCKET_ERROR)
    {
        error = MULTI_NET_ERROR_SOCKET;
        state = MULTI_NET_STATE_ERROR;
    }
}

void MultiNetSession::Pump(u32 nowMilliseconds)
{
    if (socketHandle == MULTI_NET_INVALID_SOCKET_HANDLE)
        return;
    currentTime = nowMilliseconds;
    if (state == MULTI_NET_STATE_CONNECTING && lastReceiveTime == 0)
        lastReceiveTime = nowMilliseconds;
    ReceivePackets(nowMilliseconds);
    if (role == MULTI_NET_ROLE_GUEST && state == MULTI_NET_STATE_CONNECTING &&
        nowMilliseconds - lastSendTime >= MULTI_NET_RESEND_INTERVAL_MS)
    {
        SendHello();
        lastSendTime = nowMilliseconds;
    }
    else if (role == MULTI_NET_ROLE_HOST && state == MULTI_NET_STATE_CONNECTED && !peerConfirmed &&
             nowMilliseconds - lastSendTime >= MULTI_NET_RESEND_INTERVAL_MS)
    {
        SendWelcome();
        lastSendTime = nowMilliseconds;
    }
    else if (state == MULTI_NET_STATE_CONNECTED && latestLocalFrame != MULTI_NET_INVALID_FRAME &&
             nowMilliseconds - lastSendTime >= MULTI_NET_INPUT_RESEND_INTERVAL_MS)
    {
        SendInput(latestLocalFrame, latestStateHashFrame, latestStateHash);
        lastSendTime = nowMilliseconds;
    }
    if (state == MULTI_NET_STATE_CONNECTING && lastReceiveTime != 0 &&
        nowMilliseconds - lastReceiveTime > MULTI_NET_CONNECT_TIMEOUT_MS)
    {
        error = MULTI_NET_ERROR_TIMEOUT;
        state = MULTI_NET_STATE_DISCONNECTED;
    }
    else if (state == MULTI_NET_STATE_CONNECTED && lastReceiveTime != 0 &&
             nowMilliseconds - lastReceiveTime > MULTI_NET_CONNECTED_TIMEOUT_MS)
    {
        error = MULTI_NET_ERROR_TIMEOUT;
        state = MULTI_NET_STATE_DISCONNECTED;
    }
}

void MultiNetSession::ReceivePackets(u32 nowMilliseconds)
{
    u8 data[MULTI_NET_WIRE_BUFFER_SIZE];
    sockaddr_in sender;
    int senderSize;
    int received;
    MultiNetPacketType type;
    u32 packetCount;
    for (packetCount = 0; packetCount < MULTI_NET_MAX_PACKETS_PER_PUMP; ++packetCount)
    {
        senderSize = sizeof(sender);
        received = recvfrom(static_cast<SOCKET>(socketHandle), reinterpret_cast<char *>(data),
                            sizeof(data), 0, reinterpret_cast<sockaddr *>(&sender), &senderSize);
        if (received == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                error = MULTI_NET_ERROR_SOCKET;
                state = MULTI_NET_STATE_ERROR;
            }
            break;
        }
        if (!GetMultiNetPacketType(data, received, &type))
            continue;
        if (type == MULTI_NET_PACKET_HELLO)
            HandleHello(data, received, sender.sin_addr.s_addr, sender.sin_port, nowMilliseconds);
        else if (type == MULTI_NET_PACKET_WELCOME)
            HandleWelcome(data, received, sender.sin_addr.s_addr, sender.sin_port, nowMilliseconds);
        else if (type == MULTI_NET_PACKET_INPUT)
            HandleInput(data, received, sender.sin_addr.s_addr, sender.sin_port, nowMilliseconds);
        else if (type == MULTI_NET_PACKET_DISCONNECT)
            HandleDisconnect(data, received, sender.sin_addr.s_addr, sender.sin_port);
    }
}

void MultiNetSession::HandleHello(const u8 *data, u32 size, u32 address, u16 port, u32 now)
{
    MultiNetHelloPacket packet;
    if (role != MULTI_NET_ROLE_HOST || !DecodeMultiNetHelloPacket(data, size, &packet))
        return;
    if (state == MULTI_NET_STATE_CONNECTED && !IsRemoteEndpoint(address, port))
        return;
    if (packet.buildFingerprint != buildFingerprint)
    {
        remoteAddress = address;
        remotePort = port;
        error = MULTI_NET_ERROR_BUILD_MISMATCH;
        SendDisconnect(MULTI_NET_DISCONNECT_BUILD_MISMATCH);
        return;
    }
    remoteAddress = address;
    remotePort = port;
    remoteNonce = packet.clientNonce;
    guestTeam = packet.selectedTeam;
    inputDelay = requestedInputDelay > packet.requestedInputDelay
                     ? requestedInputDelay
                     : packet.requestedInputDelay;
    sessionId = MakeSessionId(localNonce, remoteNonce, randomSeed);
    state = MULTI_NET_STATE_CONNECTED;
    lastReceiveTime = now;
    SendWelcome();
    lastSendTime = now;
}

void MultiNetSession::HandleWelcome(const u8 *data, u32 size, u32 address, u16 port, u32 now)
{
    MultiNetWelcomePacket packet;
    if (role != MULTI_NET_ROLE_GUEST || !IsRemoteEndpoint(address, port) ||
        !DecodeMultiNetWelcomePacket(data, size, &packet) ||
        packet.echoedClientNonce != localNonce || packet.guestTeam != localTeam)
        return;
    remoteNonce = packet.hostNonce;
    sessionId = packet.sessionId;
    randomSeed = packet.randomSeed;
    inputDelay = packet.inputDelay;
    hostTeam = packet.hostTeam;
    guestTeam = packet.guestTeam;
    localSlot = packet.assignedSlot;
    state = MULTI_NET_STATE_CONNECTED;
    lastReceiveTime = now;
}

void MultiNetSession::HandleInput(const u8 *data, u32 size, u32 address, u16 port, u32 now)
{
    MultiNetInputPacket packet;
    u32 i;
    if (state != MULTI_NET_STATE_CONNECTED || !IsRemoteEndpoint(address, port) ||
        !DecodeMultiNetInputPacket(data, size, &packet) || packet.sessionId != sessionId ||
        packet.senderSlot == localSlot)
        return;
    for (i = 0; i < packet.sampleCount; ++i)
    {
        if (!remoteInputs.Insert(packet.samples[i].frame, packet.samples[i].buttons))
        {
            error = MULTI_NET_ERROR_PROTOCOL;
            state = MULTI_NET_STATE_ERROR;
            return;
        }
    }
    latestRemoteFrame = packet.latestFrame;
    latestAcknowledgedFrame = packet.acknowledgedFrame;
    CompareRemoteHash(packet.stateHashFrame, packet.stateHash);
    peerConfirmed = true;
    lastReceiveTime = now;
}

void MultiNetSession::HandleDisconnect(const u8 *data, u32 size, u32 address, u16 port)
{
    MultiNetDisconnectPacket packet;
    if (!IsRemoteEndpoint(address, port) || !DecodeMultiNetDisconnectPacket(data, size, &packet) ||
        packet.sessionId != sessionId || packet.senderSlot == localSlot)
        return;
    if (packet.reason == MULTI_NET_DISCONNECT_BUILD_MISMATCH)
        error = MULTI_NET_ERROR_BUILD_MISMATCH;
    else if (packet.reason == MULTI_NET_DISCONNECT_PROTOCOL)
        error = MULTI_NET_ERROR_PROTOCOL;
    else
        error = MULTI_NET_ERROR_REMOTE_CLOSED;
    state = MULTI_NET_STATE_DISCONNECTED;
}

bool MultiNetSession::IsRemoteEndpoint(u32 address, u16 port) const
{
    return address == remoteAddress && port == remotePort;
}

bool MultiNetSession::CaptureLocalInput(u32 simulationFrame, u16 buttons,
                                        u32 hashFrame, u32 hash)
{
    u32 scheduledFrame;
    if (state != MULTI_NET_STATE_CONNECTED)
        return false;
    scheduledFrame = simulationFrame + inputDelay;
    if (!localInputs.Insert(scheduledFrame, buttons))
        return false;
    latestLocalFrame = scheduledFrame;
    StoreLocalHash(hashFrame, hash);
    latestStateHashFrame = hashFrame;
    latestStateHash = hash;
    SendInput(latestLocalFrame, hashFrame, hash);
    lastSendTime = currentTime;
    return true;
}

bool MultiNetSession::TryGetFrameInputs(u32 frame, u16 *p1Buttons, u16 *p2Buttons) const
{
    u16 localButtons;
    u16 remoteButtons;
    if (p1Buttons == NULL || p2Buttons == NULL || state != MULTI_NET_STATE_CONNECTED)
        return false;
    if (frame < inputDelay)
    {
        localButtons = 0;
        remoteButtons = 0;
    }
    else if (!localInputs.TryGet(frame, &localButtons) || !remoteInputs.TryGet(frame, &remoteButtons))
        return false;
    if (localSlot == 0)
    {
        *p1Buttons = localButtons;
        *p2Buttons = remoteButtons;
    }
    else
    {
        *p1Buttons = remoteButtons;
        *p2Buttons = localButtons;
    }
    return true;
}

void MultiNetSession::StoreLocalHash(u32 frame, u32 hash)
{
    if (frame == MULTI_NET_INVALID_FRAME)
        return;
    HashEntry &entry = localHashes[frame % MULTI_NET_INPUT_HISTORY_SIZE];
    entry.frame = frame;
    entry.hash = hash;
    entry.occupied = true;
    if (pendingRemoteHashFrame == frame)
    {
        if (pendingRemoteHash != hash)
        {
            desyncFrame = frame;
            error = MULTI_NET_ERROR_DESYNC;
            state = MULTI_NET_STATE_ERROR;
        }
        pendingRemoteHashFrame = MULTI_NET_INVALID_FRAME;
        pendingRemoteHash = 0;
    }
}

void MultiNetSession::CompareRemoteHash(u32 frame, u32 hash)
{
    const HashEntry &entry = localHashes[frame % MULTI_NET_INPUT_HISTORY_SIZE];
    if (frame == MULTI_NET_INVALID_FRAME)
        return;
    if (!entry.occupied || entry.frame != frame)
    {
        pendingRemoteHashFrame = frame;
        pendingRemoteHash = hash;
        return;
    }
    if (entry.hash != hash)
    {
        desyncFrame = frame;
        error = MULTI_NET_ERROR_DESYNC;
        state = MULTI_NET_STATE_ERROR;
    }
}

MultiNetSessionState MultiNetSession::GetState() const { return state; }
MultiNetSessionError MultiNetSession::GetError() const { return error; }
MultiNetRole MultiNetSession::GetRole() const { return role; }
u8 MultiNetSession::GetLocalSlot() const { return localSlot; }
u8 MultiNetSession::GetHostTeam() const { return hostTeam; }
u8 MultiNetSession::GetGuestTeam() const { return guestTeam; }
u16 MultiNetSession::GetInputDelay() const { return inputDelay; }
u32 MultiNetSession::GetSessionId() const { return sessionId; }
u32 MultiNetSession::GetRandomSeed() const { return randomSeed; }
u32 MultiNetSession::GetLatestRemoteFrame() const { return latestRemoteFrame; }
u32 MultiNetSession::GetLatestAcknowledgedFrame() const { return latestAcknowledgedFrame; }
u32 MultiNetSession::GetDesyncFrame() const { return desyncFrame; }

} // namespace th08
