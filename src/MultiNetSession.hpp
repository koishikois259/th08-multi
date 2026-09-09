#pragma once

#include <stddef.h>

#include "MultiNetProtocol.hpp"

namespace th08
{

enum MultiNetRole
{
    MULTI_NET_ROLE_NONE = 0,
    MULTI_NET_ROLE_HOST = 1,
    MULTI_NET_ROLE_GUEST = 2,
};

enum MultiNetSessionState
{
    MULTI_NET_STATE_CLOSED = 0,
    MULTI_NET_STATE_LISTENING = 1,
    MULTI_NET_STATE_CONNECTING = 2,
    MULTI_NET_STATE_CONNECTED = 3,
    MULTI_NET_STATE_DISCONNECTED = 4,
    MULTI_NET_STATE_ERROR = 5,
};

enum MultiNetSessionError
{
    MULTI_NET_ERROR_NONE = 0,
    MULTI_NET_ERROR_SOCKET = 1,
    MULTI_NET_ERROR_ADDRESS = 2,
    MULTI_NET_ERROR_BUILD_MISMATCH = 3,
    MULTI_NET_ERROR_PROTOCOL = 4,
    MULTI_NET_ERROR_TIMEOUT = 5,
    MULTI_NET_ERROR_REMOTE_CLOSED = 6,
    MULTI_NET_ERROR_DESYNC = 7,
};

class MultiNetSession
{
  public:
    MultiNetSession();
    ~MultiNetSession();

    bool OpenHost(u16 localPort, u8 selectedTeam, u16 requestedInputDelay,
                  u32 buildFingerprint, u32 hostNonce, u32 randomSeed);
    bool OpenGuest(u16 localPort, const char *hostAddress, u16 hostPort,
                   u8 selectedTeam, u16 requestedInputDelay,
                   u32 buildFingerprint, u32 clientNonce);
    void Close(u16 reason);
    void Pump(u32 nowMilliseconds);

    bool CaptureLocalInput(u32 simulationFrame, u16 buttons,
                           u32 stateHashFrame, u32 stateHash);
    bool TryGetFrameInputs(u32 simulationFrame, u16 *p1Buttons, u16 *p2Buttons) const;

    MultiNetSessionState GetState() const;
    MultiNetSessionError GetError() const;
    MultiNetRole GetRole() const;
    u8 GetLocalSlot() const;
    u8 GetHostTeam() const;
    u8 GetGuestTeam() const;
    u16 GetInputDelay() const;
    u32 GetSessionId() const;
    u32 GetRandomSeed() const;
    u32 GetLatestRemoteFrame() const;
    u32 GetLatestAcknowledgedFrame() const;
    u32 GetDesyncFrame() const;

  private:
    struct HashEntry
    {
        u32 frame;
        u32 hash;
        bool occupied;
    };

    void ResetState();
    bool OpenSocket(u16 localPort);
    void SendHello();
    void SendWelcome();
    void SendInput(u32 latestFrame, u32 stateHashFrame, u32 stateHash);
    void SendDisconnect(u16 reason);
    void ReceivePackets(u32 nowMilliseconds);
    void HandleHello(const u8 *data, u32 size, u32 address, u16 port, u32 nowMilliseconds);
    void HandleWelcome(const u8 *data, u32 size, u32 address, u16 port, u32 nowMilliseconds);
    void HandleInput(const u8 *data, u32 size, u32 address, u16 port, u32 nowMilliseconds);
    void HandleDisconnect(const u8 *data, u32 size, u32 address, u16 port);
    bool IsRemoteEndpoint(u32 address, u16 port) const;
    void StoreLocalHash(u32 frame, u32 hash);
    void CompareRemoteHash(u32 frame, u32 hash);

    size_t socketHandle;
    u32 remoteAddress;
    u16 remotePort;
    MultiNetRole role;
    MultiNetSessionState state;
    MultiNetSessionError error;
    u32 buildFingerprint;
    u32 localNonce;
    u32 remoteNonce;
    u32 sessionId;
    u32 randomSeed;
    u16 requestedInputDelay;
    u16 inputDelay;
    u8 localTeam;
    u8 hostTeam;
    u8 guestTeam;
    u8 localSlot;
    u32 lastSendTime;
    u32 lastReceiveTime;
    u32 currentTime;
    u32 latestLocalFrame;
    u32 latestRemoteFrame;
    u32 latestAcknowledgedFrame;
    u32 desyncFrame;
    u32 latestStateHashFrame;
    u32 latestStateHash;
    u32 pendingRemoteHashFrame;
    u32 pendingRemoteHash;
    bool peerConfirmed;
    MultiNetInputHistory localInputs;
    MultiNetInputHistory remoteInputs;
    HashEntry localHashes[MULTI_NET_INPUT_HISTORY_SIZE];
};

} // namespace th08
