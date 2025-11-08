#pragma once

#include <asx/uart.hpp>
#include <asx/modbus_rtu.hpp>

#include "config.hpp"
#include "datagram.hpp"

namespace net {
   namespace detail {
      inline bool locate_device = false;
   }

   // Modbus UART baudrate share the same config as the EEProm
   using baud_t = config::baud_t;

   // All APIs declared in datagram.hpp
   using Uart = asx::uart::Uart<1, config::UartRunTimeConfig>;

   // Our relay modbus rtu slave templated class
   using modbus_slave = asx::modbus::Slave<Datagram, Uart>;

   // Get modbus control information
   // Corresponding accessor API
   inline bool get_locate_device_status() {
      return detail::locate_device;
   }

   // Initialise the network
   void init();
   
   // Flip between recovery and normal mode
   void reset_settings();
} // End of namespace net
