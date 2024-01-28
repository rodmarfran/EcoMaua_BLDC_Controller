/**
 * @file libWheelUniEncoder.ino
 * @brief Implementação da biblioteca para encoder unidirecional montado em roda.
 * @details Esta implementação fornece a lógica necessária para lidar com um encoder unidirecional montado em roda usando interrupções em uma plataforma Arduino.
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2023-07-20
 * @license MIT
 */

#include "libWheelUniEncoder.h"

/**
 * @brief Construtor
 * @param CU8_ENCODER_PIN Pino de entrada do encoder
 * @param CI8_INTERRUPT_MODE Modo da interrupção do encoder
 * @param CF_PULSES_PER_REV Pulsos por revolução do encoder
 * @param CF_WHEEL_RADIUS_M Raio da roda (em metros)
 */
CWheelUniEncoder::CWheelUniEncoder(const uint8_t CU8_ENCODER_PIN, const int8_t CI8_INTERRUPT_MODE, const float CF_PULSES_PER_REV, const float CF_WHEEL_RADIUS_M)
  : _CF_WHEEL_RADIUS_M(CF_WHEEL_RADIUS_M), xUniDirEncoder(CU8_ENCODER_PIN, CI8_INTERRUPT_MODE, CF_PULSES_PER_REV) {}

/**
 * @brief Construtor de cópia
 * @param CX_OTHER Outro objeto da classe CWheelUniEncoder
 */
CWheelUniEncoder::CWheelUniEncoder(const CWheelUniEncoder& CX_OTHER)
  : _CF_WHEEL_RADIUS_M(CX_OTHER._CF_WHEEL_RADIUS_M), xUniDirEncoder(CX_OTHER.xUniDirEncoder) {}

/**
 * @brief Destrutor
 */
CWheelUniEncoder::~CWheelUniEncoder() {
  disableEncoder();
}

/**
 * @brief Inicializa e configura o encoder
 */
void CWheelUniEncoder::begin(void) {
  xUniDirEncoder.begin();
  _fDistanceRevOffset = 0.0f;
  _fMeasureTotalTimeS = 0.0f;
}

/**
 * @brief Habilita o encoder e suas interrupções
 */
void CWheelUniEncoder::enableEncoder(void) {
  xUniDirEncoder.enableEncoder();
}

/**
 * @brief Desabilita o encoder e suas interrupções
 */
void CWheelUniEncoder::disableEncoder(void) {
  xUniDirEncoder.disableEncoder();
}

/**
 * @brief Limpa a distância atual percorrida pela roda
 */
inline void CWheelUniEncoder::clearWhellDistance(void) {
  _fDistanceRevOffset = xUniDirEncoder.fGetRevolutionCount();
  _fMeasureTotalTimeS = 0.0f;
}

/**
 * @brief Retorna a distância percorrida pela roda em m (metros)
 * @return Distância atual percorrida pela roda em m (metros)
 */
inline float CWheelUniEncoder::fGetWhellDistanceM(void) const {
  float fDistanceM;
  
  fDistanceM = (xUniDirEncoder.fGetRevolutionCount() - _fDistanceRevOffset) * (TWO_PI * _CF_WHEEL_RADIUS_M);
  
  return (fDistanceM);
}

/**
 * @brief Retorna a distância percorrida pela roda em Km (quilômetros)
 * @return Distância atual percorrida pela roda (em quilômetros)
 */
inline float CWheelUniEncoder::fGetWhellDistanceKm(void) const {
  float fDistanceKm;
  
  fDistanceKm = (xUniDirEncoder.fGetRevolutionCount() - _fDistanceRevOffset) * (TWO_PI * _CF_WHEEL_RADIUS_M / 1000.0f);
  
  return (fDistanceKm);
}

/**
 * @brief Retorna a velocidade da roda em m/s (metros por segundo)
 * @param fDeltaTimeS Intervalo de tempo em segundos
 * @param bUpdateLastCnt Atualizar a contagem (padrão: true)
 * @return Velocidade da roda em metros por segundo
 */
inline float CWheelUniEncoder::fGetWhellSpeedMS(float fDeltaTimeS, bool bUpdateLastCnt) {
  float fWhellSpeedMS = 0.0f;
  
  fWhellSpeedMS = xUniDirEncoder.fGetSpeedRadS(fDeltaTimeS, bUpdateLastCnt) * _CF_WHEEL_RADIUS_M;

  if (bUpdateLastCnt == true) {
    _fMeasureTotalTimeS += fDeltaTimeS;
  }

  return (fWhellSpeedMS);
}

/**
 * @brief Retorna a velocidade média da roda em m/s (metros por segundo)
 * @return Velocidade média da roda em metros por segundo
 */
inline float CWheelUniEncoder::fGetWhellAvgSpeedMS(void) const {
  float fAvgSpeedMS;
  
  fAvgSpeedMS = fGetWhellDistanceM() / _fMeasureTotalTimeS;
  
  return (fAvgSpeedMS);
}

/**
 * @brief Retorna a velocidade da roda em km/h (quilômetros por hora)
 * @param fDeltaTimeS Intervalo de tempo em segundos
 * @param bUpdateLastCnt Atualizar a contagem (padrão: true)
 * @return Velocidade da roda em quilômetros por hora
 */
inline float CWheelUniEncoder::fGetWhellSpeedKmH(float fDeltaTimeS, bool bUpdateLastCnt) {
  float fWhellSpeedKmH = 0.0f;
  
  fWhellSpeedKmH = xUniDirEncoder.fGetSpeedRadS(fDeltaTimeS, bUpdateLastCnt) * (_CF_WHEEL_RADIUS_M * 3.6f);
  
  if (bUpdateLastCnt == true) {
    _fMeasureTotalTimeS += fDeltaTimeS;
  }

  return (fWhellSpeedKmH);
}

/**
 * @brief Retorna a velocidade média da roda em km/h (quilômetros por hora)
 * @return Velocidade média da roda em quilômetros por hora
 */
inline float CWheelUniEncoder::fGetWhellAvgSpeedKmH(void) const {
  float fAvgSpeedKmH;
  
  fAvgSpeedKmH = fGetWhellDistanceM() / _fMeasureTotalTimeS * 3.6f;
  
  return (fAvgSpeedKmH);
}