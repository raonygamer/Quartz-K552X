#include "quartz/rpc/handlers/PerformanceRequestPacketHandler.hpp"
#include "quartz/rpc/senders/PerformanceResponsePacketSender.hpp"

namespace quartz::rpc::handlers
{
    bool PerformanceRequestPacketHandler::_validate(const PacketHeader& header)
    {
        return header.PayloadLength == 0;
    }

    void PerformanceRequestPacketHandler::handle(const PacketHeader& header)
    {
        if (!_validate(header))
            return;
        senders::PerformanceResponsePacketSender::send(header.PacketId);
    }
}