#include "hal/usb/Controller.hpp"
#include "hal/usb/Endpoint.hpp"
#include "debug/Panic.hpp"
#include "cppmcu.h"

namespace quartz::hal::usb
{
    namespace 
    {
        constexpr std::uint32_t USB_VREG33 = 1u << 31;
        constexpr std::uint32_t USB_PHY = 1u << 30;
        constexpr std::uint32_t USB_SIE = 1u << 28;
        constexpr std::uint32_t USB_ESD = 1u << 27;
        constexpr std::uint32_t USB_INIT_CONFIG = USB_VREG33 | USB_PHY | USB_SIE | USB_ESD;
        constexpr std::uint32_t USB_DP_PULLUP = 1u << 29;
        constexpr std::uint32_t USB_CLK_ENABLE = 1u << 4;
        constexpr std::uint32_t BUS_INT_ENABLE = 1u << 31;
        constexpr std::uint32_t USB_INT_ENABLE = 1u << 29;
        constexpr std::uint32_t EPT_INT_ENABLE = 1u << 4;
        constexpr std::uint32_t INITIAL_INT_MASK = BUS_INT_ENABLE | USB_INT_ENABLE | EPT_INT_ENABLE;
        constexpr std::uint32_t PHY_PARAM1 = 0x80000000u;
        constexpr std::uint32_t PHY_PARAM2 = 0x00004004u;
    }

    // USB_SRAM is 256 bytes shared between the 5 endpoints.
    // Offset and size must be aligned by DWORD (4 bytes)
    Controller::EndpointArray Controller::Endpoints = {
        Endpoint(EndpointNumber::EP0, EndpointDirection::Both, 0x00, 0x08),
        Endpoint(EndpointNumber::EP1, EndpointDirection::In, 0x08, 0x18),
        Endpoint(EndpointNumber::EP2, EndpointDirection::In, 0x20, 0x08),
        Endpoint(EndpointNumber::EP3, EndpointDirection::Out, 0x28, 0x40),
        Endpoint(EndpointNumber::EP4, EndpointDirection::In, 0x68, 0x40)
    };

    void Controller::initialize() noexcept
    {
        SN_SYS1->AHBCLKEN |= USB_CLK_ENABLE;
        SN_USB->INTEN = INITIAL_INT_MASK;
        SN_USB->SGCTL = 0x0u;
        SN_USB->PHYPRM = PHY_PARAM1;
        SN_USB->PHYPRM2 = PHY_PARAM2;
        // Set EPnBUFOS for all endpoints except EP0
        for (std::uint8_t i = 1; i < Endpoint::MAX_ENDPOINTS; ++i)
        {
            (&SN_USB->EP1BUFOS)[i - 1] = Endpoints[i].getMemoryOffset();
        }
        // DO NOT connect now
        SN_USB->CFG = USB_INIT_CONFIG;
        reset();
        NVIC_ClearPendingIRQ(USB_IRQn);
        NVIC_EnableIRQ(USB_IRQn);
    }

    void Controller::connect() noexcept
    {
        SN_USB->CFG |= USB_DP_PULLUP;
    }

    void Controller::disconnect() noexcept
    {
        SN_USB->CFG &= ~USB_DP_PULLUP;
    }

    void Controller::reset() noexcept
    {
        SN_USB->INSTSC = -1;
        SN_USB->ADDR = 0;
        getEndpoint(EndpointNumber::EP0).enable();
        for (std::uint8_t i = 1; i < Endpoint::MAX_ENDPOINTS; ++i)
        {
            getEndpoint(static_cast<EndpointNumber>(i)).disable();
        }
    }

    void Controller::configure() noexcept
    {
        // Configure all endpoints except EP0, which is already configured in reset()
        for (std::uint8_t i = 1; i < Endpoint::MAX_ENDPOINTS; ++i)
        {
            getEndpoint(static_cast<EndpointNumber>(i)).configure();
        }
    }

    void Controller::deconfigure() noexcept
    {
        // Deconfigure all endpoints except EP0, which is always configured
        for (std::uint8_t i = 1; i < Endpoint::MAX_ENDPOINTS; ++i)
        {
            getEndpoint(static_cast<EndpointNumber>(i)).deconfigure();
        }
    }

    void Controller::setAddress(const std::uint8_t address) noexcept
    {
        SN_USB->ADDR = address & 0x7Fu;
    }

    bool Controller::isEndpointAvailable(const EndpointNumber) noexcept
    {
        // Always available for now.
        return true;
    }

    Endpoint& Controller::getEndpoint(const EndpointNumber number) noexcept
    {
        HARD_ASSERTC(value(number) < Endpoint::MAX_ENDPOINTS, PanicReason::ENDPT_INVALID_NUM);
        return Endpoints[value(number)];
    }

    Interrupt Controller::getInterruptStatus() noexcept
    {
        return static_cast<Interrupt>(SN_USB->INSTS);
    }

    void Controller::clearInterruptStatus(const Interrupt mask) noexcept
    {
        SN_USB->INSTSC = value(mask);
    }

    Endpoint& Controller::getControlEndpoint() noexcept
    {
        return getEndpoint(EndpointNumber::EP0);
    }
}
