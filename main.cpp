#define STM32F030x6
#define F_CPU   48000000UL

#include "periph_rcc.h"
#include "pin.h"
#include "modbus_slave.h"
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

struct Control {
    bool control_us : 1;
    bool control_uv : 1;
    bool distance   : 1;
    uint16_t        : 5;
    bool us_on      : 1;
    bool uv_on      : 1;
    bool alarm      : 1;
    uint16_t        : 5;
};

struct In_regs {
    uint16_t res_0;     //0
    uint16_t res_1;     //1
    uint16_t res_2;     //2
    uint16_t res_3;     //3
    Control control;    //4
}__attribute__((packed));

struct Out_regs {
    uint16_t res_0;     //0
    uint16_t res_1;     //1
    uint16_t res_2;     //2
    uint16_t res_3;     //3
    Control control;    //4
}__attribute__((packed));


#define ADR(reg) GET_ADR(In_regs, reg)


int main()
{
    auto [control_us, control_uv, distance_control] = make_pins<mcu::PinMode::Input ,DI1,DI2,DI3>();
    auto [sense_us  , sense_uv  , alarm           ] = make_pins<mcu::PinMode::Output,DO1,DO2,DO3>();

    uint8_t address{101};
    UART::Settings uart_set = {
      .parity_enable  = false,
      .parity         = USART::Parity::even,
      .data_bits      = USART::DataBits::_8,
      .stop_bits      = USART::StopBits::_1,
      .baudrate       = USART::Baudrate::BR9600,
      .res            = 0
    };

    volatile decltype(auto) modbus = Modbus_slave<In_regs, Out_regs>
        ::make<mcu::Periph::USART1, TX, RX, RTS>(address, uart_set);
    

    while (1) {

        if (distance_control) {
            modbus.outRegs.control.control_us = control_us;
            modbus.outRegs.control.control_uv = control_uv;
            modbus.outRegs.control.distance   = distance_control;
        } else {
            modbus.outRegs.control.control_us = false;
            modbus.outRegs.control.control_uv = false;
            modbus.outRegs.control.distance   = false;
        }
        

        modbus([&](auto registr){
            switch (registr) {
                case ADR(control):
                    sense_us = modbus.outRegs.control.us_on = modbus.inRegs.control.us_on;
                    sense_uv = modbus.outRegs.control.uv_on = modbus.inRegs.control.uv_on;
                    alarm    = modbus.outRegs.control.alarm = modbus.inRegs.control.alarm;
                break;
            }
        });
      

        __WFI();
    }
}

