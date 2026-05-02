/*
 *******************************************************************************
 * UIAPduino SD+WebHID variant
 * Based on CH32V003F4 -- SPI/UART enabled for SD card + WebHID debug
 *******************************************************************************
 */
#pragma once

/* ENABLE Peripherals */
#define                         ADC_MODULE_ENABLED
#define                         UART_MODULE_ENABLED
#define                         SPI_MODULE_ENABLED
// #define                      I2C_MODULE_ENABLED
#define                         TIM_MODULE_ENABLED

/* CH32V003F4 Pin definitions */
#define PA1                     PIN_A1
#define PA2                     PIN_A0
#define PC0                     2
#define PC1                     3
#define PC2                     4
#define PC3                     5
#define PC4                     PIN_A2
#define PC5                     7
#define PC6                     8
#define PC7                     9
#define PD0                     10
#define PD1                     11
#define PD2                     PIN_A3
#define PD3                     PIN_A4
#define PD4                     PIN_A7
#define PD5                     PIN_A5
#define PD6                     PIN_A6
#define PD7                     17

// Alternate pins number
#define PD5_ALT1                (PD5  | ALT1)
#define PD6_ALT1                (PD6  | ALT1)

#define NUM_DIGITAL_PINS        18
#define NUM_ANALOG_INPUTS       8

// GPIO_PIN_N aliases
#define GPIO_PIN_0              PA1   // D0
#define GPIO_PIN_1              PA2   // D1
#define GPIO_PIN_2              PC0   // D2
#define GPIO_PIN_3              PC1   // D3  (SPI SS)
#define GPIO_PIN_4              PC2   // D4
#define GPIO_PIN_5              PC3   // D5
#define GPIO_PIN_6              PC4   // D6 / A2
#define GPIO_PIN_7              PC5   // D7  (SPI SCK)
#define GPIO_PIN_8              PC6   // D8  (SPI MOSI)
#define GPIO_PIN_9              PC7   // D9  (SPI MISO)
#define GPIO_PIN_10             PD0   // D10
#define GPIO_PIN_11             PD1   // D11 (SWIO)
#define GPIO_PIN_12             PD2   // D12 / A3
#define GPIO_PIN_13             PD3   // D13 / A4  (USB D+)
#define GPIO_PIN_14             PD4   // D14 / A7  (USB D-)
#define GPIO_PIN_15             PD5   // D15 / A5  (UART TX)
#define GPIO_PIN_16             PD6   // D16 / A6  (UART RX)
#define GPIO_PIN_17             PD7   // D17       (RESET)

#define ADC_RESOLUTION          10

// On-board LED
#ifndef LED_BUILTIN
  #define LED_BUILTIN           PC0
#endif

// On-board user button
#ifndef USER_BTN
  #define USER_BTN              PNUM_NOT_DEFINED
#endif

// SPI defaults
#define PIN_SPI_SS              PC1
#define PIN_SPI_MOSI            PC6
#define PIN_SPI_MISO            PC7
#define PIN_SPI_SCK             PC5

// UART defaults (UART1)
#define PIN_SERIAL_TX           PD5
#define PIN_SERIAL_RX           PD6

// Timer Definitions
#ifndef TIMER_TONE
  #define TIMER_TONE            TIM2
#endif
#ifndef TIMER_SERVO
  #define TIMER_SERVO           TIM1
#endif

#ifdef __cplusplus
  #ifndef SERIAL_PORT_MONITOR
    #define SERIAL_PORT_MONITOR   Serial
  #endif
  #ifndef SERIAL_PORT_HARDWARE
    #define SERIAL_PORT_HARDWARE  Serial
  #endif
#endif
