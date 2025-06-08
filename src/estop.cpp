#include <cstdint>
#include <chrono>

#include <asx/ioport.hpp>
#include <asx/reactor.hpp>

#include "conf_board.h"

#include "estop.hpp"
#include "leds.hpp"

namespace estop {
   using namespace asx::ioport;

   // Local variables
   namespace {
      auto status = Status{Status::operational};
      auto cause  = Cause{Cause::none};
      auto diagnostic_code = uint8_t{0};
   }

   auto react_on_end_of_pulse = asx::reactor::bind(
      [] {
         ES_COMMAND.clear();
         led::set(led::LedId::estop, led::LedState::off);
      }
   );

   void init() {
      // Invert the pin - ES closes on power-up
      ES_COMMAND.init(dir_t::in, invert::inverted, value_t::low);
   }

   Status get_status() {
      return status;
   }

   void set_status(const Status new_status) {
      status = new_status;
   }

   uint8_t get_diagnostic_code() {
      return diagnostic_code;
   }

   Cause get_cause() {
      return cause;
   }

   void trigger(Cause cause) {
      // TODO
   }

   void trigger(ExternalTriggerType trigger, uint8_t diagnostic) {
      using namespace std::chrono;

      diagnostic_code = diagnostic;

      // If the device is already in terminal EStop - ignore
      if ( get_status() == Status::terminated ) {
         return;
      }

      switch (trigger) {
      case ExternalTriggerType::reset:
         // Reset the cause
         break;
      case ExternalTriggerType::pulse:
         ES_COMMAND.set(value_t::high);
         led::set(led::LedId::estop, led::LedState::on);
         react_on_end_of_pulse.delay(1s);
         break;
      case ExternalTriggerType::resetable:
         ES_COMMAND.set(value_t::high);
         led::set(led::LedId::estop, led::LedState::on);
         break;
      case ExternalTriggerType::terminal:
         ES_COMMAND.set(value_t::high);
         led::set(led::LedId::estop, led::LedState::on);
         break;
      }
   }
} // namespace estop
