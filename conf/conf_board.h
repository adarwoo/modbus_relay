#pragma once
/**
 * Defines all I/Os of the board using PinDef
 * Compatible with C and C++ use (ioport.h or asx/ioport.hpp)
 * In C++, the namespace asx::ioport must be used
 */

/************************************************************************/
/* Debug pins                                                           */
/* Pin DEBUG_REACTOR_IDLE is available on the uPDI connector on Pin 3   */
/* UART0 TxD is available on the uPDI connector on Pin 5                */
/************************************************************************/
#undef  DEBUG_REACTOR_IDLE          // No pin for this
#define DEBUG_REACTOR_BUSY          PinDef(A, 2)
#define UART0_TXD                   PinDef(A, 3)

/************************************************************************/
/* Alert pin                                                            */
/* This is connected to the LED_FAULT and shared for other purposes     */
/************************************************************************/
#define ALERT_OUTPUT_PIN            PinDef(A, 5)
#define ES_COMMAND                  PinDef(A, 6)

/************************************************************************/
/* Modbus LEDs                                                          */
/* Tx LED is driven by the UART XDIR Pin directly                       */
/************************************************************************/
#define LED_MODBUS_RX               PinDef(A, 3)

/************************************************************************/
/* Relay I/Os                                                           */
/************************************************************************/
#define LED_A                       PinDef(B, 5)
#define LED_B                       PinDef(B, 6)
#define LED_C                       PinDef(A, 7)

#define RELAY_A                     PinDef(C, 0)
#define RELAY_B                     PinDef(C, 1)
#define RELAY_C                     PinDef(C, 2)

#define CHECK_A                     PinDef(C, 3)
#define CHECK_B                     PinDef(C, 4)
#define CHECK_C                     PinDef(C, 5)

/************************************************************************/
/* In-Feed sensing                                                      */
/************************************************************************/
#define INFEED_DIAG                 PinDef(A, 7)
#define INFEED_LED                  PinDef(B, 4)

/************************************************************************/
/* Push button                                                          */
/************************************************************************/
#define PUSH_BUTTON                PinDef(B, 3)
