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
   /// @brief < Type of infeed to report
   enum class InputType : uint8_t {
      none = 0, ///< Ignore infeed
      dc = 1,
      ac = 2
   };

   ///< Status of the infeed monitoring system
   enum class Status : uint8_t {
      none = 0,  ///< No voltage detected
      voltage_present, ///< Voltage detected
      faulty,    ///< Infeed monitoring is faulty
      estop,     ///< Infeed monitoring has triggered an EStop
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
      inline int16_t last_dc_voltage = 0;
      /// @brief Stores the maximum voltage measured
      inline uint16_t max_voltage = std::numeric_limits<uint16_t>::min();
      /// @brief Stores the minimum voltage measured
      inline uint16_t min_voltage = std::numeric_limits<uint16_t>::max();
      /// @brief Stores the current infeed status
      inline Status current_status = Status::none;
      /// @brief Reports the current input type
      inline InputType current_input_type = InputType::none;
   }

   /// @brief Get the status of the infeed monitoring system
   inline Status get_status() {
      return detail::current_status;
   }

   /// @brief Get the current input voltage
   inline uint16_t get_input_voltage() {
      return std::max(
         detail::last_ac_voltage,
         static_cast<uint16_t>(std::abs(detail::last_dc_voltage))
      );
   }

   /// @brief Get the type of the current input voltage
   inline InputType get_input_voltage_type() {
      return detail::current_input_type;
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
