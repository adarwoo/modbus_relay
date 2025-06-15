#pragma once
/**
 * @file config.hpp
 * @brief Configuration management for the relay module.
 * @details This file provides functions to manage the configuration of the relay module,
 *          including reading and writing configuration parameters to EEPROM.
 *          It includes settings for Modbus, UART, infeed type, EStop conditions,
 *           and relay configurations.
 */

#include <cstdint>

#include <asx/uart.hpp>

#include "infeed.hpp"
#include "relay_ctrl.hpp"

namespace config {
   /**
    * @brief Configuration structure stored in EEPROM.
    * This structure contains all the configuration parameters for the relay module.
    */
   struct EepromConfig {
      /// @brief Modbus config
      uint8_t address;
      /// @brief UART setup
      uint8_t baud; // 100th of the baudrate
      asx::uart::stop stopbits;
      asx::uart::parity parity;

      /// @brief Infeed config
      infeed::CfgType infeed_type; // 0=DC, 1=50 or 2=60Hz
      uint16_t infeed_min_volt_threshold; // Infeed Min voltage in 1/10 volts
      uint16_t infeed_max_volt_threshold; // Infeed Max voltage in 1/10 volts

      /// @brief EStop config
      bool estop_on_undervolt; // EStop on infeed undervoltage
      bool estop_on_overvolt;  // EStop on infeed overvoltage
      bool estop_on_bad_voltage_type; // EStop on bad voltage type (AC/DC mismatch)
      uint16_t estop_modbus_watchdog; // EStop on watchdog timeout. Period in seconds

      /// @brief Relay config
      relay::Config relays_config[3];
   };

   void reset_config();

   const EepromConfig& get_config();

   void set_device_id(uint8_t id);
   void set_baud(uint16_t baud);
   void set_parity(uint16_t parity);
   void set_stopbits(uint16_t stopbits);

   void set_watchdog(uint16_t period);
   void set_estop_on_undervolt(bool yes);
   void set_estop_on_overvolt(bool yes);
   void set_estop_on_bad_voltage_type(bool yes);
   void set_infeed_min_voltage_threshold(uint16_t threshold);
   void set_infeed_max_voltage_threshold(uint16_t threshold);

   void set_relay_config(uint8_t address, uint8_t conf, uint8_t filter);

   void reset_config();

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

} // End of config namespace
