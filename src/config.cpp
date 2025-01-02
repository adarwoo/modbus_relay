#include <asx/eeprom.hpp>
#include <config.hpp>
#include "datagram.hpp"

using namespace asx;

namespace relay {

   static const auto default_config = EepromConfig{
      .address = 44,
      .baud = 96, // 100th of the actual rate
      .stopbits = uart::stop::_1,
      .parity = uart::parity::none,
      .watchdog = 0
   };

   static auto eeprom_config = asx::eeprom::Storage<EepromConfig, 4>(default_config);

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
   }

   void config_device(uint8_t addr, uint16_t baud, uint8_t parity, uint8_t stopbits, uint16_t wd) {
      Datagram::set_size(6);
      eeprom_config.address = addr;
      eeprom_config.baud = baud;
      eeprom_config.parity = (uart::parity)parity;
      eeprom_config.stopbits = (uart::stop)stopbits;
      eeprom_config.watchdog = wd;
      eeprom_config.update();
   }
} // End of relay namespace