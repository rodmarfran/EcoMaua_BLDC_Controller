/**
 * @file libPIController.h
 * @brief Biblioteca para um controlador PI.
 * @details Essa biblioteca permite a implementação de um controlador PI com anti-windup e saturação na saída.
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2023-07-21
 * @license MIT License
 */

#ifndef LIBPICONTROLLER_H
#define LIBPICONTROLLER_H

#include <Arduino.h>

/**
 * @class CPIController
 * @brief Classe para definição de um controlador PI com anti-windup e saturação na saída.
 */
class CPIController {
public:

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
  explicit CPIController(const float CF_PROPORTIONAL_GAIN, const float CF_INTEGRAL_GAIN, const float CF_ITERATION_TIME_S, const float CF_MINIMUM_OUTPUT = 0.0f, const float CF_MAXIMUM_OUTPUT = 1.0f, const float CF_CHANGE_RATE_PERCENT = 1.0f, const bool CB_ANTI_WINDUP_ENABLE = true);

  /**
   * @brief Construtor de cópia
   * @param CX_OTHER Outro objeto da classe CPIController
   */
  CPIController(const CPIController& CX_OTHER);

  /**
   * @brief Destrutor
   */
  ~CPIController();

  /**
   * @brief Inicia o controlador PI
   */
  void begin(void);

  /**
   * @brief Reinicia o controlador PI
   */
  void resetController(void);

  /**
   * @brief Configura uma saida de controle inicial
   * @param fCtrlOutput O valor de saída que deve ser configurado
   */
  void setInitialCtrlOutput(float fCtrlOutput);

  /**
   * @brief Calcula a saída do controlador PI
   * @param fCtrlSetpoint O valor de referência para o controlador
   * @param fCurrentValue O valor atual que está sendo controlado
   * @return A saída do controlador PI
   */
  float fCalculateController(float fCtrlSetpoint, float fCurrentValue);

private:

  const float _CF_PROPORTIONAL_GAIN; /**< Ganho proporcional do controlador PI */
  const float _CF_INTEGRAL_GAIN;     /**< Ganho integral do controlador PI */
  const float _CF_MINIMUM_OUTPUT;    /**< Saída mínima permitida para o controlador */
  const float _CF_MAXIMUM_OUTPUT;    /**< Saída máxima permitida para o controlador */
  const float _CF_CHANGE_RATE_VALUE; /**< Variação máxima do controle */
  const float _CF_ITERATION_TIME_S;  /**< Tempo de iteração em segundos */
  const bool _CB_ANTI_WINDUP_ENABLE; /**< Flag para habilitar ou desabilitar o mecanismo de anti-windup */
  float _fIntegratedError;           /**< Erro integrado acumulado */
  float _fPreviousError;             /**< Valor do erro na iteração anterior */
  float _fPreviousOutput;            /**< Valor do saída na iteração anterior */

};

#endif
