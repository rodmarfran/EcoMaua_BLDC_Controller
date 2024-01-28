/**
 * @file incSystemScheduler.h
 * @brief Include com a definição do agendador do sustema para o projeto do controlador JY01 (equipe EcoMauá).
 * @details Este arquivo contém a definição do agendador do sistema para serem usados em no projeto do controlador JY01 (equipe EcoMauá).
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2023-07-21
 * @license MIT License
 */

#ifndef INCSYSTEMSCHEDULER_H
#define INCSYSTEMSCHEDULER_H

#include <Arduino.h>

/**
 * @class CSystemScheduler
 * @brief Classe para definição do agendador do sistema para o projeto do controlador JY01 (equipe EcoMauá)
 */
class CSystemScheduler {
public:

  /**
   * @brief Definição do tipo para enumeração das tarefas do sistema
   */
  typedef enum SystemTasks {
    E_ENERGY_MEASURE_TASK = 0, /* Tarefa de medição de energia */
    E_MOTOR_CONTROL_TASK,      /* Tarefa de controle do motor */
    E_TELEMETRY_TASK,          /* Tarefa de telemetria */
    E_TASKS_COUNT
  } TSystemTasks;

  static constexpr uint32_t _U32_TASK_EXEC_TIME_MS[E_TASKS_COUNT] = { /**< Tempo de execução das tarefas em (milisegundos) */
    50ul,  /* Tarefa de medição de energia */
    100ul, /* Tarefa de controle do motor */
    1000ul /* Tarefa de telemetria */
  };

  /**
   * @brief Habilita ou desabilita uma tarefa
   * @param bTaskEnable True para habilitar a tarefa e False para desabilitar
   * @param eSystemTask Tarefa do sistema a ser verificada
   */
  inline void setTaskEnable(bool bTaskEnable, TSystemTasks eSystemTask) {
    _bTaskEnable[eSystemTask] = bTaskEnable;
    _u32LastTaskExecTimeMs[eSystemTask] = millis();
  }

  /**
   * @brief Retorna o flag de execução de uma tarefa do sistema
   * @param eSystemTask Tarefa do sistema a ser verificada
   * @return Verdadeiro se é o momento de executar a tarefa do sistema, Falso se não passou ainda o tempo necessário de execução
   */
  inline bool bGetTaskExecFlag(TSystemTasks eSystemTask) {
    bool bTaskExecutionFlag = false;

    uint32_t u32CurrentTimeMs = millis();
    if ((_bTaskEnable[eSystemTask] == true) && (_U32_TASK_EXEC_TIME_MS[eSystemTask] <= static_cast<uint32_t>(u32CurrentTimeMs - _u32LastTaskExecTimeMs[eSystemTask]))) {
      _u32LastTaskExecTimeMs[eSystemTask] = u32CurrentTimeMs;
      bTaskExecutionFlag = true;
    }

    return (bTaskExecutionFlag);
  }

private:

  uint32_t _u32LastTaskExecTimeMs[E_TASKS_COUNT];                     /**< Último tempo de execução das tarefas (em milisegundos) */
  bool _bTaskEnable[E_TASKS_COUNT];                                   /**< Flag de tarefa habilitada para ativar e desativar as tarefas */

};

#endif