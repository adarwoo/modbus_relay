#pragma GCC optimize("Os")

/**
 * Implement all modbus callback functions
 */
#include <avr/io.h>

#include <chrono>

#include <asx/reactor.hpp>
#include <asx/ulog.hpp>

#include "estop.hpp"
#include "infeed.hpp"
#include "counters.hpp"
#include "relay.hpp"
#include "conf_version.hpp"
#include "net.hpp"
#include "state.hpp"

namespace net {
   using namespace asx::modbus;

   // Create an alias for the static method
   using dg = Datagram;

   //
   // Constants
   //

   // ID to use in recovery
   auto constexpr RECOVERY_DEVICE_ID = uint8_t{248};

   // The register position gives us the relay index
   // WARNING: Keep in sync with the python definition
   auto constexpr RELAY_0_CONFIG_REGISTER_ADDRESS = 0x18;

   // Timer instance for the watchdog
   auto watchdog_timer = asx::timer::Instance{};

   // Reactor for the watchdog
   auto react_on_watchdog = asx::reactor::bind(
      []() {
         estop::trigger(
            estop::Cause::modbus_watchdog,
            config::get_config().estop_modbus_watchdog
         );
      }
   );

   // Callback when a valid packet is recieved
   void on_ready_reply(std::string_view view) {
      uint16_t t = config::get_config().estop_modbus_watchdog;

      if ( t > 0 ) {
         // Reset the watchdog

         // Stop the previous one
         watchdog_timer.cancel();

         // Arm a fresh one
         watchdog_timer = react_on_watchdog.delay(std::chrono::seconds(t));
      }
   }

   //
   // Implement all the callbacks
   //
   void on_read_coils(uint8_t addr, uint8_t qty) {
      ULOG_INFO("Read Coils Address: {} - Quantity: {}", addr, qty);

      dg::pack( uint8_t{1} ); // Number of bytes returned

      uint8_t value = relay::get(2);
      value <<=1;
      value |= relay::get(1);
      value <<=1;
      value |= relay::get(0);

      // If address is 0, keep all, if 1 remove the first etc..
      value >>= addr;

      // Mask to keep the count
      value &= (1 << qty) - 1;

      if ( qty > (3 - addr) ) {
         dg::reply_error(error_t::illegal_data_value);
      } else {
         dg::pack(value);
      }
   }

   void on_set_single(uint8_t index, uint16_t operation) {
      ULOG_INFO("Setting single relay index:{} - operation:{}", index, operation);
      bool success = false;

      switch ( operation ) {
         case 0x0000: success = relay::set(index, false);
            break;
         case 0xFF00: success = relay::set(index);
            break;
         case 0x5500: success = relay::toggle(index);
            break;
         default:
            break;
      }

      if ( !success ) {
         dg::reply_error(error_t::slave_device_failure);
      }
   }

   void on_set_multiple(uint8_t values) {
      ULOG_INFO("Setting multiple relays: Values: 0b{:08b}", values);

      for ( uint8_t i=0; i<3; ++i ) {
         relay::set( i, values & 1 ); // Ignore the reply
         values >>= 1;
      }

      // For the response, we need to shorten the frame
      // SlaveAddr[1]+FunctionCode[1]+Start[2]+Qty[2]
      dg::set_size(6);
   }

   /** Read any of the input register */
   void on_read_inputs(uint8_t addr, uint8_t qty) {
      // Reply with a byte count
      dg::pack<uint8_t>(qty*2);

      while ( qty-- ) {
         switch(addr++) {
         // Device identification
         case 0x00: dg::pack( DEVICE_ID ); break;
         case 0x01: dg::pack( HW_VERSION ); break;
         case 0x02: dg::pack( FW_VERSION ); break;
         case 0x03: dg::pack( NUMBER_OF_RELAYS ); break;

         // Status & Monitoring
         case 0x08: dg::pack( static_cast<uint16_t>(estop::get_status()) ); break;
         case 0x09: dg::pack<uint16_t>( counter::get_running_minutes() >> 16 ); break;
         case 0x0A: dg::pack<uint16_t>( counter::get_running_minutes() & 0xFFFF ); break;

         case 0x0B: dg::pack( static_cast<uint16_t>(infeed::get_input_voltage_type()) ); break;
         case 0x0C: dg::pack( infeed::get_input_voltage() ); break;
         case 0x0D: dg::pack( infeed::get_highest_voltage() ); break;
         case 0x0E: dg::pack( infeed::get_lowest_voltage() ); break;

         case 0x0F: dg::pack( static_cast<uint16_t>(estop::get_cause()) ); break;
         case 0x10: dg::pack( static_cast<uint16_t>(estop::get_diagnostic_code()) ); break;

         // Relay Diagnostics & Stats
         case 0x18: dg::pack( static_cast<uint16_t>(relay::get_status(0)) ); break;
         case 0x19: dg::pack<uint16_t>( counter::get(0) >> 16 ); break;
         case 0x1A: dg::pack<uint16_t>( counter::get(0) & 0xFFFF ); break;

         case 0x1B: dg::pack( static_cast<uint16_t>(relay::get_status(1)) ); break;
         case 0x1C: dg::pack<uint16_t>( counter::get(1) >> 16 ); break;
         case 0x1D: dg::pack<uint16_t>( counter::get(1) & 0xFFFF ); break;

         case 0x1E: dg::pack( static_cast<uint16_t>(relay::get_status(2)) ); break;
         case 0x1F: dg::pack<uint16_t>( counter::get(2) >> 16 ); break;
         case 0x20: dg::pack<uint16_t>( counter::get(2) & 0xFFFF ); break;

         default:
            dg::pack(uint16_t{0});
         }
      }
   }

   void on_read_holdings(uint8_t index, uint8_t qty) {
      // Number of bytes returned
      dg::pack<uint8_t>(qty*2);
      auto &cfg = config::get_config();

      while ( qty-- ) {
         switch(index++) {
         // Communication settings
         case 0x00: dg::pack<uint16_t>( cfg.address ); break;
         case 0x01: dg::pack<uint16_t>( static_cast<uint16_t>(cfg.baud) ); break;
         case 0x02: dg::pack<uint16_t>( static_cast<uint16_t>(cfg.parity) ); break;
         case 0x03: dg::pack<uint16_t>( static_cast<uint16_t>(cfg.stopbits) ); break;

         // Power Infeed Configuration
         case 0x08: dg::pack<uint16_t>( static_cast<uint16_t>(cfg.infeed_type) ); break;
         case 0x09: dg::pack<uint16_t>( cfg.infeed_min_volt_threshold ); break;
         case 0x0A: dg::pack<uint16_t>( cfg.infeed_max_volt_threshold ); break;

         // Safety Logic Configuration
         case 0x10: dg::pack<uint16_t>( cfg.estop_on_undervolt); break;
         case 0x11: dg::pack<uint16_t>( cfg.estop_on_overvolt); break;
         case 0x12: dg::pack<uint16_t>( cfg.estop_on_bad_voltage_type); break;
         case 0x13: dg::pack<uint16_t>( cfg.estop_modbus_watchdog); break;

         // Relay Configuration
         case 0x18:
         case 0x19:
         case 0x1A:
            dg::pack( cfg.relays_config[index-RELAY_0_CONFIG_REGISTER_ADDRESS-1].on_filter );
            dg::pack( cfg.relays_config[index-RELAY_0_CONFIG_REGISTER_ADDRESS-1].off_filter );
            break;

         default:
            dg::pack(uint16_t{0});
         }
      }
   }

   // -------------------------------------------------------------------------
   // Write holding
   // -------------------------------------------------------------------------
   void on_write_comms_settings(uint8_t addr, uint8_t baud, uint8_t parity, uint8_t stopbits) {
      // Note : All ranges have already been enforced
      if ( state::is_in_recovery_mode() ) {
         config::set_comm(
            addr,
            static_cast<baud_t>(baud),
            static_cast<asx::uart::parity>(parity),
            static_cast<asx::uart::stop>(stopbits)
         );

         // Drop out of recovery mode. This will reset the UART
         state::set_recovery_mode(false);

         // For the response, we need to shorten the frame
         // SlaveAddr[1]+FunctionCode[1]+Start[2]+Qty[2]
         dg::set_size(6);
      } else {
         dg::reply_error(error_t::negative_acknowledge);
      }
   }

   void on_write_infeed_config(uint8_t vtype, uint16_t lower_threshold, uint16_t upper_threshold) {
      auto res = config::set_infeed_config(
         static_cast<infeed::CfgType>(vtype), lower_threshold, upper_threshold
      );

      if ( res ) {
         // For the response, we need to shorten the frame
         // SlaveAddr[1]+FunctionCode[1]+Start[2]+Qty[2]
         dg::set_size(6);
      } else {
         dg::reply_error(error_t::negative_acknowledge);
      }
   }

   void on_write_estop_on_under(uint8_t onoff) {
      config::set_estop_on_undervolt(static_cast<bool>(onoff));
   }

   void on_write_estop_on_over(uint8_t onoff) {
      config::set_estop_on_overvolt(static_cast<bool>(onoff));
   }

   void on_write_estop_on_bad_voltage_type(uint8_t onoff) {
      config::set_estop_on_bad_voltage_type(static_cast<bool>(onoff));
   }

   void on_write_estop_on_timeout(uint16_t seconds) {
      config::set_watchdog(seconds);
   }

   void on_write_estop_settings(uint8_t over, uint8_t under, uint8_t timeout) {
      config::set_estop_on_undervolt(static_cast<bool>(over));
      config::set_estop_on_overvolt(static_cast<bool>(under));
      config::set_watchdog(timeout);
   }

   void on_write_single_relay_cfg(uint8_t address, uint8_t filter_on, uint8_t filter_off) {
      // We need to offset the address
      uint8_t index = address - RELAY_0_CONFIG_REGISTER_ADDRESS;

      // Make sure not to go over!
      alert_and_stop_if( index > 2 );

      // This configuration must follow rules. Check that the change was accepted
      if ( not config::set_relay_config(index, filter_on, filter_off) ) {
         dg::reply_error(error_t::illegal_data_value);
      }
   }

   // Trigger an estop
   void on_estop(uint8_t type, uint8_t diag) {
      // If the device is already on terminal EStop - return an error
      if ( estop::get_status() == estop::Status::terminated ) {
         dg::reply_error(error_t::negative_acknowledge);
      } else {
         estop::trigger(
            estop::Cause::command,
            diag,
            static_cast<estop::ExternalTriggerType>(type)
         );
      }
   }

   void on_measurement_reset() {
      infeed::reset_min_max();
   }

   void on_locate(uint8_t onoff) {
      state::set_locate_mode(onoff);
   }

   void on_factory_reset() {
      config::reset_config();
   }

   void on_reset() {
      // Manually trigger a reset
      ccp_write_io((uint8_t *)&RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
   }

   void on_exit_recovery() {
      ULOG_MILE("Exit recovery");

      state::set_recovery_mode(false);
   }

   // Implement this method to reset the watchdog
   void on_payload_received(std::string_view x) {
      watchdog_timer.cancel();
      auto per = config::get_config().estop_modbus_watchdog;

      if ( per ) {
         watchdog_timer = react_on_watchdog.delay( std::chrono::seconds(per) );
      }
   }

   /**
    * Called at start, and during recovery mode activation/deactivation
    * by the state manager.
    */
   void init() {
      // Set the modbus install ID
      dg::set_device_id(
         state::is_in_recovery_mode()
            ? RECOVERY_DEVICE_ID
            : config::get_config().address
      );

      // Initialise the modbus slave template API. Overrides the UART settings
      modbus_slave::init();
   }

} // End of namespace net
