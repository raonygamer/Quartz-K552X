#pragma once
#include <cstdint>

namespace quartz::usb::proto {
    struct InTransferState {
        std::span<const std::byte> Data = {};
        std::size_t Offset = 0;
        std::size_t InFlight = 0;
        bool Active = false;
        bool ShouldSendZeroLength = false;

        void reset() noexcept
        {
            Data = {};
            Offset = 0;
            InFlight = 0;
            Active = false;
            ShouldSendZeroLength = false;
        }
    };
}
