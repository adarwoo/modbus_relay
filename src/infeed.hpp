#pragma once
/**
 * @file infeed.hpp
 * @brief Infeed voltage monitoring interface.
 * @details This module provides functions to monitor the infeed voltage,
 *          including getting the lowest and highest voltages, the current
 *          input voltage, and the type of input voltage (AC or DC).
 */

#include <cstdint>
#include <limits>

namespace infeed {
   /// @brief < Type of infeed to monitor
   enum class CfgType : uint8_t {
      dc = 0,
      ac_50hz = 1,
      ac_60hz = 2
   };

   /// @brief < Type of infeed to report
   enum class InputType : uint8_t {
      none = 0, ///< No voltage detected
      dc = 0,
      ac = 1
   };

   ///< Status of the infeed monitoring system
   enum class Status : uint8_t {
      none = 0,  ///< No voltage detected
      in_range,  ///< Voltage detected in range
      above,     ///< Infeed monitoring is in EStop state
      below,     ///< Infeed monitoring has been terminated
      wrong_type ///< Voltage detected with a type mismatch (AC/DC)
   };

   namespace literal {
      /// @brief Literal operator for volts, converting to 1/10th of a volt
      constexpr inline uint16_t operator"" _volts(unsigned long long int value) {
         return static_cast<uint16_t>(value * 10);
      }

      /// @brief Literal operator for volts, converting to 1/10th of a volt
      constexpr inline uint16_t operator"" _volts(long double value) {
         return static_cast<uint16_t>(value * 10);
      }
   }

   namespace detail {
      /// @brief Stores the last measured AC voltage
      inline uint16_t last_ac_voltage = 0;
      /// @brief Stores the last measured DC voltage
      inline uint16_t last_dc_voltage = 0;
      /// @brief Stores the maximum voltage measured
      inline uint16_t max_voltage = std::numeric_limits<uint16_t>::max();
      /// @brief Stores the minimum voltage measured
      inline uint16_t min_voltage = std::numeric_limits<uint16_t>::min();
      /// @brief Stores the current infeed status
      inline Status current_status = Status::none;
      /// @brief Reports the current input type
      inline InputType current_input_type = InputType::dc;
   }

   /// @brief Get the status of the infeed monitoring system
   inline Status get_status() {
      return detail::current_status;
   }

   /// @brief Get the current input voltage
   inline uint16_t get_input_voltage() {
      return std::max(detail::last_ac_voltage, detail::last_dc_voltage);
   }

   /// @brief Get the type of the current input voltage
   inline InputType get_input_voltage_type() {
      using namespace literal;
      if ( detail::last_dc_voltage + detail::last_ac_voltage < 10_volts ) {
         return InputType::none;
      }

      if ( detail::last_dc_voltage > detail::last_ac_voltage ) {
         return InputType::dc;
      }

      return InputType::ac;
   }

   /// @brief Get the last measured AC voltage
   inline uint16_t get_lowest_voltage() {
      return detail::min_voltage;
   }

   /// @brief Get the last measured DC voltage
   inline uint16_t get_highest_voltage() {
      return detail::max_voltage;
   }

   /// @brief Reset the minimum and maximum voltage measurements
   inline void reset_min_max() {
      detail::max_voltage = std::numeric_limits<uint16_t>::min();
      detail::min_voltage = std::numeric_limits<uint16_t>::max();
   }

   /// @brief Ready the infeed monitoring system
   void init();
} // namespace infeed
