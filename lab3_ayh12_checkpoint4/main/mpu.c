#include "mpu.h"

#define SCL_PIN 20
#define SDA_PIN 26
#define I2C_HOST I2C_NUM_1
#define I2C_FREQ_HZ (500 * 1000)

static mpu6050_handle_t mpu_dev = NULL;

void mpu_setup() {
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,  // I2C LCD is a master node
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_HOST, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_HOST, I2C_MODE_MASTER, 0, 0, 0));

    mpu_dev = mpu6050_create(I2C_NUM_1, MPU6050_I2C_ADDRESS);
    mpu6050_config(mpu_dev, ACCE_FS_4G, GYRO_FS_500DPS);
    mpu6050_wake_up(mpu_dev);
}

mpu6050_acce_value_t mpu_read_acce() {
    static mpu6050_acce_value_t acce;
    mpu6050_get_acce(mpu_dev, &acce);
    return acce;
}

mpu6050_gyro_value_t mpu_read_gyro() {
    static mpu6050_gyro_value_t gyro;
    mpu6050_get_gyro(mpu_dev, &gyro);
    return gyro;
}