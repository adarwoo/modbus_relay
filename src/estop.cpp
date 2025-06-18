#include <cstdint>
#include <chrono>

#include <boost/sml.hpp>

#include <asx/ioport.hpp>
#include <asx/reactor.hpp>

#include "conf_board.h"

#include "estop.hpp"
#include "leds.hpp"

namespace estop {
   using namespace asx::ioport;
   using namespace boost::sml;

   // Local variables
   namespace {
      asx::timer::Instance timer_end_of_pulse{};
   }

   auto react_on_end_of_pulse = asx::reactor::bind(
      [] { reset(); }
   );

   // Events
   struct trigger_event {
      Cause cause;
      uint16_t diagnostic;
      ExternalTriggerType type;
   };

   struct reset_event {};

   constexpr auto is_terminal = [](const trigger_event &event) {
      return event.cause == Cause::faulty_relay || event.type == ExternalTriggerType::terminal;
   };

   // Actions
   constexpr auto stop = [](const trigger_event &event) {
      timer_end_of_pulse.cancel(); // Cancel any previous pulse delay

      if ( event.type == ExternalTriggerType::pulse ) {
         // Delay for 4 seconds before resetting
         timer_end_of_pulse = react_on_end_of_pulse.delay(std::chrono::seconds(4));
      }

      detail::current_status = Status::estop;
      detail::current_cause = event.cause;
      detail::diagnostic = event.diagnostic;

      // Activate the command pin
      ES_COMMAND.set(value_t::high);
   };

   constexpr auto on_reset = [](const reset_event &) {
      // Reset the cause
      detail::current_cause = Cause::none;

      // Reset the diagnostic code
      detail::diagnostic = 0;

      // Set the status to operational
      detail::current_status = Status::operational;

      // Clear the command pin
      ES_COMMAND.clear();
   };

   // State Machine
   struct EStopStateMachine {
      auto operator()() const {
         using namespace boost::sml;

         return make_transition_table(
            *"operational"_s + event<trigger_event> [is_terminal] / stop = "terminated"_s,
            "operational"_s  + event<trigger_event> / stop               = "estop"_s,
            "estop"_s        + event<trigger_event> [is_terminal] / stop = "terminated"_s,
            "estop"_s        + event<trigger_event> / stop               = "estop"_s,
            "estop"_s        + event<reset_event>   / on_reset           = "operational"_s
         );
      }
   };

   // State machine instance
   sm<EStopStateMachine> sm;

   void init() {
      // Invert the pin - ES closes on power-up
      ES_COMMAND.init(dir_t::out, invert::inverted, value_t::low);
   }

   void trigger(Cause cause, uint16_t diagnostic, ExternalTriggerType type ) {
      sm.process_event(trigger_event{cause, diagnostic, type});
      led::refresh();
   }

   void reset() {
      sm.process_event(reset_event{});
      led::refresh();
   }
} // namespace estop
