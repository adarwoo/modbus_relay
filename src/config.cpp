#include <asx/eeprom.hpp>
#include <config.hpp>

#include "datagram.hpp"
#include "state.hpp"
#include "net.hpp"

using namespace asx;

namespace config {
   using namespace infeed::literal;

   static const auto default_config = EepromConfig {
      .address = 44,
      .baud = baud_t::_9600,
      .stopbits = uart::stop::_1,
      .parity = uart::parity::none,
      .infeed_type = infeed::CfgType::ac_50hz,
      .infeed_min_volt_threshold = 10_volts,
      .infeed_max_volt_threshold = 250_volts,
      .estop_on_undervolt = false,
      .estop_on_overvolt = false,
      .estop_on_bad_voltage_type = false,
      .estop_modbus_watchdog = 0,
      .relays_config = {relay::Config{0}, relay::Config{0}, relay::Config{0}}
   };

   constexpr auto baudrates = std::array<uint32_t, 10>{
      300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
   };

   constexpr bool is_valid_baudrate(uint8_t check) {
      return check < baudrates .size();
   }

   static auto eeprom_config = asx::eeprom::Storage<EepromConfig, 2>(
      default_config);

   /*
    * Getters for the runtime configuration
    */
   uart::parity UartRunTimeConfig::get_parity() {
      return state::is_in_recovery_mode()
         ? uart::parity::none
         : eeprom_config.parity;
   }

   uart::stop UartRunTimeConfig::get_stop() {
      return state::is_in_recovery_mode()
         ? uart::stop::_1
         : eeprom_config.stopbits;
   }

   uint32_t UartRunTimeConfig::get_baud() {
      return state::is_in_recovery_mode()
         ? 9600
         : baudrates[static_cast<uint8_t>(eeprom_config.baud)];
   }

   void reset_config() {
      eeprom_config = default_config;
      eeprom_config.update();
   }

   const EepromConfig& get_config() {
      return eeprom_config;
   }

   /*
    * Modbus callbacks
    */
   void set_comm(uint8_t device_id, baud_t baud, uart::parity parity, uart::stop stopbits) {
      eeprom_config.address = device_id;
      eeprom_config.baud = baud;
      eeprom_config.parity = parity;
      eeprom_config.stopbits = stopbits;
      eeprom_config.update();
   }

   void set_watchdog(uint16_t period) {
      eeprom_config.estop_modbus_watchdog = period;
      eeprom_config.update();
   }

   void set_estop_on_undervolt(bool yes) {
      eeprom_config.estop_on_undervolt = yes;
      eeprom_config.update();
   }

   void set_estop_on_overvolt(bool yes) {
      eeprom_config.estop_on_overvolt = yes;
      eeprom_config.update();
   }

   void set_infeed_min(uint8_t threshold) {
      eeprom_config.infeed_min_volt_threshold = threshold;
      eeprom_config.update();
   }

   void set_infeed_max(uint8_t threshold) {
      eeprom_config.infeed_max_volt_threshold = threshold;
      eeprom_config.update();
   }

   void set_estop_on_bad_voltage_type(bool yes) {
      eeprom_config.estop_on_bad_voltage_type = yes;
      eeprom_config.update();
   }

   /**
    * Set the relay configuration using the modbus value
    * @return false if the configuration is invalid
    */
   bool set_relay_config(uint8_t index, uint16_t filter) {
      if ( filter == 0 ) {
         eeprom_config.relays_config[index].filter_ms = 0;
         eeprom_config.relays_config[index].disabled = false;
      } else if ( filter == 0xFFFF ) {
         eeprom_config.relays_config[index].filter_ms = 0;
         eeprom_config.relays_config[index].disabled = true;
      } else if ( filter >= 100 and filter <= 60000 ) {
         eeprom_config.relays_config[index].filter_ms = filter;
         eeprom_config.relays_config[index].disabled = false;
      } else {
         return false;
      }

      return true;
   }
} // End of relay namespace