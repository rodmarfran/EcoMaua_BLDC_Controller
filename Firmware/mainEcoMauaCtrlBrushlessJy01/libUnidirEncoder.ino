/**
 * @file libUnidirEncoder.ino
 * @brief Implementação da biblioteca para encoder unidirecional.
 * @details Esta implementação fornece a lógica necessária para lidar com um encoder unidirecional usando interrupções em uma plataforma Arduino.
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2023-04-06
 * @license MIT
 */

#include "libUnidirEncoder.h"

/**
 * @brief Inicializa a instância única da classe como nullptr
 */
CUnidirEncoder* CUnidirEncoder::_pxInstance = nullptr;

/**
 * @brief Construtor
 * @param CU8_ENCODER_PIN Pino de entrada do encoder
 * @param CI8_INTERRUPT_MODE Modo da interrupção do encoder
 * @param CF_PULSES_PER_REV Pulsos por revolução do encoder
 */
CUnidirEncoder::CUnidirEncoder(const uint8_t CU8_ENCODER_PIN, const int8_t CI8_INTERRUPT_MODE, const float CF_PULSES_PER_REV)
  : _CU8_ENCODER_PIN(CU8_ENCODER_PIN), _CI8_INTERRUPT_MODE(CI8_INTERRUPT_MODE), _CF_PULSES_PER_REV(CF_PULSES_PER_REV) {}

/**
 * @brief Construtor de cópia
 * @param CX_OTHER Outro objeto da classe CUnidirEncoder
 */
CUnidirEncoder::CUnidirEncoder(const CUnidirEncoder& CX_OTHER)
  : _CU8_ENCODER_PIN(CX_OTHER._CU8_ENCODER_PIN), _CI8_INTERRUPT_MODE(CX_OTHER._CI8_INTERRUPT_MODE), _CF_PULSES_PER_REV(CX_OTHER._CF_PULSES_PER_REV) {}

/**
 * @brief Destrutor
 */
CUnidirEncoder::~CUnidirEncoder() {
  disableEncoder();
  _pxInstance = nullptr;
}

/**
 * @brief Inicializa e configura o encoder
 */
void CUnidirEncoder::begin(void) {
  pinMode(_CU8_ENCODER_PIN, INPUT);
  enableEncoder();
  _pxInstance = this;
}

/**
 * @brief Habilita o encoder e suas interrupções
 */
void CUnidirEncoder::enableEncoder(void) {
  _clearEncPulsesCnts();
  attachInterrupt(digitalPinToInterrupt(_CU8_ENCODER_PIN), _encPulsesIsr, _CI8_INTERRUPT_MODE);
}

/**
 * @brief Desabilita o encoder e suas interrupções
 */
void CUnidirEncoder::disableEncoder(void) {
  detachInterrupt(digitalPinToInterrupt(_CU8_ENCODER_PIN));
  _clearEncPulsesCnts();
}

/**
 * @brief Retorna a contagem atual de pulsos do encoder
 * @return Contagem atual de pulsos do encoder
 */
inline uint16_t CUnidirEncoder::u16GetEncPulsesCnt(void) const {
  return (_vu16EncPulsesCnt);
}

/**
 * @brief Limpa as contagens de pulsos do encoder
 */
void CUnidirEncoder::_clearEncPulsesCnts(void) {
  _vu16EncPulsesCnt = 0;
  _vu16EncPulsesLastCnt = 0;
}

/**
 * @brief Retorna a diferença de pulsos desde a última chamada (opcionalmente atualiza a contagem)
 * @param bUpdateLastCnt Atualizar a contagem (padrão: true)
 * @return Diferença de pulsos desde a última chamada
 */
uint16_t CUnidirEncoder::u16GetEncPulsesDelta(bool bUpdateLastCnt) const {
  uint16_t u16EncPulses;
  uint16_t u16EncPulsesDelta;

  u16EncPulses = _vu16EncPulsesCnt;
  u16EncPulsesDelta = static_cast<uint16_t>(u16EncPulses - _vu16EncPulsesLastCnt);

  if (bUpdateLastCnt == true) {
    _vu16EncPulsesLastCnt = u16EncPulses;
  }

  return (u16EncPulsesDelta);
}

/**
 * @brief Retorna a contagem de revoluções
 * @return Contagem de revoluções
 */
inline float CUnidirEncoder::fGetRevolutionCount(void) const {
  float fRevCount;

  fRevCount = static_cast<float>(_vu16EncPulsesCnt) / _CF_PULSES_PER_REV;

  return (fRevCount);
}

/**
 * @brief Retorna o ângulo em radianos
 * @return Ângulo em radianos
 */
inline float CUnidirEncoder::fGetAngleRad(void) const {
  float fAngleRad;

  fAngleRad = static_cast<float>(_vu16EncPulsesCnt) * (TWO_PI / _CF_PULSES_PER_REV);

  return (fAngleRad);
}

/**
 * @brief Retorna a velocidade em RPM (rotações por minuto)
 * @param fDeltaTimeS Intervalo de tempo em segundos
 * @param bUpdateLastCnt Atualizar a contagem (padrão: true)
 * @return Velocidade em RPM (rotações por minuto)
 */
float CUnidirEncoder::fGetSpeedRpm(float fDeltaTimeS, bool bUpdateLastCnt) const {
  uint16_t u16EncPulsesDelta;
  float fSpeedRpm = 0.0f;

  u16EncPulsesDelta = u16GetEncPulsesDelta(bUpdateLastCnt);

  if (fDeltaTimeS > 0.0f) {
    fSpeedRpm = (static_cast<float>(u16EncPulsesDelta) / fDeltaTimeS) * (60.0f / _CF_PULSES_PER_REV);
  }

  return (fSpeedRpm);
}

/**
 * @brief Retorna a velocidade em radianos por segundo
 * @param fDeltaTimeS Intervalo de tempo em segundos
 * @param bUpdateLastCnt Atualizar a contagem (padrão: true)
 * @return Velocidade em radianos por segundo
 */
float CUnidirEncoder::fGetSpeedRadS(float fDeltaTimeS, bool bUpdateLastCnt) const {
  uint16_t u16EncPulsesDelta;
  float fSpeedRadS = 0.0f;

  u16EncPulsesDelta = u16GetEncPulsesDelta(bUpdateLastCnt);
  if (fDeltaTimeS > 0.0f) {
    fSpeedRadS = (static_cast<float>(u16EncPulsesDelta) / fDeltaTimeS) * (TWO_PI / _CF_PULSES_PER_REV);
  }

  return (fSpeedRadS);
}

/**
 * @brief Função de interrupção para pulsos do encoder
 */
void CUnidirEncoder::_encPulsesIsr(void) {
  CUnidirEncoder::_pxInstance->_vu16EncPulsesCnt++;
}
