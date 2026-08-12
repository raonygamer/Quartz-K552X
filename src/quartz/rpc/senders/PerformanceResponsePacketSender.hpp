#pragma once
#include <cstdint>

namespace quartz::rpc::senders
{
    class PerformanceResponsePacketSender
    {
    public:
        static void send(std::uint32_t responseFor);
    };
}