#ifndef __DC_MOTOR_H
#define __DC_MOTOR_H

#include <stdint.h>

/* ---------- 引脚定义 ---------- */
#define MOTOR_PIN_IN1           GPIO_PIN_4
#define MOTOR_PIN_IN2           GPIO_PIN_5
#define MOTOR_PORT              GPIOA

/* ---------- PWM 参数（运行时重新配置为 20kHz） ---------- */
#define MOTOR_TIM               &htim2
#define MOTOR_PWM_CHANNEL       TIM_CHANNEL_2
#define MOTOR_TIM_PSC           7           /* 72MHz / (7+1) = 9MHz */
#define MOTOR_TIM_ARR           449         /* 9MHz / 450 = 20kHz */
#define MOTOR_PWM_PERIOD        MOTOR_TIM_ARR

/* ---------- 转向定义 ---------- */
#define MOTOR_DIR_CW            0       /* IN1=1, IN2=0 */
#define MOTOR_DIR_CCW           1       /* IN1=0, IN2=1 */
#define MOTOR_DIR_STOP          2       /* IN1=0, IN2=0 */

/* ---------- 角度-占空比映射 ---------- */
#define MOTOR_ANGLE_MAX         60.0f   /* 最大有效俯仰角 */
#define MOTOR_DEAD_ANGLE        5.0f    /* 死区角度，小于此值电机停止 */
#define MOTOR_DUTY_IDLE         0       /* 停止时脉冲值 */
#define MOTOR_DUTY_MIN_START    100     /* 电机启动最小脉冲（约 22% 占空比） */
#define MOTOR_DUTY_FULL         449     /* 最大角度时脉冲值（约 100%） */

/**
  * @brief   初始化电机驱动（启动 PWM 输出，默认停止）
  */
void DC_Motor_Init(void);

/**
  * @brief   设置电机转速（PWM 脉冲值）
  * @param   pulse  PWM 比较值，0 ~ MOTOR_PWM_PERIOD
  */
void DC_Motor_SetSpeed(uint16_t pulse);

/**
  * @brief   设置电机转向
  * @param   dir   MOTOR_DIR_CW / MOTOR_DIR_CCW / MOTOR_DIR_STOP
  */
void DC_Motor_SetDirection(uint8_t dir);

/**
  * @brief   根据 MPU6050 俯仰角控制电机
  * @param   pitch  俯仰角（度），正=前倾，负=后仰
  */
void DC_Motor_ControlByPitch(float pitch);

#endif
