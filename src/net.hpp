#pragma once

#include <asx/uart.hpp>
#include <asx/modbus_rtu.hpp>

#include "config.hpp"
#include "datagram.hpp"

namespace net {
   // All APIs declared in datagram.hpp
   using Uart = asx::uart::Uart<1, config::UartRunTimeConfig>;

   // Our relay modbus rtu slave templated class
   using modbus_slave = asx::modbus::Slave<Datagram, Uart>;

   // Get modbus control information
   bool get_locate_device_status();

   // Initialise the network
   void init();

} // End of namespace net
