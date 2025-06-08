/**
 * Implement all modbus callback functions
 */
#include <trace.h>
#include <stats.hpp>

#include "modbus.hpp"
#include "estop.hpp"
#include "infeed.hpp"
#include "relay_ctrl.hpp"
#include "conf_version.hpp"

using namespace asx;

namespace relay {
   //
   // Implement all the callbacks
   //
   void on_read_coils(uint8_t addr, uint8_t qty) {
      TRACE_INFO(RELAY, "%d - %d", addr, qty);

      Datagram::pack( uint8_t{1} ); // Number of bytes returned

      uint8_t value = status(2);
      value <<=1;
      value |= status(1);
      value <<=1;
      value |= status(0);

      // If address is 0, keep all, if 1 remove the first etc..
      value >>= addr;

      // Mask to keep the count
      value &= (1 << qty) - 1;

      if ( qty > (3 - addr) ) {
         Datagram::reply_error(modbus::error_t::illegal_data_value);
      } else {
         Datagram::pack(value);
      }
   }

   void on_set_single(uint8_t index, uint16_t operation) {
      TRACE_INFO(RELAY, "%d - %d", index, operation);

      switch ( operation ) {
         case 0x0000: set(index, false);
            break;
         case 0xFF00: set(index);
            break;
         case 0x5500: set(index, !status(index));
            break;
         default:
            break;
      }
   }

   void on_set_multiple(uint8_t values) {
      TRACE_INFO(RELAY, "%.2x", values);

      for ( uint8_t i=0; i<3; ++i ) {
         set( i, values & 1 );
         values >>= 1;
      }

      // For the response, we need to shorten the frame
      // SlaveAddr[1]+FunctionCode[1]+Start[2]+Qty[2]
      Datagram::set_size(6);
   }

   /** Read any of the input register */
   void on_read_inputs(uint8_t addr, uint8_t count) {
      Datagram::pack<uint8_t>(qty*2);

      while ( qty-- ) {
         switch(index++) {
         case 0x00: Datagram::pack( DEVICE_ID ); break;
         case 0x01: Datagram::pack( HW_VERSION ); break;
         case 0x02: Datagram::pack( FW_VERSION ); break;
         case 0x03: Datagram::pack( NUMBER_OF_RELAYS ); break;
         case 0x04: Datagram::pack( NUMBER_OF_BANKS ); break;

         case 0x08: Datagram::pack( estop::get_status_value() ); break;

         case 0x09: Datagram::pack( stat::get_running_minutes() >> 16 ); break;
         case 0x0A: Datagram::pack( stat::get_running_minutes() & 0xFFFF ); break;
         case 0x0B: Datagram::pack( infeed::get_ac_voltage() ); break;
         case 0x0C: Datagram::pack( infeed::get_dc_voltage() ); break;
         case 0x0D: Datagram::pack( estop::get_root_cause_value() ); break;
         case 0x0E: Datagram::pack( estop::get_diagnostic_code() ); break;

         case 0x0F: Datagram::pack( infeed::get_min_voltage() ); break;
         case 0x10: Datagram::pack( infeed::get_max_voltage() ); break;

         case 0x18: Datagram::pack( relay::is_ok(0) ); break;
         case 0x19: Datagram::pack( relay::is_ok(1) ); break;
         case 0x1A: Datagram::pack( relay::is_ok(2) ); break;

         case 0x20: Datagram::pack( relay::get_cycle(0) >> 16 ); break;
         case 0x21: Datagram::pack( relay::get_cycle(0) & 0xFFFF ); break;
         case 0x22: Datagram::pack( relay::get_cycle(1) >> 16 ); break;
         case 0x23: Datagram::pack( relay::get_cycle(1) & 0xFFFF ); break;
         case 0x24: Datagram::pack( relay::get_cycle(2) >> 16 ); break;
         case 0x25: Datagram::pack( relay::get_cycle(3) & 0xFFFF ); break;

         default:
            Datagram::pack(uint16_t{0});
         }
      }
   }

   void on_read_holdings(uint8_t index, uint8_t qty) {
      using namespace datagram;

      pack<uint8_t>(qty*2);
      auto &cfg = config::get_config();

      while ( qty-- ) {
         switch(index++) {
         case 0x00: pack<uint16_t>( cfg.address ); break;
         case 0x01: pack<uint16_t)( cfg.baud ); break;
         case 0x02: pack<uint16_t)( cfg.parity ); break;
         case 0x03: pack<uint16_t)( cfg.stopbits ); break;

         case 0x08: pack<uint16_t>( cfg.infeed_type ); break;
         case 0x09: pack<uint16_t>( cfg.infeed_type == infeed::Type::dc ? cfg.infeed_dc_min : cfg.infeed_ac_min ); break;
         case 0x0A: pack<uint16_t>( cfg.infeed_type == infeed::Type::dc ? cfg.infeed_dc_max : cfg.infeed_ac_max ); break;

         case 0x10: pack<uint16_t>( cfg.estop_on_undervolt); break;
         case 0x11: pack<uint16_t>( cfg.estop_on_overvolt); break;
         case 0x12: pack<uint16_t>( cfg.estop_on_wd); break;

         case 0x18: pack<uint16_t>( cfg.relay_config.value[0] ); break;
         case 0x19: pack<uint16_t>( cfg.relay_config.value[1] ); break;
         case 0x1A: pack<uint16_t>( cfg.relay_config.value[2] ); break;
         }
      }
   }

   // -------------------------------------------------------------------------
   // Write holding
   // -------------------------------------------------------------------------

   void on_write_device_address(uint8_t addr) {
      config::set_device_id(addr);
   }

   void on_write_baud_rate(uint8_t baud) {
      config::set_baud(baud);
   }

   void on_write_parity(uint8_t parity) {
      config::set_parity(parity);
   }

   void on_write_stopbits(uint8_t stopbits) {
      config::set_stopbits(stopbits);
   }

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
      config::set_estop_on_wd(seconds);
   }

   void on_write_estop_settings(uint8_t over, uint8_t under, uint8_t timeout) {
      config::set_estop_on_undervolt(static_cast<bool>(over));
      config::set_estop_on_overvolt(static_cast<bool>(under));
      config::set_estop_on_wd(timeout);
   }

   void on_write_single_relay_cfg(uint8_t address, uint8_t conf, uint8_t filter) {
      config::set_relay_config(address, conf, filter);
   }

   void on_write_relay_cfgs(uint8_t conf1, uint8_t filter1, uint8_t conf2, uint8_t filter2, uint8_t conf3, uint8_t filter3) {
      config::set_relay_config(0, conf1, filter1);
      config::set_relay_config(1, conf2, filter2);
      config::set_relay_config(2, conf3, filter3);
   }

   void on_read_reset() {
      Datagram::pack<uint8_t>(4);
      Datagram::pack<uint32_t>(0xDEAD5AFE);
   }

} // End of namespace relay
