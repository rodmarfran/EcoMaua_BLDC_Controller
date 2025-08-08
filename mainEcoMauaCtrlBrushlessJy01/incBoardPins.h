/**
 * @file incBoardPins.h
 * @brief Include com a definição dos pinos do Arduino Nano para o projeto do controlador JY01 (equipe EcoMauá).
 * @details Este arquivo contém a definição dos pinos do Arduino Nano para serem usados em no projeto do controlador JY01 (equipe EcoMauá), 
 *          incluindo os pinos de controle JY01, pino analógico do acelerador e pinos I2C.
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2023-04-08
 * @license MIT License
 */

#ifndef INCBOARDPINS_H
#define INCBOARDPINS_H

#include <Arduino.h>

/**
 * @class CBoardPins
 * @brief Classe para definição dos pinos do Arduino Nano para o projeto do controlador JY01 (equipe EcoMauá)
 */
class CBoardPins {
public:
  static const uint8_t CU8_ENC_0_DI_IRQ_PIN             = 2u;  /**< Pino do encoder de velocidade 0 (roda) */
  static const uint8_t CU8_JY01_CTRL_M_DI_IRQ_PIN       = 3u;  /**< Pino do encoder de velocidade do controlador JY01 */
  static const uint8_t CU8_JY01_CTRL_ZF_DO_PIN          = 4u;  /**< Pino de direção do controlador JY01 */
  static const uint8_t CU8_JY01_CTRL_EL_DO_PIN          = 5u;  /**< Pino de enable do controlador JY01 */
  static const uint8_t CU8_LED_0_BLUE_DO_PIN            = 6u;  /**< Pino do LED 0 (azul) */
  static const uint8_t CU8_LED_1_RED_DO_PIN             = 7u;  /**< Pino de LED 1 (vermelho) */
  static const uint8_t CU8_EXT_UART_RX_PIN              = 8u;  /**< Pino de Tx do UART Externo */
  static const uint8_t CU8_EXT_UART_TX_PIN              = 9u;  /**< Pino de Rx do UART Externo */
  static const uint8_t CU8_MCP2515_CS_DO_PIN            = 8u;  /**< Pino de Chip Select do modulo CAN MCP2515 */
  static const uint8_t CU8_MCP2515_INT_DI_IRQ_PIN       = 9u;  /**< Pino de Interrupção do modulo CAN MCP2515 */
  static const uint8_t CU8_COUPLING_MOTOR_ENGAGE_DO_PIN = 9u;  /**< Pino de controle do motor de acoplamento */
  static const uint8_t CU8_ARDUINO_SPI_MOSI_PIN         = 11u; /**< Pino de SPI MOSI do Arduino Nano */
  static const uint8_t CU8_ARDUINO_SPI_MISO_PIN         = 12u; /**< Pino de SPI MISO do Arduino Nano */
  static const uint8_t CU8_ARDUINO_SPI_SCK_PIN          = 13u; /**< Pino de SPI SCK do Arduino Nano */
  static const uint8_t CU8_THROTTLE_AN_PIN              = A0;  /**< Pino analógico do acelerador */
  static const uint8_t CU8_BTN_0_DI_PIN                 = A1;  /**< Pino do Botão/Chave 0 */
  static const uint8_t CU8_BTN_1_DI_PIN                 = A2;  /**< Pino do Botão/Chave 1 */
  static const uint8_t CU8_BTN_2_DI_PIN                 = A3;  /**< Pino do Botão/Chave 2 */
  static const uint8_t CU8_ARDUNO_I2C_SDA_PIN           = A4;  /**< Pino I2C SDA do Arduino Nano */
  static const uint8_t CU8_ARDUNO_I2C_SCL_PIN           = A5;  /**< Pino I2C SCL do Arduino Nano */
  static const uint8_t CU8_VIN_SENSE_AN_PIN             = A6;  /**< Pino analógico da medição de tensão de entrada */
  static const uint8_t CU8_IM_SENSE_AN_PIN              = A7;  /**< Pino analógico da medição de corrente do motor */
};

#endif