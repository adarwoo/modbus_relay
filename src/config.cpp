#include <asx/eeprom.hpp>
#include <config.hpp>

using namespace asx;

namespace relay {

   static const auto default_config = EepromConfig{
      .address = 44,
      .baud = 9600,
      .stopbits = uart::stop::_1,
      .parity = uart::parity::none,
      .watchdog = 0
   };

   static auto eeprom_config = asx::eeprom::Storage<EepromConfig, 2>(default_config);

   uart::parity UartRunTimeConfig::get_parity() {
      return eeprom_config.parity;
   }

   uart::stop UartRunTimeConfig::get_stop() {
      return eeprom_config.stopbits;
   }

   size_t UartRunTimeConfig::get_baud() {
      return eeprom_config.baud;
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

   void set_config(uint16_t baud, uint16_t parity, uint16_t stopbits) {
      eeprom_config.baud = baud;
      eeprom_config.parity = (uart::parity)parity;
      eeprom_config.stopbits = (uart::stop)stopbits;
      eeprom_config.update();
   }

   void set_watchdog(uint16_t period) {
      eeprom_config.watchdog = period;
   }

} // End of relay namespace