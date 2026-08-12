#include "quartz/rpc/RPC.hpp"
#include "quartz/rpc/senders/PongPacketSender.hpp"

namespace quartz::rpc::senders
{
    void PongPacketSender::send(const std::uint32_t responseFor)
    {
        RPC::send(PacketType::Pong, responseFor, {});
    }
}