/**
 * @file libAnalogSensor.ino
 * @brief Implementação da biblioteca para leitura de sensores analógicos.
 * @details Esta biblioteca permite ler e processar sinais de sensores analógicos, oferecendo métodos para obter leituras brutas, tensão e leituras escalonadas.
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2023-04-02
 * @license MIT License
 */

#include "libAnalogSensor.h"

/**
 * @brief Construtor da classe CAnalogSensor
 */
CAnalogSensor::CAnalogSensor(const uint8_t CU8_ANALOG_PIN, const float CF_LIN_GRADIENT, const float CF_LIN_INTERCEPT, const uint8_t CU8_OUTPUT_BITS, const float CF_VOLTAGE_REFERENCE)
  : _CU8_ANALOG_PIN(CU8_ANALOG_PIN), _CF_LIN_GRADIENT(CF_LIN_GRADIENT), _CF_LIN_INTERCEPT(CF_LIN_INTERCEPT),
    _CU8_DECIMATION_FACTOR(CU8_OUTPUT_BITS > _CU8_DEFAULT_ADC_BITS ? static_cast<uint8_t>(min(CU8_OUTPUT_BITS - _CU8_DEFAULT_ADC_BITS, 6u)) : 0u), _CF_VOLTAGE_REFERENCE(CF_VOLTAGE_REFERENCE) {}

/**
 * @brief Construtor de cópia da classe CAnalogSensor
 */
CAnalogSensor::CAnalogSensor(const CAnalogSensor& CX_OTHER)
  : _CU8_ANALOG_PIN(CX_OTHER._CU8_ANALOG_PIN), _CF_LIN_GRADIENT(CX_OTHER._CF_LIN_GRADIENT), _CF_LIN_INTERCEPT(CX_OTHER._CF_LIN_INTERCEPT),
    _CU8_DECIMATION_FACTOR(CX_OTHER._CU8_DECIMATION_FACTOR), _CF_VOLTAGE_REFERENCE(CX_OTHER._CF_VOLTAGE_REFERENCE) {}

/**
 * @brief Destrutor da classe CAnalogSensor
 */
CAnalogSensor::~CAnalogSensor() {}

/**
 * @brief Inicializa o pino do sensor
 */
void CAnalogSensor::begin(void) {
  pinMode(_CU8_ANALOG_PIN, INPUT);
  _fLinAutoZeroIntercept = _CF_LIN_INTERCEPT;
}

/**
 * @brief Retorna a leitura bruta do sensor
 * @return Valor bruto do sensor
 */
uint16_t CAnalogSensor::u16GetSensorRaw(void) const {
  uint32_t u32AnalogReadAcumulator = 0u;
  uint16_t u16AnalogReadDecimated;
  uint16_t u16AnalogReadCnt;

  /* Caso não haja decimação */
  if (_CU8_DECIMATION_FACTOR == 0u) {
    u16AnalogReadDecimated = static_cast<uint16_t>(analogRead(_CU8_ANALOG_PIN));
  } else {
    /* Caso haja decimação, realiza a leitura várias vezes e acumula */
    for (u16AnalogReadCnt = 0u; u16AnalogReadCnt < (1u << (2u * _CU8_DECIMATION_FACTOR)); u16AnalogReadCnt++) {
      u32AnalogReadAcumulator += static_cast<uint32_t>(analogRead(_CU8_ANALOG_PIN));
    }
    /* Realiza a decimação das leituras acumuladas */
    u16AnalogReadDecimated = static_cast<uint16_t>(u32AnalogReadAcumulator >> _CU8_DECIMATION_FACTOR);
  }
  return (u16AnalogReadDecimated);
}

/**
 * @brief Retorna a tensão do sensor
 * @return Tensão do sensor
 */
float CAnalogSensor::fGetSensorVoltage(void) const {
  uint16_t u16AnalogSensorRaw;
  float fAnalogSensorVoltage;

  /* Obtém a leitura bruta do sensor */
  u16AnalogSensorRaw = u16GetSensorRaw();

  /* Calcula a tensão do sensor com base na leitura bruta */
  fAnalogSensorVoltage = static_cast<float>(u16AnalogSensorRaw) * _CF_VOLTAGE_REFERENCE / static_cast<float>((1u << (_CU8_DEFAULT_ADC_BITS + _CU8_DECIMATION_FACTOR)) - 1u);
  return (fAnalogSensorVoltage);
}

/**
 * @brief Retorna a leitura escalonada do sensor
 * @return Leitura escalonada do sensor
 */
float CAnalogSensor::fGetSensorScaled(void) const {
  float fAnalogSensorVoltage;
  float fAnalogSensorScaled;

  /* Obtém a tensão do sensor */
  fAnalogSensorVoltage = fGetSensorVoltage();

  /* Calcula a leitura escalonada com base na tensão do sensor */
  fAnalogSensorScaled = _CF_LIN_GRADIENT * fAnalogSensorVoltage - _CF_LIN_INTERCEPT;
  return (fAnalogSensorScaled);
}

/**
 * @brief Realiza a calibração de auto-zero do sensor
 */
void CAnalogSensor::sensorAutoZeroCalibration(void) {
  /* Obtém a tensão do sensor e atribuí como tensão de auto-zero */
  _fLinAutoZeroIntercept = fGetSensorVoltage() * _CF_LIN_GRADIENT;
}

/**
 * @brief Retorna a leitura escalonada de auto-zero do sensor
 * @return Leitura escalonada de auto-zero do sensor
 */
float CAnalogSensor::fGetSensorAutoZeroScaled(void) const {
  float fAnalogSensorVoltage;
  float fAnalogSensorScaled;

  /* Obtém a tensão do sensor */
  fAnalogSensorVoltage = fGetSensorVoltage();

  /* Calcula a leitura escalonada de auto-zero com base na tensão do sensor */
  fAnalogSensorScaled = _CF_LIN_GRADIENT * fAnalogSensorVoltage - _fLinAutoZeroIntercept;
  return (fAnalogSensorScaled);
}
