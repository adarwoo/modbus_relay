#include <cstdint>
#include <chrono>

#include <boost/sml.hpp>

#include <asx/ulog.hpp>
#include <asx/ioport.hpp>
#include <asx/reactor.hpp>

#include "conf_board.h"

#include "estop.hpp"
#include "leds.hpp"
#include "relay.hpp"


namespace estop {
   using namespace asx::ioport;
   using namespace boost::sml;

   // Local variables
   namespace {
      asx::timer::Instance timer_end_of_pulse{};
      asx::reactor::Handle react_on_end_of_pulse{};
   }

   // Events
   struct trigger_event {
      Cause cause;
      uint16_t diagnostic;
      ExternalTriggerType type;
   };

   struct reset_event {};
   struct end_of_pulse {};

   constexpr auto is_terminal = [](const trigger_event &event) {
      return event.cause == Cause::faulty_relay || event.type == ExternalTriggerType::terminal;
   };

   constexpr auto is_pulsed = [](const trigger_event &event) {
      bool retval = event.type == ExternalTriggerType::pulse;
      ULOG_ERROR("Is pulsed: {}", retval);
      return retval;
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

      // Set the LED
      led::refresh();
   };

   constexpr auto terminate = [](const trigger_event &event) {
      detail::current_status = Status::terminated;
      detail::current_cause = event.cause;
      detail::diagnostic = event.diagnostic;

      // Activate the command pin
      ES_COMMAND.set(value_t::high);

      // Set the LED
      led::refresh();
   };

   constexpr auto rst = []() {
      // The cause and diagnostic code are not reset so a diagnostic is possible post crisis

      // Set the status to operational
      detail::current_status = Status::operational;

      // Clear the command pin
      ES_COMMAND.clear();

      // Set the LED
      led::refresh();
   };

   // State Machine
   struct EStopStateMachine {
      auto operator()() const {
         using namespace boost::sml;

         return make_transition_table(
            *"operational"_s  + event<trigger_event> [is_terminal] / terminate = "terminated"_s
            ,"operational"_s  + event<trigger_event> [is_pulsed]   / stop  = "pulse"_s
            ,"operational"_s  + event<trigger_event>               / stop  = "estop"_s
            ,"pulse"_s        + event<end_of_pulse>                / rst   = "operational"_s
            ,"pulse"_s        + event<trigger_event> [is_terminal]         = "terminated"_s
            ,"pulse"_s        + event<trigger_event> [is_pulsed]   / stop
            ,"pulse"_s        + event<trigger_event>                       = "estop"_s
            ,"estop"_s        + event<trigger_event> [is_terminal] / stop  = "terminated"_s // Update status and LED
            ,"estop"_s        + event<reset_event>                 / rst   = "operational"_s
         );
      }
   };

   // State machine instance
   sm<EStopStateMachine> sm;

   auto react_on_end_of_pulse = asx::reactor::bind(
      [] { sm.process_event(end_of_pulse{}); }
   );

   void init() {
      ULOG_MILE("Initialising E-Stop");

      // Invert the pin - ES closes on power-up
      ES_COMMAND.init(dir_t::out, invert::inverted, value_t::low);
   }

   void trigger(Cause cause, uint16_t diagnostic, ExternalTriggerType type ) {
      ULOG_WARN("Triggering E-Stop: Type:0x{:02x} Cause:0x{:02x} Diagnostic:0x{:04x}",
         static_cast<uint8_t>(type), static_cast<uint8_t>(cause), diagnostic
      );

      sm.process_event(trigger_event{cause, diagnostic, type});
      relay::apply_estop();
   }

   void reset() {
      ULOG_WARN("Resetting E-Stop");

      sm.process_event(reset_event{});
   }
} // namespace estop
