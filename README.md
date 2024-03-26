# iot-lab

This repository contains the code for 5 labs completed for ECE 590: Full Stack Internet of Things at Duke University. The labs are designed to be completed in order, as each lab builds on the previous one. The labs are as follows with a brief description of key takeaways from each:

### Lab 0: ESP32 Environment Setup and Initial Program

- Understanding how to set up the ESP32 environment and use GPIO pins to do basic interactions with various inputs and outputs.

### Lab 1: Introduction to GPIO, Timers, and Interrupts

- Set up GPIO pins to perform more complex interacts with inputs and outputs.
- Set up interrupt-based timers to perform periodic tasks such that the application does not need to run at all times.
- Minimize CPU usage by using interrupts to perform tasks.

### Lab 2: Introduction to PWM, OLED, and ADC

- Understanding of fundamental espressif libraries that such as LEDC, LCD, LVGL, and ADC.
- Understanding how to use ADC library to convert analog temperature signals to digital signals.

### Lab 3: Serial Bus Protocols

- Implemented 1-Wire protocol manually to interface DHT11 temperature and humidity sensor to the ESP32.
- Used oscilloscope to analyze the signal and understand the 1-wire protocol.
- Interfaced with multiple I2C devices simultaneously.

### Lab 4: Connecting ESP32 to Wi-Fi and Interacting with HTTP APIs

- Understanding how to connect ESP32 to Wi-Fi and hit a Geolocation API as well as a Weather API.
- Understanding how to use the cJSON library to parse JSON data and display it on an OLED display.

### Lab 5: Web Server, Database, and Putting Everything Together

- Set up AWS EC2 instance to host a Flask web server and a PostgreSQL database.
- Created a complete pipeline that starts from the ESP32 collecting temperature and humidity data from the DHT11 sensor, sending it to the Flask server, and storing it in the PostgreSQL database.
- Exported the database to a csv to analyze temperature and humidity trends over a 24-hour period.
