TOP=..

# Select the architecture
ARCH=attiny3224

# Name of the binary to produce
BIN := relay

# -I throughout (C and C++)
INCLUDE_DIRS = src

# Project own files
SRCS = \
   src/modbus.cpp \
   src/relay_ctrl.cpp \
   src/main.cpp \

ifdef SIM
SRCS += \
   src/test_relay.cpp
endif

# Inlude the actual build rules
include $(TOP)/make/rules.mak

# Add dependency to generate the datagram from the config
main.cpp : conf/datagram.hpp