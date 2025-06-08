#pragma once

#include <cstdint>
#include <asx/uart.hpp>

#include "infeed.hpp"

namespace relay {
   namespace config {
      constexpr auto hundredth_baudrates = std::array<uint16_t, 10>{
         300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
      };

      constexpr bool is_valid_baudrate(uint16_t check) {
         return check < hundredth_baudrates.size();
      }

      struct EepromConfig {
         // Modbus config
         uint8_t address;

         // UART setup
         uint8_t baud; // 100th of the baudrate
         asx::uart::stop stopbits;
         asx::uart::parity parity;

         /**
          * Period of time in seconds of modbus inactivity before estop
          */
         uint16_t watchdog;

         // Infeed config
         infeed::Type infeed_type; // 0=DC, 50 or 60Hz
         uint16_t infeed_dc_min; // Infeed Min DC voltage in 1/10 volts
         uint16_t infeed_dc_max; // Infeed Max DC voltage in 1/10 volts
         uint16_t infeed_ac_min; // Infeed Min AC voltage in 1/10 volts
         uint16_t infeed_ac_max; // Infeed Max AC voltage in 1/10 volts

         // EStop config
         bool estop_on_undervolt; // EStop on infeed undervoltage
         bool estop_on_overvolt;  // EStop on infeed overvoltage
         bool estop_on_wd;        // EStop on watchdog timeout

         // Relay config
         struct Relay {
            union {
               uint16_t value;
               struct {
                  uint8_t debounce_time;
                  uint8_t disable : 1;
                  uint8_t default_position : 1;
                  uint8_t invert : 1;
               };
            };
         } relay_config[3];
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

      void set_device_id(uint8_t id);
      void set_baud(uint16_t baud);
      void set_parity(uint16_t parity);
      void set_stopbits(uint16_t stopbits);
      void set_watchdog(uint16_t period);
      void set_estop_on_wd(bool yes);
      void set_estop_on_relay(bool yes);
      void set_estop_on_undervolt(bool yes);
      void set_estop_on_overvolt(bool yes);
      void set_frequency(uint8_t freq);
      void set_infeed_dc_min(uint8_t threshold);
      void set_infeed_dc_max(uint8_t threshold);
      void set_infeed_ac_min(uint16_t threshold);
      void set_infeed_ac_max(uint16_t threshold);

      void set_
   } // End of config namespace
} // End of relay namespace