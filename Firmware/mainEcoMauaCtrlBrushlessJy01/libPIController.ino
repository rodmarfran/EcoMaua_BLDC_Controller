/**
 * @file libPIController.ino
 * @brief Implementação da biblioteca para um controlador PI.
 * @details Essa implementação inclui as funções para cálculo do controle PI.
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2023-07-21
 * @license MIT License
 */

#include "libPIController.h"

/**
 * @brief Construtor
 * @param CF_PROPORTIONAL_GAIN Ganho proporcional do controlador PI
 * @param CF_INTEGRAL_GAIN Ganho integral do controlador PI
 * @param CF_ITERATION_TIME_S Tempo de iteração do controlador em segundos
 * @param CF_MINIMUM_OUTPUT Valor mínimo que a saída do controlador pode assumir (padrão 0.0)
 * @param CF_MAXIMUM_OUTPUT Valor máximo que a saída do controlador pode assumir (padrão 1.0)
 * @param CF_CHANGE_RATE_PERCENT Taxa de variação percentual máxima do controle (padrão 1.0)
 * @param CB_ANTI_WINDUP_ENABLE Habilita ou desabilita o mecanismo anti-windup (padrão true)
 */
CPIController::CPIController(const float CF_PROPORTIONAL_GAIN, const float CF_INTEGRAL_GAIN, const float CF_ITERATION_TIME_S, const float CF_MINIMUM_OUTPUT, const float CF_MAXIMUM_OUTPUT, const float CF_CHANGE_RATE_PERCENT, const bool CB_ANTI_WINDUP_ENABLE)
  : _CF_PROPORTIONAL_GAIN(CF_PROPORTIONAL_GAIN), _CF_INTEGRAL_GAIN(CF_INTEGRAL_GAIN), _CF_MINIMUM_OUTPUT(CF_MINIMUM_OUTPUT), _CF_CHANGE_RATE_VALUE(CF_CHANGE_RATE_PERCENT * (CF_MAXIMUM_OUTPUT - CF_MINIMUM_OUTPUT)),
    _CF_MAXIMUM_OUTPUT(CF_MAXIMUM_OUTPUT), _CF_ITERATION_TIME_S(CF_ITERATION_TIME_S), _CB_ANTI_WINDUP_ENABLE(CB_ANTI_WINDUP_ENABLE) {}

/**
 * @brief Construtor de cópia
 * @param CX_OTHER Outro objeto da classe CPIController
 */
CPIController::CPIController(const CPIController& CX_OTHER)
  : _CF_PROPORTIONAL_GAIN(CX_OTHER._CF_PROPORTIONAL_GAIN), _CF_INTEGRAL_GAIN(CX_OTHER._CF_INTEGRAL_GAIN), _CF_MINIMUM_OUTPUT(CX_OTHER._CF_MINIMUM_OUTPUT), _CF_CHANGE_RATE_VALUE(CX_OTHER._CF_CHANGE_RATE_VALUE),  
    _CF_MAXIMUM_OUTPUT(CX_OTHER._CF_MAXIMUM_OUTPUT), _CF_ITERATION_TIME_S(CX_OTHER._CF_ITERATION_TIME_S), _CB_ANTI_WINDUP_ENABLE(CX_OTHER._CB_ANTI_WINDUP_ENABLE) {}

/**
 * @brief Destrutor
 */
CPIController::~CPIController() {}

/**
 * @brief Inicia o controlador PI
 */
void CPIController::begin(void) {
  _fIntegratedError = 0.0f;
  _fPreviousError = 0.0f;
  _fPreviousOutput = 0.0f;
}

/**
 * @brief Reinicia o controlador PI
 */
void CPIController::resetController(void) {
  _fIntegratedError = 0.0f;
  _fPreviousError = 0.0f;
  _fPreviousOutput = 0.0f;
}

/**
 * @brief Configura uma saida de controle inicial
 * @param fCtrlOutput O valor de saída que deve ser configurado
 */
void CPIController::setInitialCtrlOutput(float fCtrlOutput) {
  _fPreviousOutput = fCtrlOutput;
}

/**
 * @brief Calcula a saída do controlador PI
 * @param fCtrlSetpoint O valor de referência para o controlador
 * @param fCurrentValue O valor atual que está sendo controlado
 * @return A saída do controlador PI
 */
float CPIController::fCalculateController(float fCtrlSetpoint, float fCurrentValue) {
  float fCurrentError = 0.0f;
  float fNewIntegratedError = 0.0f;
  float fCtrlPOutput = 0.0f;
  float fCtrlIOutput = 0.0f;
  float fCtrlOutput = 0.0f;
  bool bCtrlSaturated = false;

  /* Calcula o erro atual */
  fCurrentError = fCtrlSetpoint - fCurrentValue;

  /* Calcula a saída P do controlador */
  fCtrlPOutput = _CF_PROPORTIONAL_GAIN * fCurrentError;

  /* Calcula a saída I do controlador */
  fNewIntegratedError = _fIntegratedError + _CF_ITERATION_TIME_S * (fCurrentError + _fPreviousError) / 2.0f;
  fCtrlIOutput = _CF_INTEGRAL_GAIN * _fIntegratedError;

  /* Calcula a saída final do controlador PI */
  fCtrlOutput = fCtrlPOutput + fCtrlIOutput;

  /* Saturação da sáida */
  if (fCtrlOutput > _CF_MAXIMUM_OUTPUT) {
    fCtrlOutput = _CF_MAXIMUM_OUTPUT;
    bCtrlSaturated = true;
  } else if (fCtrlOutput < _CF_MINIMUM_OUTPUT) {
    fCtrlOutput = _CF_MINIMUM_OUTPUT;
    bCtrlSaturated = true;
  }

  /* Aplica a taxa máxima de variação do controle */
  float fCtrlDiff = fCtrlOutput - _fPreviousOutput;
  if (fCtrlDiff > _CF_CHANGE_RATE_VALUE) {
    fCtrlOutput = _fPreviousOutput + _CF_CHANGE_RATE_VALUE;
    bCtrlSaturated = true;
  } else if (fCtrlDiff < (-_CF_CHANGE_RATE_VALUE)) {
    fCtrlOutput = _fPreviousOutput - _CF_CHANGE_RATE_VALUE;
    bCtrlSaturated = true;
  }

  /* Mecanismo de Anti-windup */
  if ((_CB_ANTI_WINDUP_ENABLE == false) || (bCtrlSaturated == false)) {
    _fIntegratedError = fNewIntegratedError;
  }

  /* Atualiza o erro anterior com o valor atual */
  _fPreviousError = fCurrentError;

  /* Atualiza a saída anterior com o valor atual */
  _fPreviousOutput = fCtrlOutput;

  return (fCtrlOutput);
}