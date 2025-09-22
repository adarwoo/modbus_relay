#include <asx/eeprom.hpp>
#include <asx/ulog.hpp>

#include <config.hpp>

#include "datagram.hpp"
#include "state.hpp"
#include "net.hpp"
#include "relay.hpp"


using namespace asx;

namespace config {
   using namespace infeed::literal;

   static const auto default_config = EepromConfig {
      .address                   = 44,
      .baud                      = baud_t::_9600,
      .stopbits                  = uart::stop::_1,
      .parity                    = uart::parity::none,
      .infeed_type               = infeed::CfgType::ac_50hz,
      .infeed_min_volt_threshold = 10_volts,
      .infeed_max_volt_threshold = 250_volts,
      .estop_on_undervolt        = false,
      .estop_on_overvolt         = false,
      .estop_on_bad_voltage_type = false,
      .estop_modbus_watchdog     = 0,
      .relays_config = {
         relay::Config{0},
         relay::Config{0},
         relay::Config{0}
      }
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
      ULOG_INFO("Resetting configuration to default");

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
      ULOG_INFO("Setting communication parameters ID:{} Baud:{} Parity:{} StopBits:{}",
         device_id,
         static_cast<uint8_t>(baud),
         static_cast<uint8_t>(parity),
         static_cast<uint8_t>(stopbits)
      );

      eeprom_config.address = device_id;
      eeprom_config.baud = baud;
      eeprom_config.parity = parity;
      eeprom_config.stopbits = stopbits;
      eeprom_config.update();
   }

   bool set_infeed_config(infeed::CfgType vtype, uint16_t lower_threshold, uint16_t upper_threshold) {
      ULOG_INFO("Setting infeed config type:{}", static_cast<uint8_t>(vtype));
      ULOG_INFO("Setting infeed config thresholds:{} lower:{} upper:{}", lower_threshold, upper_threshold);

      if ( upper_threshold < lower_threshold and ((upper_threshold - lower_threshold) < 10) ) {
         return false;
      }

      eeprom_config.infeed_min_volt_threshold = lower_threshold;
      eeprom_config.infeed_max_volt_threshold = upper_threshold;
      eeprom_config.infeed_type = vtype;
      eeprom_config.update();

      return true;
   }

   void set_watchdog(uint16_t period) {
      ULOG_INFO("Setting watchdog period: {} seconds", period);

      eeprom_config.estop_modbus_watchdog = period;
      eeprom_config.update();
   }

   void set_estop_on_undervolt(bool yes) {
      ULOG_INFO("Setting E-Stop on undervolt: {}", yes);

      eeprom_config.estop_on_undervolt = yes;
      eeprom_config.update();
   }

   void set_estop_on_overvolt(bool yes) {
      ULOG_INFO("Setting E-Stop on overvolt: {}", yes);

      eeprom_config.estop_on_overvolt = yes;
      eeprom_config.update();
   }

   void set_estop_on_bad_voltage_type(bool yes) {
      ULOG_INFO("Setting E-Stop on bad voltage type: {}", yes);

      eeprom_config.estop_on_bad_voltage_type = yes;
      eeprom_config.update();
   }

   void set_estop_commloss_mask(uint16_t mask) {
      ULOG_INFO("Setting E-Stop communication loss mask: 0x{:04X}", mask);

      eeprom_config.estop_commloss_mask = mask;
      eeprom_config.update();
   }

   void set_estop_infeed_mask(uint16_t mask) {
      ULOG_INFO("Setting E-Stop infeed mask: 0x{:04X}", mask);

      eeprom_config.estop_infeed_mask = mask;
      eeprom_config.update();
   }

   /**
    * Set a relay configuration.
    * If ON is 255, false must be 255 - else error.
    * This indicates the relay is disabled.
    *
    * @param index 0 Based index for the relay
    * @param filter_on Value for the on filter
    * @param filter_off Value for the off
    *
    * @return false if the configuration is invalid
    */
   bool set_relay_config(uint8_t index, uint8_t filter_on, uint8_t filter_off) {
      ULOG_INFO("Setting relay config index:{} on_filter:{} off_filter:{}", index, filter_on, filter_off);

      // Accept all values - but if on is 255 then off must be too
      if ( (filter_on == 255) xor (filter_off == 255) ) {
         return false;
      }

      if ( filter_on == 255 or (filter_off == 255) ) {
         eeprom_config.relays_config[index].on_filter = 0;
         eeprom_config.relays_config[index].off_filter = 0;
      } else {
         eeprom_config.relays_config[index].on_filter = filter_on;
         eeprom_config.relays_config[index].off_filter = filter_off;
      }

      eeprom_config.update();

      return true;
   }
} // End of relay namespace