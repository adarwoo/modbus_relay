#include <asx/eeprom.hpp>
#include <config.hpp>
#include "datagram.hpp"

using namespace asx;

namespace relay {

   static const auto default_config = EepromConfig{
      .address = 44,
      .baud = 1152, // 100th of the actual rate
      .stopbits = uart::stop::_1,
      .parity = uart::parity::even,
      .watchdog = 5,
      .frequency = 50,
      .infeed_dc_min = 10,
      .infeed_dc_max = 28,
      .infeed_ac_min = 200,
      .infeed_ac_max = 250,
      .estop_on_wd = true,
      .estop_on_relay = true,
      .estop_on_undervolt = true,
      .estop_on_overvolt = true,
   };

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
      eeprom_config.watchdog = period;
      eeprom_config.update();
   }

   void set_estop_on_wd(bool yes) {
      eeprom_config.estop_on_wd = yes;
      eeprom_config.update();
   }

   void set_estop_on_relay(bool yes) {
      eeprom_config.estop_on_relay = yes;
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

   void set_frequency(uint8_t freq) {
      if (freq != 50 && freq != 60) {
         freq = 0;
      }

      eeprom_config.frequency = freq;
      eeprom_config.update();
   }

   void set_infeed_dc_min(uint8_t threshold) {
      eeprom_config.infeed_dc_min = threshold;
      eeprom_config.update();
   }

   void set_infeed_dc_max(uint8_t threshold) {
      eeprom_config.infeed_dc_max = threshold;
      eeprom_config.update();
   }

   void set_infeed_ac_min(uint16_t threshold) {
      eeprom_config.infeed_ac_min = threshold;
      eeprom_config.update();
   }

   void set_infeed_ac_max(uint16_t threshold) {
      eeprom_config.infeed_ac_max = threshold;
      eeprom_config.update();
   }
} // End of relay namespace