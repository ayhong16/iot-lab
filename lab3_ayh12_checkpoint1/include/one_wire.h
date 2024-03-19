#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t int_rh;
    uint8_t dec_rh;
    uint8_t int_temp;
    uint8_t dec_temp;
    uint8_t checksum;
} SensorData;

void send_start_signal();
bool check_start_response();
bool check_start();
SensorData read_sensor_data();