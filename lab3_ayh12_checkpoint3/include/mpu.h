#include "driver/i2c.h"
#include "mpu6050.h"

void mpu_setup();
mpu6050_acce_value_t mpu_read_acce();
mpu6050_gyro_value_t mpu_read_gyro();