#pragma once
#include <cstdint>

namespace quartz::rpc::senders
{
    class PongPacketSender
    {
    public:
        static void send(std::uint32_t responseFor);
    };
}