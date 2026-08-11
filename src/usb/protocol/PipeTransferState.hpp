#pragma once
#include "hal/usb/Endpoint.hpp"
#include "usb/protocol/InTransferState.hpp"
#include "usb/protocol/OutTransferState.hpp"

namespace quartz::usb::proto
{
    constexpr std::size_t NON_CONTROL_ENDPOINT_COUNT = hal::usb::Endpoint::MAX_ENDPOINTS - 1;

    struct PipeTransferState
    {
        std::array<InTransferState, NON_CONTROL_ENDPOINT_COUNT> In = {};
        std::array<OutTransferState, NON_CONTROL_ENDPOINT_COUNT> Out = {};

        void reset() noexcept
        {
            for (std::size_t i = 0; i < NON_CONTROL_ENDPOINT_COUNT; ++i)
            {
                In[i].reset();
                Out[i].reset();
            }
        }
    };
}