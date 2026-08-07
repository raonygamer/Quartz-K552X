#pragma once
#include <cstdint>
#include <array>
#include "PacketType.hpp"

namespace quartz::protocol::packets {
    struct DebugMessagePacket {
        static constexpr PacketType Type = PacketType::DebugMessage;

        std::size_t serialize(std::uint8_t* buffer) const;
        const char* getMessage() const;
        void setMessage(const char* msg);
        std::uint8_t getLength() const;

    private:
        std::uint8_t mLength;
        std::array<char, 256> mMessage;
    };
}