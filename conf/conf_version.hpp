#pragma once

#include <cstdint>

constexpr auto DEVICE_ID = uint16_t{0x3701};
constexpr auto HW_VERSION = uint16_t{0x0001};
constexpr auto FW_VERSION = uint16_t{0x0001};

constexpr auto NUMBER_OF_RELAYS = uint16_t{0x0003};
constexpr auto NUMBER_OF_BANKS = uint16_t{ 1 + NUMBER_OF_RELAYS / 8 };

/**
 * Version of the schema used for the storing the configuration of the eeprom
 * Increment for every new change.
 * This will force a reformat of the eeprom content
 */
constexpr auto EEPROM_CONFIG_VERSION = 3;