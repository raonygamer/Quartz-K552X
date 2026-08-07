#pragma once
#include <cstdint>
#include "PacketType.hpp"
#include "DebugMessagePacket.hpp"

namespace quartz::protocol::packets {
    std::size_t DebugMessagePacket::serialize(std::uint8_t* buffer) const {

    }

    void DebugMessagePacket::setMessage(const char* msg) {

    }

    const char* DebugMessagePacket::getMessage() const {

    }

    std::uint8_t DebugMessagePacket::getLength() const {

    }
}