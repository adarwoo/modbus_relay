#pragma once

#include <asx/uart.hpp>

namespace board {

   /** Define the Usart to use by the rs485 */
   using Uart =
      asx::uart::Uart<
         1,                            // We use UART1 with regular pin mux
         115200,                       // Baudrate
         asx::uart::width::_8,         // Width standard 8 bits per frame
         asx::uart::parity::even,      // Even parity (standard for Modbus)
         asx::uart::stop::_1,          // Unique stop bit (standard in Modbus)
         // Force RS485 mode and one wire (Rx pin is not used)
         asx::uart::rs485 | asx::uart::onewire
      >;
}
