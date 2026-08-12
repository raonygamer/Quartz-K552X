#include "RGBMatrix.hpp"
#include "cppmcu.h"
#include "hal/timer/HighResolutionTimer.hpp"
#include "kb/ElectricalMatrix.hpp"
#include "kb/Matrix.hpp"
#include "quartz/Profiling.hpp"

#include <cstring>

namespace quartz::kb::rgb
{
    void RGBMatrix::initialize() noexcept
    {
        SN_SYS1->AHBCLKEN |= 1u << 7;
        SN_CT16B1->PWMIOENB = 0u;
        SN_CT16B1->TMRCTRL = 0u;
        SN_CT16B1->PRE = PWMPrescaler;
        SN_CT16B1->MR24 = PWMPeriod;
        SN_CT16B1->PWMCTRL = 0x55555555u;
        SN_CT16B1->PWMCTRL2 = 0x55555555u;
        SN_CT16B1->PWMENB = PWMOutputMask;
        SN_CT16B1->MCTRL = 0u;
        SN_CT16B1->MCTRL2 = 0u;
        SN_CT16B1->MCTRL3 = (1u << 12) | (1u << 14);
        SN_CT16B1->TMRCTRL = 1u << 1;
        while (SN_CT16B1->TMRCTRL & (1u << 1)) __NOP();
        _configureSelectorPinsOutput();
        _deselectAllColumns();
        WritingBuffer = &BackBuffer;
        ReadingBuffer = &FrontBuffer;
        clear();
        preload();
        NVIC_ClearPendingIRQ(CT16B1_IRQn);
        NVIC_EnableIRQ(CT16B1_IRQn);
    }

    void RGBMatrix::clear() noexcept
    {
        fill(0u, 0u, 0u);
    }

    void RGBMatrix::fill(const utils::Color32 color) noexcept
    {
        for (std::size_t row = 0; row < MatrixDefinitions::Rows; ++row)
        {
            for (std::size_t column = 0; column < MatrixDefinitions::Cols; ++column)
            {
                (*WritingBuffer)[row][column] = color;
            }
        }
    }

    void RGBMatrix::fill(const std::uint8_t r, const std::uint8_t g, const std::uint8_t b) noexcept
    {
        fill(utils::Color32{ .R = r, .G = g, .B = b });
    }

    void RGBMatrix::setPixel(const std::uint8_t row, const std::uint8_t column, const utils::Color32 color) noexcept
    {
        if (row >= MatrixDefinitions::Rows || column >= MatrixDefinitions::Cols)
            return;
        (*WritingBuffer)[row][column] = color;
    }

    void RGBMatrix::setPixel(const std::uint8_t row, const std::uint8_t column, const std::uint8_t r, const std::uint8_t g, const std::uint8_t b) noexcept
    {
        setPixel(row, column, utils::Color32{ .R = r, .G = g, .B = b });
    }

    bool RGBMatrix::setFramebuffer(const SizedFlatColor32Matrix colors) noexcept
    {
        constexpr auto REQUIRED_FRAMEBUFFER_SIZE = MatrixDefinitions::Size * sizeof(utils::Color32);
        if (SwapPending)
            return false;
        if (!colors)
            return false;
        std::memcpy(WritingBuffer, colors, REQUIRED_FRAMEBUFFER_SIZE);
        return true;
    }

    bool RGBMatrix::swapBuffers() noexcept
    {
        if (SwapPending)
            return false;
        SwapPending = true;
        return true;
    }

    utils::Color32 RGBMatrix::getPixel(const std::uint8_t row, const std::uint8_t column) noexcept
    {
        if (row >= MatrixDefinitions::Rows || column >= MatrixDefinitions::Cols)
            return {};
        return (*ReadingBuffer)[row][column];
    }

    void RGBMatrix::preload() noexcept
    {
        const std::uint8_t column = CurrentColumn;
        const utils::Color32& row0 = (*ReadingBuffer)[0][column];
        const utils::Color32& row1 = (*ReadingBuffer)[1][column];
        const utils::Color32& row2 = (*ReadingBuffer)[2][column];
        const utils::Color32& row3 = (*ReadingBuffer)[3][column];
        const utils::Color32& row4 = (*ReadingBuffer)[4][column];
        const utils::Color32& row5 = (*ReadingBuffer)[5][column];
        const utils::Color32& row6 = (*ReadingBuffer)[6][column];

        // Physical channel order is R, B, G.
        SN_CT16B1->MR0 = row0.R;
        SN_CT16B1->MR1 = row0.B;
        SN_CT16B1->MR2 = row0.G;
        SN_CT16B1->MR3 = row1.R;
        SN_CT16B1->MR4 = row1.B;
        SN_CT16B1->MR5 = row1.G;
        SN_CT16B1->MR6 = row2.R;
        SN_CT16B1->MR7 = row2.B;
        SN_CT16B1->MR8 = row2.G;
        SN_CT16B1->MR9 = row3.R;
        SN_CT16B1->MR10 = row3.B;
        SN_CT16B1->MR11 = row3.G;
        SN_CT16B1->MR12 = row4.R;
        SN_CT16B1->MR13 = row4.B;
        SN_CT16B1->MR14 = row4.G;
        SN_CT16B1->MR16 = row5.R;
        SN_CT16B1->MR17 = row5.B;
        SN_CT16B1->MR18 = row5.G;
        SN_CT16B1->MR19 = row6.R;
        SN_CT16B1->MR20 = row6.B;
        SN_CT16B1->MR21 = row6.G;
    }

    void RGBMatrix::advance() noexcept
    {
        ++CurrentColumn;
        if (CurrentColumn >= MatrixDefinitions::Cols)
        {
            CurrentColumn = 0;
            if (SwapPending)
            {
                std::swap(ReadingBuffer, WritingBuffer);
                SwapPending = false;
            }
        }
    }

    std::uint8_t RGBMatrix::currentColumn() noexcept
    {
        return CurrentColumn;
    }

    void RGBMatrix::handOver() noexcept
    {
        if (ElectricalMatrix::Ownership == SharedOwnership::RGBMatrix)
            ElectricalMatrix::Ownership = SharedOwnership::ScanHandOverRequest;
    }

    bool RGBMatrix::handedOver() noexcept
    {
        return ElectricalMatrix::Ownership == SharedOwnership::Matrix;
    }

    void RGBMatrix::acquire() noexcept
    {
        if (ElectricalMatrix::Ownership != SharedOwnership::Matrix)
            return;
        _configureSelectorPinsOutput();
        _deselectAllColumns();
        SN_CT16B1->IC = 1u << 24;
        ElectricalMatrix::Ownership = SharedOwnership::RGBMatrix;
        _startCurrentColumn();
    }

    void RGBMatrix::_configureSelectorPinsOutput() noexcept
    {
        SN_GPIO2->MODE |= SelectorMaskPortC;
        SN_GPIO1->MODE |= SelectorMaskPortB;
    }

    void RGBMatrix::_deselectAllColumns() noexcept
    {
        // Selectors are active-low, so HIGH means off.
        SN_GPIO2->BSET = SelectorMaskPortC;
        SN_GPIO1->BSET = SelectorMaskPortB;
    }

    void RGBMatrix::_selectColumn(const std::uint8_t column) noexcept
    {
        // Columns: C0-C1, C3-C12, B6-B9.
        // Black magic
        if (column < 2u)
        {
            SN_GPIO2->BCLR = 1u << column;
            return;
        }

        if (column < 12u)
        {
            SN_GPIO2->BCLR = 1u << (column + 1u);
            return;
        }

        SN_GPIO1->BCLR = 1u << (column - 6u);
    }

    void RGBMatrix::_startCurrentColumn() noexcept
    {
        preload();
        _selectColumn(CurrentColumn);
        SN_CT16B1->TMRCTRL = 1u << 1;
        while (SN_CT16B1->TMRCTRL & (1u << 1)) __NOP();
        SlotStartedAt = hal::HighResolutionTimer::rawTicks();
        SN_CT16B1->TMRCTRL = 1u;
        SN_CT16B1->PWMIOENB = PWMOutputMask;
    }

    void RGBMatrix::resume() noexcept
    {
        Running = true;
        SN_CT16B1->IC = 1u << 24;
        NVIC_ClearPendingIRQ(CT16B1_IRQn);
        _startCurrentColumn();
        NVIC_EnableIRQ(CT16B1_IRQn);
    }

    void RGBMatrix::_handleInterrupt() noexcept
    {
        if ((SN_CT16B1->RIS & (1u << 24)) == 0u)
            return;
        const std::uint32_t slotTicks = (hal::HighResolutionTimer::rawTicks() - SlotStartedAt) & 0x00FFFFFFu;
        if (slotTicks > profiling::RGBSlotMaxTicks)
            profiling::RGBSlotMaxTicks = slotTicks;
        SN_CT16B1->IC = 1u << 24;
        SN_CT16B1->PWMIOENB = 0u;
        const auto startTime = hal::HighResolutionTimer::rawTicks();
        _deselectAllColumns();
        advance();
        if (ElectricalMatrix::Ownership == SharedOwnership::ScanHandOverRequest)
        {
            ElectricalMatrix::Ownership = SharedOwnership::Matrix;
            return;
        }

        _startCurrentColumn();
        profiling::RGBTicks = hal::HighResolutionTimer::rawTicks() - startTime;
    }
}

extern "C" void CT16B1_IRQHandler()
{
    quartz::kb::rgb::RGBMatrix::_handleInterrupt();
}
