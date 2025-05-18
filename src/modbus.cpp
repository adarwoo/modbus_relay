/**
 * Implement all modbus callback functions
 */
#include <trace.h>
#include <stats.hpp>

#include "modbus.hpp"
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

   void on_read_info(uint8_t index, uint8_t qty) {
      Datagram::pack<uint8_t>(qty*2);

      while ( qty-- ) {
         switch(index++) {
         case 0: Datagram::pack( DEVICE_ID ); break;
         case 1: Datagram::pack( HW_VERSION ); break;
         case 2: Datagram::pack( FW_VERSION ); break;

         default:
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         }
      }
   }

   void on_read_stats(uint8_t index, uint8_t qty) {
      Datagram::pack<uint8_t>(qty*2);

      while ( qty ) {
         switch(index++) {
         case 200: Datagram::pack( stat::get_running_minutes() ); break;
         case 201: Datagram::pack( stat::get_op_count(0) ); break;
         case 202: Datagram::pack( stat::get_op_count(1) ); break;
         case 203: Datagram::pack( stat::get_op_count(2) ); break;
         }
         qty -= 2;
      }
   }

   void on_read_config(uint8_t index, uint8_t qty) {
      Datagram::pack<uint8_t>(qty*2);

      while ( qty-- ) {
         switch(index++) {
         case 100: Datagram::pack( (uint16_t)get_config().address ); break;
         case 101: Datagram::pack( get_config().baud ); break;
         case 102: Datagram::pack( (uint16_t)get_config().parity ); break;
         case 103: Datagram::pack( (uint16_t)get_config().stopbits ); break;

         case 104: Datagram::pack( (uint16_t)get_config().watchdog ); break;

         case 105: Datagram::pack( (uint16_t)get_config().frequency ); break;
         case 106: Datagram::pack( (uint16_t)get_config().infeed_dc_min ); break;
         case 107: Datagram::pack( (uint16_t)get_config().infeed_dc_max ); break;
         case 108: Datagram::pack( (uint16_t)get_config().infeed_ac_min ); break;
         case 109: Datagram::pack( (uint16_t)get_config().infeed_ac_max ); break;

         case 110: Datagram::pack( (uint16_t)get_config().estop_on_wd ); break;
         case 111: Datagram::pack( (uint16_t)get_config().estop_on_relay ); break;
         case 112: Datagram::pack( (uint16_t)get_config().estop_on_undervolt ); break;
         case 113: Datagram::pack( (uint16_t)get_config().estop_on_overvolt ); break;
         }
      }
   }

   /**
    * Write a single register address to change the config
    */
   void on_write_config(uint8_t index, uint16_t value) {
      switch(index++) {
      case 100: // Set the device ID
         if (value < 1 || value > 127) {
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         } else {
            set_device_id(value);
         }
         break;

      case 101: // Set the baud rate (as 1/100th of the actual)
         if ( is_valid_baudrate(value) ) {
            set_baud(value);
         } else {
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         }
         break;

      case 102: // Set the parity
         if ( value > 2 ) {
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         } else {
            set_parity(value);
         }
         break;

      case 103: // Set the stop bits
         if ( value > 2 ) {
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         } else {
            set_stopbits(value);
         }
         break;

      case 104: // Set the watchdog timeout
         if ( value < 1 || value > 60 ) {
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         } else {
            set_watchdog(value);
         }
         break;

      case 105: // Set the frequency
         if ( value != 0 || value != 50 || value != 60 ) {
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         } else {
            set_frequency(value);
         }
         break;

      case 106: // Set the infeed DC min
         if ( value > 100 ) {
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         } else {
            set_infeed_dc_min(value);
         }
         break;

      case 107: // Set the infeed DC max
         if ( value > 120 ) {
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         } else {
            set_infeed_dc_max(value);
         }
         break;

      case 108: // Set the infeed AC min
         if ( value > 250 ) {
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         } else {
            set_infeed_ac_min(value);
         }
         break;

      case 109: // Set the infeed AC max
         if ( value > 250 ) {
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         } else {
            set_infeed_ac_max(value);
         }
         break;

      case 110: // Set the estop on watchdog
         if ( value > 1 ) {
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         } else {
            set_estop_on_wd(value);
         }
         break;
      case 111: // Set the estop on relay
         if ( value > 1 ) {
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         } else {
            set_estop_on_relay(value);
         }
         break;
      case 112: // Set the estop on undervolt
         if ( value > 1 ) {
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         } else {
            set_estop_on_undervolt(value);
         }
         break;
      case 113: // Set the estop on overvolt
         if ( value > 1 ) {
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         } else {
            set_estop_on_overvolt(value);
         }
         break;
      default:
         Datagram::reply_error(modbus::error_t::illegal_data_value);
      }
   }

   void on_read_reset() {
      Datagram::pack<uint8_t>(4);
      Datagram::pack<uint32_t>(0xDEAD5AFE);
   }

} // End of namespace relay
