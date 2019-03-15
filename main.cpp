#define STM32F030x6
#define F_CPU   48000000UL

#include "periph_rcc.h"
#include "pin.h"
#include "modbus_master.h"

/// эта функция вызываеться первой в startup файле
extern "C" void init_clock ()
{
   // FLASH::set (FLASH::Latency::_1);

   REF(RCC).set (mcu::RCC:: AHBprescaler::AHBnotdiv)
           .set (mcu::RCC:: APBprescaler::APBnotdiv)
           .set (mcu::RCC::  SystemClock::CS_PLL)
           .set (mcu::RCC::    PLLsource::HSIdiv2)
           .set (mcu::RCC::PLLmultiplier::_12)
           .on_PLL()
           .wait_PLL_ready();
}

int main()
{
	constexpr bool parity_enable {true};
    constexpr int timeout {50}; // ms
    UART::Settings set {
          not parity_enable
        , UART::Parity::even
        , UART::DataBits::_8
        , UART::StopBits::_1
        , UART::Baudrate::BR9600
    };

    Register<1, 2> temp;
    Register<3, 7> uf;
    Register<2, 4> time;

    decltype(auto) master = make_modbus_master <
          mcu::Periph::USART1
        , mcu::PA2
        , mcu::PA3
        , mcu::PB1
    >(timeout, set, temp, time, uf);

    while (1) {
        master();
    }
}