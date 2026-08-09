#pragma once
#include <array>
#include <cstdint>
#include "kb/MatrixDefinitions.hpp"
#include "utils/Color32.hpp"

namespace quartz::rpc {
    struct [[gnu::packed]] SetRGBMatrixPacket
    {
        std::array<utils::Color32, kb::MatrixDefinitions::Size> Colors;
    };
}