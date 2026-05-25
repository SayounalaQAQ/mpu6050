#ifndef __MPU6050_H
#define __MPU6050_H

#include <stdint.h>

/* ---------- I2C 地址 ---------- */
#define MPU6050_ADDR            0x68
#define MPU6050_I2C_ADDR        (MPU6050_ADDR << 1)
#define MPU6050_I2C_TIMEOUT     100

/* ---------- 寄存器地址 ---------- */
#define MPU6050_REG_SMPLRT_DIV  0x19
#define MPU6050_REG_CONFIG      0x1A
#define MPU6050_REG_GYRO_CONFIG 0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_PWR_MGMT_1  0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

/* ---------- 寄存器值 ---------- */
#define MPU6050_PWR_WAKE        0x00
#define MPU6050_GYRO_RANGE_250  0x00
#define MPU6050_ACCEL_RANGE_2G  0x00
#define MPU6050_DLPF_44HZ       0x03

/* ---------- 量程系数（物理量换算） ---------- */
#define MPU6050_ACCEL_SCALE     16384.0f   /* ±2g */
#define MPU6050_GYRO_SCALE      131.0f     /* ±250°/s */
#define MPU6050_TEMP_SCALE      340.0f
#define MPU6050_TEMP_OFFSET     36.53f

/* ---------- 数据长度 ---------- */
#define MPU6050_DATA_LEN        14

/**
  * @brief   初始化 MPU6050：唤醒 + 配置量程/滤波器/采样率
  * @retval  0-成功，1-失败
  */
uint8_t MPU6050_Init(void);

/**
  * @brief   读取所有传感器数据并换算为物理单位
  * @retval  0-成功，1-失败
  */
uint8_t MPU6050_ReadAll(void);

/**
  * @brief  获取 X 轴加速度（g）
  */
float MPU6050_GetAccelX(void);

/**
  * @brief  获取 Y 轴加速度（g）
  */
float MPU6050_GetAccelY(void);

/**
  * @brief  获取 Z 轴加速度（g）
  */
float MPU6050_GetAccelZ(void);

/**
  * @brief  获取 X 轴角速度（°/s）
  */
float MPU6050_GetGyroX(void);

/**
  * @brief  获取 Y 轴角速度（°/s）
  */
float MPU6050_GetGyroY(void);

/**
  * @brief  获取 Z 轴角速度（°/s）
  */
float MPU6050_GetGyroZ(void);

/**
  * @brief  获取温度（°C）
  */
float MPU6050_GetTemperature(void);

#endif
