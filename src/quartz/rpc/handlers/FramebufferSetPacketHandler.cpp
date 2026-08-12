#include "quartz/rpc/handlers/FramebufferSetPacketHandler.hpp"

#include "kb/rgb/RGBMatrix.hpp"

namespace quartz::rpc::handlers
{
    using MatrixFramebufferSetPayload = payloads::FramebufferSetPayload<kb::MatrixDefinitions::Size>;
    bool FramebufferSetPacketHandler::_validate(const PacketHeader& header)
    {
        return header.PayloadLength == sizeof(MatrixFramebufferSetPayload);
    }

    void FramebufferSetPacketHandler::handle(const PacketHeader& header)
    {
        if (!_validate(header))
            return;
        // 101% Safe cast, trust me
        const auto& payload = *header.getPayload<MatrixFramebufferSetPayload>();
        if (payload.MatrixSize != kb::MatrixDefinitions::Size)
            return;
        kb::rgb::RGBMatrix::setFramebuffer(payload.Framebuffer.data());
        kb::rgb::RGBMatrix::swapBuffers();
    }
}