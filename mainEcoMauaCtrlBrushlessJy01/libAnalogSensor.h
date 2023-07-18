/**
 * @file libAnalogSensor.h
 * @brief Biblioteca para leitura de sensores analógicos.
 * @details Esta biblioteca permite ler e processar sinais de sensores analógicos, oferecendo métodos para obter leituras brutas, tensão e leituras escalonadas.
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2023-04-02
 * @license MIT License
 */

#ifndef LIBANALOGSENSOR_H
#define LIBANALOGSENSOR_H

#include <Arduino.h>

/**
 * @class CAnalogSensor
 * @brief Classe para ler valores de sensores analógicos
 */
class CAnalogSensor {
public:
  /**
   * @brief Construtor
   * @param CU8_ANALOG_PIN Número do pino analógico
   * @param CF_LIN_GRADIENT Gradiente linear para escalonar a tensão (padrão 1.0)
   * @param CF_LIN_INTERCEPT Interceptação linear para escalonar a tensão (padrão 0.0)
   * @param CU8_OUTPUT_BITS Número de bits para a saída (padrão 10)
   * @param CF_VOLTAGE_REFERENCE Tensão de referência (padrão 5.0)
   */
  explicit CAnalogSensor(const uint8_t CU8_ANALOG_PIN, const float CF_LIN_GRADIENT = 1.0f, const float CF_LIN_INTERCEPT = 0.0f, const uint8_t CU8_OUTPUT_BITS = 10u, const float CF_VOLTAGE_REFERENCE = 5.0f);

  /**
   * @brief Construtor de cópia
   * @param CX_OTHER Objeto CAnalogSensor a ser copiado
   */
  CAnalogSensor(const CAnalogSensor& CX_OTHER);

  /**
   * @brief Destrutor
   */
  ~CAnalogSensor();

  /**
   * @brief Inicializa o pino do sensor
   */
  void begin(void);

  /**
   * @brief Retorna a leitura bruta do sensor
   * @return Valor bruto do sensor
   */
  uint16_t u16GetSensorRaw(void) const;

  /**
   * @brief Retorna a tensão do sensor
   * @return Tensão do sensor
   */
  float fGetSensorVoltage(void) const;

  /**
   * @brief Retorna a leitura escalonada do sensor
   * @return Leitura escalonada do sensor
   */
  float fGetSensorScaled(void) const;

  /**
   * @brief Realiza a calibração de auto-zero do sensor
   */
  void sensorAutoZeroCalibration(void);

  /**
   * @brief Retorna a leitura escalonada de auto-zero do sensor
   * @return Leitura escalonada de auto-zero do sensor
   */
  float fGetSensorAutoZeroScaled(void) const;
private:
  static const uint8_t _CU8_DEFAULT_ADC_BITS = 10u; /**< Quantidade padrão da resolução do ADC interno */
  const uint8_t _CU8_ANALOG_PIN;                    /**< Pino analógico do sensor */
  const float _CF_LIN_GRADIENT;                     /**< Gradiente linear para escalar a tensão */
  const float _CF_LIN_INTERCEPT;                    /**< Interceptação linear para escalar a tensão */
  const uint8_t _CU8_DECIMATION_FACTOR;             /**< Fator de decimação para aumentar a resolução da leitura */
  const float _CF_VOLTAGE_REFERENCE;                /**< Tensão de referência do sensor */
  float _fLinAutoZeroIntercept;                     /**< Interceptação linear do Auto-Zero para escalar a tensão */
};

#endif
