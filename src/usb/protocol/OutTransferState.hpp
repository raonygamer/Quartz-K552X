#pragma once
#include <cstdint>

namespace quartz::usb::proto
{
    struct OutTransferState
    {
        std::span<std::byte> Data = {};
        std::size_t Offset = 0;
        std::size_t ExpectedLength = 0;
        bool Active = false;

        void reset() noexcept
        {
            Data = {};
            Offset = 0;
            ExpectedLength = 0;
            Active = false;
        }
    };
}