#pragma once
#include "quartz/rpc/PacketHeader.hpp"

namespace quartz::rpc::handlers
{
    class PingPacketHandler
    {
        static bool _validate(const PacketHeader& header);
    public:
        static void handle(const PacketHeader& header);
    };
}