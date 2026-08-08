#include "RGBMatrix.hpp"
extern "C" {
    #include "SN32F240B.h"
}

namespace quartz::kb::rgb
{
    namespace
    {
        constexpr std::uint32_t PWMOutputMask = 0x003F7FFFu;

        constexpr std::uint32_t SelectorMaskPortC = 0x1FFBu; // P2.0, P2.1, P2.3-P2.12
        constexpr std::uint32_t SelectorMaskPortB = 0x03C0u; // P1.6-P1.9

        // CT16B1:
        // 48 MHz / (PRE + 1) / 256
        //
        // PRE = 47:
        // 48 MHz / 48 / 256 = 3.90625 kHz
        // one complete PWM cycle ~= 262.144 us.
        constexpr std::uint32_t PWMPrescaler = 47u;
        constexpr std::uint32_t PWMPeriod = 255u;
    }

    void RGBMatrix::initialize() noexcept
    {
        // CT16B1 clock.
        SN_SYS1->AHBCLKEN |= (1u << 7);

        // Keep outputs electrically blank while configuring everything.
        SN_CT16B1->PWMIOENB = 0u;
        SN_CT16B1->TMRCTRL = 0u;

        SN_CT16B1->PRE = PWMPrescaler;

        // 8-bit PWM period.
        SN_CT16B1->MR24 = PWMPeriod;

        // Mode 2:
        // TC < MRn => HIGH
        // TC >= MRn => LOW
        //
        // Therefore the framebuffer byte can map directly to MRn:
        //   0   = off
        //   255 = essentially full duty.
        SN_CT16B1->PWMCTRL = 0x55555555u;
        SN_CT16B1->PWMCTRL2 = 0x55555555u;

        // Enable only the 21 physically-used PWM channels:
        // PWM0..14 and PWM16..21.
        SN_CT16B1->PWMENB = PWMOutputMask;

        SN_CT16B1->MCTRL = 0u;
        SN_CT16B1->MCTRL2 = 0u;

        // MR24RST. Automatically reset TC at the end of every PWM cycle.
        // No MR24 interrupt and no MR24STOP.
        SN_CT16B1->MCTRL3 = (1u << 13);

        // Reset TC + PC.
        SN_CT16B1->TMRCTRL = (1u << 1);
        while (SN_CT16B1->TMRCTRL & (1u << 1)) {
        }

        configureSelectorPinsOutput();
        deselectAllColumns();

        clear();
        preload();

        // Leave RGB disabled. Whoever owns the matrix scheduler decides
        // when the first visible RGB phase begins.
    }

    void RGBMatrix::clear() noexcept
    {
        fill(0u, 0u, 0u);
    }

    void RGBMatrix::fill(const Color color) noexcept
    {
        for (std::size_t row = 0; row < Rows; ++row) {
            for (std::size_t column = 0; column < Columns; ++column) {
                Framebuffer[row][column] = color;
            }
        }
    }

    void RGBMatrix::fill(const std::uint8_t r, const std::uint8_t g, const std::uint8_t b) noexcept
    {
        fill(Color{
            .R = r,
            .G = g,
            .B = b
        });
    }

    void RGBMatrix::setPixel(const std::uint8_t row, const std::uint8_t column, const Color color) noexcept
    {
        if (row >= Rows || column >= Columns)
            return;

        Framebuffer[row][column] = color;
    }

    void RGBMatrix::setPixel(const std::uint8_t row, const std::uint8_t column, const std::uint8_t r, const std::uint8_t g, const std::uint8_t b) noexcept
    {
        setPixel(row, column, Color{
            .R = r,
            .G = g,
            .B = b
        });
    }

    RGBMatrix::Color RGBMatrix::getPixel(const std::uint8_t row, const std::uint8_t column) noexcept
    {
        if (row >= Rows || column >= Columns)
            return {};

        return Framebuffer[row][column];
    }

    void RGBMatrix::disable() noexcept
    {
        // First electrically disconnect the PWM outputs.
        SN_CT16B1->PWMIOENB = 0u;

        // Stop the PWM counter while the matrix owns the shared pins.
        SN_CT16B1->TMRCTRL = 0u;

        // Ensure every RGB selector is inactive before Matrix changes
        // their GPIO configuration.
        deselectAllColumns();
    }

    void RGBMatrix::preload() noexcept
    {
        const std::uint8_t column = CurrentColumn;

        const Color& row0 = Framebuffer[0][column];
        const Color& row1 = Framebuffer[1][column];
        const Color& row2 = Framebuffer[2][column];
        const Color& row3 = Framebuffer[3][column];
        const Color& row4 = Framebuffer[4][column];
        const Color& row5 = Framebuffer[5][column];
        const Color& row6 = Framebuffer[6][column];

        // Stock physical channel order is R, B, G.

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

        // MR15 is not physically used.

        SN_CT16B1->MR16 = row5.R;
        SN_CT16B1->MR17 = row5.B;
        SN_CT16B1->MR18 = row5.G;

        SN_CT16B1->MR19 = row6.R;
        SN_CT16B1->MR20 = row6.B;
        SN_CT16B1->MR21 = row6.G;
    }

    void RGBMatrix::enable() noexcept
    {
        // Matrix::end() may have left these as inputs.
        configureSelectorPinsOutput();

        // Start from a known electrically-off selector state.
        deselectAllColumns();

        // Select exactly one RGB column.
        // Reverse only
        selectColumn(15u - CurrentColumn);

        // Restart each RGB slot from TC=0 so brightness doesn't depend on
        // whatever phase the timer happened to be at previously.
        SN_CT16B1->TMRCTRL = (1u << 1);
        while (SN_CT16B1->TMRCTRL & (1u << 1)) {
        }

        SN_CT16B1->TMRCTRL = 1u;

        // Only expose PWM after the correct column is selected and the
        // timer is running from the beginning of a PWM cycle.
        SN_CT16B1->PWMIOENB = PWMOutputMask;
    }

    void RGBMatrix::advance() noexcept
    {
        ++CurrentColumn;

        if (CurrentColumn >= Columns)
            CurrentColumn = 0;
    }

    void RGBMatrix::configureSelectorPinsOutput() noexcept
    {
        SN_GPIO2->MODE |= SelectorMaskPortC;
        SN_GPIO1->MODE |= SelectorMaskPortB;
    }

    void RGBMatrix::deselectAllColumns() noexcept
    {
        // Selectors are active-low, so HIGH means off.
        SN_GPIO2->BSET = SelectorMaskPortC;
        SN_GPIO1->BSET = SelectorMaskPortB;
    }

    void RGBMatrix::selectColumn(const std::uint8_t column) noexcept
    {
        if (column < 2u) {
            // col0 -> P2.0
            // col1 -> P2.1
            SN_GPIO2->BCLR = (1u << column);
            return;
        }

        if (column < 12u) {
            // col2  -> P2.3
            // ...
            // col11 -> P2.12
            SN_GPIO2->BCLR = (1u << (column + 1u));
            return;
        }

        // col12 -> P1.6
        // col13 -> P1.7
        // col14 -> P1.8
        // col15 -> P1.9
        SN_GPIO1->BCLR = (1u << (column - 6u));
    }
}