#define STM32F030x6
#define F_CPU   48000000UL

#include "periph_rcc.h"
#include "pin.h"
#include "modbus_master.h"
#include "literals.h"
#include "init_clock.h"
#include <bitset>
#include <algorithm>

/// эта функция вызываеться первой в startup файле
extern "C" void init_clock () { init_clock<8_MHz,F_CPU>(); }

using DI1 = mcu::PA9;
using DI2 = mcu::PA8;
using DI3 = mcu::PB15;
using DI4 = mcu::PB14;

using DO1 = mcu::PB0;
using DO2 = mcu::PA7;
using DO3 = mcu::PA6;
using DO4 = mcu::PA5;

using TX  = mcu::PA2;
using RX  = mcu::PA3;
using RTS = mcu::PB1;

template<int lamps_qty>
bool any_lamps_off (uint16_t lamps_flag)
{
    std::bitset<lamps_qty> bits {lamps_flag};
    return bits.any();
}

int main()
{
    auto [control_us, control_uv, distance_control] = make_pins<mcu::PinMode::Input ,DI1,DI2,DI3>();
    auto [sense_us  , sense_uv  , alarm           ] = make_pins<mcu::PinMode::Output,DO1,DO2,DO3>();

    constexpr bool parity_enable {true};
    constexpr int  timeout       {200_ms};
    constexpr UART::Settings set {
          not parity_enable
        , UART::Parity::even
        , UART::DataBits::_8
        , UART::StopBits::_1
        , UART::Baudrate::BR9600
    };

    constexpr auto uov_address {1};

    struct Work_flags {
        bool us_on :1;
        bool uv_on :1;
        uint16_t   :14;
    };

    struct Modbus {
        Register<uov_address, Modbus_function::read_03      , 4, Work_flags> state;

        Register<uov_address, Modbus_function::force_coil_05, 0> us;
        Register<uov_address, Modbus_function::force_coil_05, 1> uv;

        Register<uov_address, Modbus_function::read_03, 11> lamp_flags;
        Register<uov_address, Modbus_function::read_03, 7>  uv_level;
        Register<uov_address, Modbus_function::read_03, 8>  min_uv_level;
        Register<uov_address, Modbus_function::read_03, 5>  temperature;
        Register<uov_address, Modbus_function::read_03, 6>  max_temperature;
    } modbus;

    modbus.max_temperature = 55; // пока не пришло значение по модбасу

    decltype(auto) modbus_master =
        make_modbus_master <mcu::Periph::USART1, TX, RX, RTS> (
            timeout, set, modbus
        );

    bool overheat {false};
    constexpr auto recovery_temperature {20};

    while (1) {
        modbus_master();

        modbus.us.disable = not distance_control;
        modbus.uv.disable = not distance_control;
        modbus.us = control_us;
        modbus.uv = control_uv;

        Work_flags state = modbus.state;
        sense_uv = state.uv_on;
        sense_us = state.us_on;

        if (overheat |= modbus.temperature > modbus.max_temperature)
            overheat  = modbus.temperature > recovery_temperature;


        alarm = (state.uv_on and any_lamps_off<4>(modbus.lamp_flags))
             or (state.uv_on and modbus.uv_level < modbus.min_uv_level)
             or overheat;

        __WFI();
    }
}

