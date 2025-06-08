#pragma once

#include <cstdint>

constexpr auto DEVICE_ID = uint16_t{0x3701};
constexpr auto HW_VERSION = uint16_t{0x0001};
constexpr auto FW_VERSION = uint16_t{0x0001};

constexpr auto NUMBER_OF_RELAYS = uint16_t{0x0003};
constexpr auto NUMBER_OF_BANKS = uint16_t{ 1 + NUMBER_OF_RELAYS / 8 };
