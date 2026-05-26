#include "DC_Motor.h"
#include "main.h"
#include "tim.h"
#include "gpio.h"
#include <math.h>

/**
  * @brief   初始化电机驱动：重配置 TIM2 为 20kHz PWM，默认停止
  */
void DC_Motor_Init(void)
{
    /* 重新配置 TIM2 为 20kHz PWM（适合直流电机驱动） */
    HAL_TIM_PWM_Stop(MOTOR_TIM, MOTOR_PWM_CHANNEL);
    __HAL_TIM_SET_PRESCALER(MOTOR_TIM, MOTOR_TIM_PSC);
    __HAL_TIM_SET_AUTORELOAD(MOTOR_TIM, MOTOR_TIM_ARR);
    TIM2->EGR = TIM_EGR_UG;      /* 强制更新，加载预分频器 */
    __HAL_TIM_SET_COUNTER(MOTOR_TIM, 0);

    HAL_TIM_PWM_Start(MOTOR_TIM, MOTOR_PWM_CHANNEL);
    DC_Motor_SetDirection(MOTOR_DIR_STOP);
    DC_Motor_SetSpeed(0);
}

/**
  * @brief   设置电机转速（PWM 脉冲值）
  * @param   pulse  PWM 比较值
  */
void DC_Motor_SetSpeed(uint16_t pulse)
{
    __HAL_TIM_SET_COMPARE(MOTOR_TIM, MOTOR_PWM_CHANNEL, pulse);
}

/**
  * @brief   设置电机转向
  * @param   dir   MOTOR_DIR_CW / MOTOR_DIR_CCW / MOTOR_DIR_STOP
  */
void DC_Motor_SetDirection(uint8_t dir)
{
    switch (dir)
    {
        case MOTOR_DIR_CW:
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN_IN1, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN_IN2, GPIO_PIN_RESET);
            break;
        case MOTOR_DIR_CCW:
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN_IN1, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN_IN2, GPIO_PIN_SET);
            break;
        case MOTOR_DIR_STOP:
        default:
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN_IN1, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN_IN2, GPIO_PIN_RESET);
            break;
    }
}

/**
  * @brief   根据俯仰角控制电机转向与转速
  * @param   pitch  俯仰角（度），正=前倾，负=后仰
  * @note    角度越大 → 占空比越大 → 转速越快
  */
void DC_Motor_ControlByPitch(float pitch)
{
    float absPitch = fabsf(pitch);

    /* 死区判断：小角度内电机停止，避免频繁启停抖动 */
    if (absPitch < MOTOR_DEAD_ANGLE)
    {
        DC_Motor_SetDirection(MOTOR_DIR_STOP);
        DC_Motor_SetSpeed(0);
        return;
    }

    /* 转向控制 */
    if (pitch > 0)
        DC_Motor_SetDirection(MOTOR_DIR_CW);
    else
        DC_Motor_SetDirection(MOTOR_DIR_CCW);

    /* 角度限幅 */
    if (absPitch > MOTOR_ANGLE_MAX)
        absPitch = MOTOR_ANGLE_MAX;

    /*
     * 占空比映射：角度越大 → 脉冲值越大 → 转速越快
     * pulse = MIN_START + (FULL - MIN_START) * (|角度| / 最大角度)
     */
    float ratio = absPitch / MOTOR_ANGLE_MAX;
    uint16_t pulse = (uint16_t)(MOTOR_DUTY_MIN_START +
                      (float)(MOTOR_DUTY_FULL - MOTOR_DUTY_MIN_START) * ratio);

    DC_Motor_SetSpeed(pulse);
}
