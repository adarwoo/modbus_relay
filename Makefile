TOP=.

# Select the architecture
ARCH=attiny3224

# Name of the binary to produce
BIN := modbus_relay

# -I throughout (C and C++)
INCLUDE_DIRS = conf src

ASX_USE = modbus_rtu eeprom

# Project own files
SRCS = \
	src/stats.cpp \
	src/config.cpp \
   src/modbus.cpp \
   src/relay_ctrl.cpp \
   src/main.cpp \

ifdef SIM
SRCS += \
   src/test_relay.cpp
endif

# Inlude the actual build rules
include asx/make/rules.mak

# Add dependency to generate the datagram from the config
src/main.cpp : conf/datagram.hpp conf/broadcast_datagram.hpp

#CLEAN_FILES+=conf/datagram.hpp conf/broadcast_datagram.hpp