#ifndef __MPU6050_H
#define __MPU6050_H

#include <stdint.h>

/* 换算后的数据（供 main.c 显示用） */
extern float Ax, Ay, Az;  // 加速度 (g)
extern float Gx, Gy, Gz;  // 角速度 (°/s)
extern float Temperature;  // 温度 (°C)

/* 初始化 MPU6050：唤醒 + 配置量程/滤波器/采样率 */
void MPU6050_Init(void);

/* 读取所有数据并换算为单位值 */
void MPU6050_Read_All(void);

#endif
