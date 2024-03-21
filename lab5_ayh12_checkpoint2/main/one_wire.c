#include "one_wire.h"

#define DHT_PIN 7

static void us_delay(int us) {
    uint32_t curr = esp_timer_get_time();
    while ((esp_timer_get_time() - curr) < us) {
        continue;
    }
}

bool check_start() {
    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    us_delay(18010);

    gpio_set_level(DHT_PIN, 1);
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);
    us_delay(42);

    uint64_t start = esp_timer_get_time();
    while (!gpio_get_level(DHT_PIN)) {
        if (esp_timer_get_time() - start > 100) {
            return false;
        }
    }

    while (gpio_get_level(DHT_PIN)) {
        continue;
    }

    return true;
}

uint64_t read_one_bit() {
    while (!gpio_get_level(DHT_PIN)) {
        continue;
    }

    uint64_t start = esp_timer_get_time();
    uint64_t end = 0;

    while (gpio_get_level(DHT_PIN)) {
        end = esp_timer_get_time();
        if (end - start > 110) {
            printf("Bit Timeout\n");
            return 0;
        }
    }

    return end - start;
}

SensorData read_sensor_data() {
    uint8_t data[5] = {0};
    uint8_t checksum = 0;
    uint8_t num_bits = 40;
    SensorData result = {0};

    if (check_start() == false) {
        printf("Start signal error\n");
        result.error = true;
        return result;
    }

    // Loop to read all 40 bits of data
    for (int i = 0; i < num_bits; i++) {
        uint32_t time = read_one_bit();
        if (time == 0) {
            result.error = true;
            return result;
        }
        uint8_t bit = time > 60 ? 1 : 0;
        data[i / 8] |= bit << (7 - (i % 8));
    }

    // Calculate checksum
    for (int i = 0; i < 4; i++) {
        checksum += data[i];
    }

    // Verify checksum
    if (checksum != data[4]) {
        printf("Checksum error!\n");
        result.error = true;
        return result;
    }

    // Prepare the result
    result.int_rh = data[0];
    result.dec_rh = data[1];
    result.int_temp = data[2];
    result.dec_temp = data[3];
    result.checksum = data[4];
    result.error = false;

    return result;
}

// bool check_start() {
//     // Pull the GPIO pin low for at least 18ms to let DHT11 detect the start signal
//     gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
//     gpio_set_level(DHT_PIN, 0);
//     us_delay(18010);

//     // Pull the GPIO pin high and wait for DHT11 response (20-40 microseconds)
//     gpio_set_level(DHT_PIN, 1);
//     gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);
//     us_delay(41);

//     uint64_t start = esp_timer_get_time();
//     while (!gpio_get_level(DHT_PIN)) {
//         if ((esp_timer_get_time() - start) > 80) {
//             return false;
//         }
//     }
//     us_delay(80);
//     return true;
// }

// uint64_t read_one_bit() {
//     // wait for the wire to go high
//     while (!gpio_get_level(DHT_PIN)) {
//         continue;
//     }

//     uint64_t start = esp_timer_get_time();

//     // wait for wire to go low
//     uint64_t end = 0;
//     while (gpio_get_level(DHT_PIN)) {
//         end = esp_timer_get_time();
//         if ((end - start) > 80) {
//             printf("Bit timeout\n");
//             return 0;
//         }
//     }
//     return end - start;
// }

// SensorData read_sensor_data() {
//     uint8_t data[5] = {0};
//     int num_bits = 40;
//     uint8_t checksum = 0;
//     int error = 0;

//     if (!check_start()) {
//         printf("Start signal error\n");
//         return (SensorData){0};
//     }

//     for (int i = 0; i < num_bits; i++) {
//         uint64_t diff = read_one_bit();
//         if (diff == 0) {
//             error = 1;
//             break;
//         }
//         int bit = diff > 60 ? 1 : 0;
//         data[i / 8] |= bit << (7 - (i % 8));
//     }

//     for (int i = 0; i < 4; i++) {
//         checksum += data[i];
//     }

//     // Verify checksum
//     if (checksum != data[4]) {
//         printf("Checksum error!\n");
//         error = 1;
//     }

//     if (error) {
//         return (SensorData){0};
//     }

//     SensorData ret;
//     ret.int_rh = data[0];
//     ret.dec_rh = data[1];
//     ret.int_temp = data[2];
//     ret.dec_temp = data[3];
//     ret.checksum = data[4];

//     return ret;
// }
