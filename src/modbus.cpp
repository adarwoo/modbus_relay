/**
 * Implement all modbus callback functions
 */
#include <logger.h>
#include "modbus.hpp"
#include "relay_ctrl.hpp"
#include "conf_version.hpp"
#include <stats.hpp>

using namespace asx;

namespace relay {
   //
   // Implement all the callbacks
   //
   void on_read_coils(uint8_t addr, uint8_t qty) {
      LOG_TRACE("RELAY", "%d - %d", addr, qty);

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
      LOG_TRACE("RELAY", "%d - %d", index, operation);

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
      LOG_TRACE("RELAY", "%.2x", values);

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
         case 3: Datagram::pack( 0 ); break;
         case 4: Datagram::pack( get_config().baud ); break;
         case 5: Datagram::pack( (uint16_t)get_config().parity ); break;
         case 6: Datagram::pack( (uint16_t)get_config().stopbits ); break;
         case 7: Datagram::pack( (uint16_t)get_config().watchdog ); break;

         default:
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         }
      }
   }

   void on_read_running_time() {
      Datagram::pack<uint8_t>(4);
      Datagram::pack( stat::get_running_minutes() );
   }

   void on_read_relay_stats(uint8_t index, uint8_t qty) {
      index -= 200;

      if ( ( qty % 1 ) or ( ((qty/2) + index) > 3 ) ) {
         Datagram::reply_error(modbus::error_t::illegal_data_value);
      } else {
         Datagram::pack<uint8_t>(qty*2);

         while ( qty ) {
            Datagram::pack( stat::get_op_count(index++) );
            qty -= 2;
         }
      }
   }
} // End of namespace relay
