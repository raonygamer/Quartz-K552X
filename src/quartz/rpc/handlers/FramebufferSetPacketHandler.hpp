#pragma once
#include "kb/MatrixDefinitions.hpp"
#include "quartz/rpc/PacketHeader.hpp"
#include "quartz/rpc/payloads/FramebufferSetPayload.hpp"

namespace quartz::rpc::handlers
{
    class FramebufferSetPacketHandler
    {
        static bool _validate(const PacketHeader& header);
    public:
        static void handle(const PacketHeader& header);
    };
}