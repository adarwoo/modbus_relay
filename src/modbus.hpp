#pragma once

#include <asx/modbus_rtu.hpp>

#include "datagram.hpp"
#include "conf_uart.hpp"

namespace modbus {
   // All APIs declared in datagram.hpp
   using Uart = asx::uart::Uart<1, uart::UartRunTimeConfig>;

   // Our relay modbus rtu slave templated class
   using modbus_slave = asx::modbus::Slave<Datagram, Uart>;
} // End of namespace modbus
