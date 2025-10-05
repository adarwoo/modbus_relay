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
#define UART0_TXD                   IOPORT(B, 2)
#define UART1_TXD                   IOPORT(A, 1)
#define UART1_XDIR                  IOPORT(A, 4)


/************************************************************************/
/* Alert pin                                                            */
/* This is connected to the LED_FAULT and shared for other purposes     */
/************************************************************************/
#define ES_COMMAND                  IOPORT(B, 1)
#define ALERT_OUTPUT_PIN            IOPORT(B, 0)

/************************************************************************/
/* Modbus LEDs                                                          */
/************************************************************************/

// The Rx LED is driven by the TimerB1
#define LED_MODBUS                  IOPORT(A, 3)

/************************************************************************/
/* Relay I/Os                                                           */
/************************************************************************/
#define LED_A                       IOPORT(B, 5)
#define LED_B                       IOPORT(B, 6)
#define LED_C                       IOPORT(B, 7)

#define RELAY_A                     IOPORT(C, 0)
#define RELAY_B                     IOPORT(C, 1)
#define RELAY_C                     IOPORT(C, 2)

#define CHECK_A                     IOPORT(C, 3)
#define CHECK_B                     IOPORT(C, 4)
#define CHECK_C                     IOPORT(C, 5)

/************************************************************************/
/* In-Feed sensing                                                      */
/************************************************************************/
#undef INFEED_DIAG                  // No pin for this
#define INFEED_LED                  IOPORT(B, 4)

/************************************************************************/
/* Push button                                                          */
/************************************************************************/
#define PUSH_BUTTON                 IOPORT(B, 3)
