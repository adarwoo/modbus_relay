#pragma once
/**
 * Defines all I/Os of the board using Pin
 * Compatible with C and C++ use (ioport.h or asx/ioport.hpp)
 * In C++, the namespace asx::ioport must be used
 */

/************************************************************************/
/* Debug pins                                                           */
/* Pin DEBUG_REACTOR_IDLE is available on the uPDI connector on Pin 3   */
/* UART0 TxD is available on the uPDI connector on Pin 5                */
/************************************************************************/
#undef  DEBUG_REACTOR_IDLE          // No pin for this
#define DEBUG_REACTOR_BUSY          IOPORT(A, 2)
#define UART0_TXD                   IOPORT(A, 3)

/************************************************************************/
/* Alert pin                                                            */
/* This is connected to the LED_FAULT and shared for other purposes     */
/************************************************************************/
#define ALERT_OUTPUT_PIN            IOPORT(A, 5)
#define ES_COMMAND                  IOPORT(A, 6)

/************************************************************************/
/* Modbus LEDs                                                          */
/* Tx LED is driven by the UART XDIR Pin directly                       */
/************************************************************************/
#define LED_MODBUS_RX               IOPORT(A, 3)

/************************************************************************/
/* Relay I/Os                                                           */
/************************************************************************/
#define LED_A                       IOPORT(B, 5)
#define LED_B                       IOPORT(B, 6)
#define LED_C                       IOPORT(A, 7)

#define RELAY_A                     IOPORT(C, 0)
#define RELAY_B                     IOPORT(C, 1)
#define RELAY_C                     IOPORT(C, 2)

#define CHECK_A                     IOPORT(C, 3)
#define CHECK_B                     IOPORT(C, 4)
#define CHECK_C                     IOPORT(C, 5)

/************************************************************************/
/* In-Feed sensing                                                      */
/************************************************************************/
#define INFEED_DIAG                 IOPORT(A, 7)
#define INFEED_LED                  IOPORT(B, 4)

/************************************************************************/
/* Push button                                                          */
/************************************************************************/
#define PUSH_BUTTON                 IOPORT(B, 3)
