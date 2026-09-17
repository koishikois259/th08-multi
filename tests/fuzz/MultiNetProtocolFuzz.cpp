#include "MultiNetProtocol.hpp"

#include <stddef.h>
#include <stdint.h>

using namespace th08;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    MultiNetPacketType type;
    MultiNetHelloPacket hello;
    MultiNetWelcomePacket welcome;
    MultiNetInputPacket input;
    MultiNetDisconnectPacket disconnect;
    u8 encoded[512];
    u32 encodedSize;

    if (size > 65535)
        return 0;
    GetMultiNetPacketType(data, static_cast<u32>(size), &type);
    if (DecodeMultiNetHelloPacket(data, static_cast<u32>(size), &hello))
        EncodeMultiNetHelloPacket(hello, encoded, sizeof(encoded), &encodedSize);
    if (DecodeMultiNetWelcomePacket(data, static_cast<u32>(size), &welcome))
        EncodeMultiNetWelcomePacket(welcome, encoded, sizeof(encoded), &encodedSize);
    if (DecodeMultiNetInputPacket(data, static_cast<u32>(size), &input))
        EncodeMultiNetInputPacket(input, encoded, sizeof(encoded), &encodedSize);
    if (DecodeMultiNetDisconnectPacket(data, static_cast<u32>(size), &disconnect))
        EncodeMultiNetDisconnectPacket(disconnect, encoded, sizeof(encoded), &encodedSize);
    return 0;
}
