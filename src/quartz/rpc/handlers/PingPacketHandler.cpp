#include "quartz/rpc/handlers/PingPacketHandler.hpp"

#include "quartz/rpc/senders/PongPacketSender.hpp"

namespace quartz::rpc::handlers
{
    bool PingPacketHandler::_validate(const PacketHeader& header)
    {
        return header.PayloadLength == 0;
    }

    void PingPacketHandler::handle(const PacketHeader& header)
    {
        if (!_validate(header))
            return;
        senders::PongPacketSender::send(header.PacketId);
    }
}