#pragma once
/**
 * @file config.hpp
 * @brief Configuration management for the relay module.
 * @details This file provides functions to manage the configuration of the relay module,
 *          including reading and writing configuration parameters to EEPROM.
 *          It includes settings for Modbus, UART, infeed type, EStop conditions,
 *           and relay configurations.
 * @note This principle is the following, each entity in the system such as
 *        relays, infeed, estop etc. is responsible for defining its own types
 *        for the data to be persisted.
 *       A function is then added here to store the data as type safe data.
 *       The adaptation from raw data must be done in the caller (such as for modbus data).
 */

#include <cstdint>

#include <asx/uart.hpp>

#include "infeed.hpp"
#include "relay.hpp"


namespace config {
   /**
    * Specific type for storing the baud rate as 1 bytes
    */
   enum class baud_t : uint8_t {
      _300    = 0,
      _600    = 1,
      _1200   = 2,
      _2400   = 3,
      _4800   = 4,
      _9600   = 5,
      _19200  = 6,
      _38400  = 7,
      _57600  = 8,
      _115200 = 9
   };

   /**
    * @brief Configuration structure stored in EEPROM.
    * This structure contains all the configuration parameters for the relay module.
    */
   struct EepromConfig {
      /// @brief Modbus config
      uint8_t address;
      /// @brief UART setup
      baud_t baud; // Baud rate selection
      asx::uart::stop stopbits;
      asx::uart::parity parity;

      /// @brief Infeed config
      infeed::InputType infeed_type; // 0=Invalid, 1=DC, 2=AC
      uint16_t infeed_min_volt_threshold; // Infeed Min voltage in 1/10 volts
      uint16_t infeed_max_volt_threshold; // Infeed Max voltage in 1/10 volts

      /// @brief EStop config
      bool estop_on_undervolt; // EStop on infeed undervoltage
      bool estop_on_overvolt;  // EStop on infeed overvoltage
      bool estop_on_bad_voltage_type; // EStop on bad voltage type (AC/DC mismatch)
      uint16_t estop_modbus_watchdog; // EStop on watchdog timeout. Period in seconds
      uint16_t estop_infeed_mask; // EStop on infeed mask
      uint16_t estop_commloss_mask; // EStop on comm loss mask

      /// @brief Relay config
      relay::Config relays_config[3];
   };

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


   // -------------------------------------------------------------------------
   // API
   // -------------------------------------------------------------------------

   // Global function
   void init();
   void reset_config();
   const EepromConfig& get_config();

   // Dedicated storage update functions
   // These match the modbus capabilities and groups
   void set_comm(uint8_t device_id, baud_t baud, asx::uart::parity parity, asx::uart::stop stops);
   void set_watchdog(uint16_t period);
   void set_estop_on_undervolt(bool yes);
   void set_estop_on_overvolt(bool yes);
   void set_estop_on_bad_voltage_type(bool yes);
   void set_estop_commloss_mask(uint16_t mask);
   void set_estop_infeed_mask(uint16_t mask);
   bool set_infeed_config(infeed::InputType, uint16_t lower_threshold, uint16_t upper_threshold);
   bool set_relay_config(uint8_t index, uint8_t filter_on, uint8_t filter_off);
} // End of config namespace
