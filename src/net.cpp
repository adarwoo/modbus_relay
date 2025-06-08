/**
 * Implement all modbus callback functions
 */
#include <avr/io.h>
#include <trace.h>
#include <stats.hpp>

#include <chrono>

#include <asx/reactor.hpp>

#include "estop.hpp"
#include "infeed.hpp"
#include "relay_ctrl.hpp"
#include "conf_version.hpp"
#include "net.hpp"

namespace net {
   using namespace asx::modbus;

   // Create an alias for the static method
   using dg = Datagram;

   // Timer instance for the watchdog
   auto watchdog_timer = asx::timer::Instance{};

   // Reactor for the watchdog
   auto react_on_watchdog = asx::reactor::bind(
      []() { estop::trigger(estop::Cause::modbus_watchdog); }
   );

   //
   // Implement all the callbacks
   //
   void on_read_coils(uint8_t addr, uint8_t qty) {
      TRACE_INFO(RELAY, "%d - %d", addr, qty);

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
      TRACE_INFO(RELAY, "%d - %d", index, operation);

      switch ( operation ) {
         case 0x0000: relay::set(index, false);
            break;
         case 0xFF00: relay::set(index);
            break;
         case 0x5500: relay::set(index, !relay::get(index));
            break;
         default:
            break;
      }
   }

   void on_set_multiple(uint8_t values) {
      TRACE_INFO(RELAY, "%.2x", values);

      for ( uint8_t i=0; i<3; ++i ) {
         relay::set( i, values & 1 );
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
         case 0x00: dg::pack( DEVICE_ID ); break;
         case 0x01: dg::pack( HW_VERSION ); break;
         case 0x02: dg::pack( FW_VERSION ); break;
         case 0x03: dg::pack( NUMBER_OF_RELAYS ); break;
         case 0x04: dg::pack( NUMBER_OF_BANKS ); break;

         case 0x08: dg::pack( static_cast<uint16_t>(estop::get_status()) ); break;

         case 0x09: dg::pack( stat::get_running_minutes() >> 16 ); break;
         case 0x0A: dg::pack( stat::get_running_minutes() & 0xFFFF ); break;
         case 0x0B: dg::pack( infeed::get_ac_voltage() ); break;
         case 0x0C: dg::pack( infeed::get_dc_voltage() ); break;
         case 0x0D: dg::pack( static_cast<uint16_t>(estop::get_cause()) ); break;
         case 0x0E: dg::pack( estop::get_diagnostic_code() ); break;

         case 0x0F: dg::pack( infeed::get_min_voltage() ); break;
         case 0x10: dg::pack( infeed::get_max_voltage() ); break;

         case 0x18: dg::pack( relay::is_ok(0) ); break;
         case 0x19: dg::pack( relay::is_ok(1) ); break;
         case 0x1A: dg::pack( relay::is_ok(2) ); break;

         case 0x20: dg::pack( relay::get_cycles(0) >> 16 ); break;
         case 0x21: dg::pack( relay::get_cycles(0) & 0xFFFF ); break;
         case 0x22: dg::pack( relay::get_cycles(1) >> 16 ); break;
         case 0x23: dg::pack( relay::get_cycles(1) & 0xFFFF ); break;
         case 0x24: dg::pack( relay::get_cycles(2) >> 16 ); break;
         case 0x25: dg::pack( relay::get_cycles(3) & 0xFFFF ); break;

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
         case 0x00: dg::pack<uint16_t>( cfg.address ); break;
         case 0x01: dg::pack<uint16_t>( cfg.baud ); break;
         case 0x02: dg::pack( static_cast<uint16_t>(cfg.parity) ); break;
         case 0x03: dg::pack( static_cast<uint16_t>(cfg.stopbits) ); break;

         case 0x08: dg::pack( static_cast<uint16_t>(cfg.infeed_type) ); break;
         case 0x09: dg::pack<uint16_t>( cfg.infeed_min ); break;
         case 0x0A: dg::pack<uint16_t>( cfg.infeed_max ); break;

         case 0x10: dg::pack<uint16_t>( cfg.estop_on_undervolt); break;
         case 0x11: dg::pack<uint16_t>( cfg.estop_on_overvolt); break;
         case 0x12: dg::pack<uint16_t>( cfg.estop_modbus_watchdog); break;

         case 0x18: dg::pack<uint16_t>( cfg.relays_config[0].value ); break;
         case 0x19: dg::pack<uint16_t>( cfg.relays_config[1].value ); break;
         case 0x1A: dg::pack<uint16_t>( cfg.relays_config[2].value ); break;
         }
      }
   }

   // -------------------------------------------------------------------------
   // Write holding
   // -------------------------------------------------------------------------

   void on_write_comms_settings(uint8_t addr, uint8_t baud, uint8_t parity, uint8_t stopbits) {
      config::set_device_id(addr);
      config::set_baud(baud);
      config::set_parity(parity);
      config::set_stopbits(stopbits);
   }

   void on_write_estop_on_under(uint8_t onoff) {
      config::set_estop_on_undervolt(static_cast<bool>(onoff));
   }

   void on_write_estop_on_over(uint8_t onoff) {
      config::set_estop_on_overvolt(static_cast<bool>(onoff));
   }

   void on_write_estop_on_timeout(uint16_t seconds) {
      config::set_watchdog(seconds);
   }

   void on_write_estop_settings(uint8_t over, uint8_t under, uint8_t timeout) {
      config::set_estop_on_undervolt(static_cast<bool>(over));
      config::set_estop_on_overvolt(static_cast<bool>(under));
      config::set_watchdog(timeout);
   }

   void on_write_single_relay_cfg(uint8_t address, uint8_t conf, uint8_t filter) {
      config::set_relay_config(address, conf, filter);
   }

   void on_write_relay_cfgs(uint8_t conf1, uint8_t filter1, uint8_t conf2, uint8_t filter2, uint8_t conf3, uint8_t filter3) {
      config::set_relay_config(0, conf1, filter1);
      config::set_relay_config(1, conf2, filter2);
      config::set_relay_config(2, conf3, filter3);
   }

   // Trigger an estop
   void on_estop_set(uint8_t type, uint8_t diag) {
      // If the device is already on terminal EStop - return an error
      if ( estop::get_status() == estop::Status::terminated ) {
         dg::reply_error(error_t::negative_acknowledge);
      } else {
         estop::trigger(static_cast<estop::ExternalTriggerType>(type), diag);
      }
   }

   void on_measurement_reset() {
      infeed::reset_min_max();
   }

   bool locate_device = false;

   void on_locate(uint8_t onoff) {
      locate_device = static_cast<bool>(onoff);
   }

   // Corresponding accessor API
   bool get_locate_device_status() {
      return locate_device;
   }

   void on_factory_reset() {
      config::reset_config();
   }

   void on_reset() {
      // Manually trigger a reset
      ccp_write_io((uint8_t *)&RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
   }

   // Implement this method to reset the watchdog
   void on_payload_received(std::string_view x) {
      watchdog_timer.cancel();
      auto per = config::get_config().estop_modbus_watchdog;

      if ( per ) {
         watchdog_timer = react_on_watchdog.delay( std::chrono::seconds(per) );
      }
   }

   void init() {
      // Set the modbus install ID
      dg::set_device_id(config::get_config().address);

      // Initialise the modbus slave template API. Overrides the UART settings
      modbus_slave::init();
   }

} // End of namespace net
