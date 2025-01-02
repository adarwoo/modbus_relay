#pragma once

#include <cstdint>
#include <asx/uart.hpp>

namespace relay {
   struct EepromConfig {
      // Modbus config
      uint8_t address;

      // UART setup
      uint16_t baud; // 100th of the baudrate
      asx::uart::stop stopbits;
      asx::uart::parity parity;

      /**
       * Period of time in seconds of bus inactivity
       *  before the relay opens. If 0, the watchdog is off
       */
      uint16_t watchdog;
   };

   void reset_config();

   const EepromConfig& get_config();

   // Run-time configuration for the UART
   struct UartRunTimeConfig {
      static constexpr void init() {}

      static constexpr asx::uart::width get_width() {
         return asx::uart::width::_8;
      }

      static asx::uart::parity get_parity();
      static asx::uart::stop get_stop();
      static uint32_t get_baud();

      static constexpr bool has(int options) {
         return (asx::uart::rs485 | asx::uart::onewire) & options;
      }
   };

} // End of relay namespace