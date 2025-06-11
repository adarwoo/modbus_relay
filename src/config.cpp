#include <asx/eeprom.hpp>
#include <config.hpp>
#include "datagram.hpp"

using namespace asx;

namespace config {
   using namespace infeed::literal;

   static const auto default_config = EepromConfig {
      .address = 44,
      .baud = 5, // 9600
      .stopbits = uart::stop::_1,
      .parity = uart::parity::even,
      .infeed_type = infeed::CfgType::ac_50hz,
      .infeed_min_volt_threshold = 10_volts,
      .infeed_max_volt_threshold = 250_volts,
      .estop_on_undervolt = false,
      .estop_on_overvolt = false,
      .estop_on_bad_voltage_type = false,
      .estop_modbus_watchdog = 0,
      .relays_config = {relay::Config{0}, relay::Config{0}, relay::Config{0}}
   };

   constexpr auto hundredth_baudrates = std::array<uint32_t, 10>{
      300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
   };

   constexpr bool is_valid_baudrate(uint8_t check) {
      return check < hundredth_baudrates.size();
   }

   static auto eeprom_config = asx::eeprom::Storage<EepromConfig, 6>(default_config);

   uart::parity UartRunTimeConfig::get_parity() {
      return eeprom_config.parity;
   }

   uart::stop UartRunTimeConfig::get_stop() {
      return eeprom_config.stopbits;
   }

   uint32_t UartRunTimeConfig::get_baud() {
      return eeprom_config.baud * 100UL;
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
   void set_device_id(uint8_t id) {
      eeprom_config.address = id;
      eeprom_config.update();
   }

   void set_baud(uint16_t baud) {
      eeprom_config.baud = baud;
      eeprom_config.update();
   }

   void set_parity(uint16_t parity) {
      eeprom_config.parity = (uart::parity)parity;
      eeprom_config.update();
   }

   void set_stopbits(uint16_t stopbits) {
      eeprom_config.stopbits = (uart::stop)stopbits;
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
   void set_relay_config(uint8_t address, uint8_t conf, uint8_t filter) {
      eeprom_config.relays_config[address].debounce_time = filter;
      eeprom_config.relays_config[address].config = conf;
   }
} // End of relay namespace