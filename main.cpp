#include <cmath>
#include <cstdint>
#include <cstdio>

#include "ThisThread.h"
#include "Thread.h"
#include "mbed.h"

#include "src/CharCircularBuffer.h"
#include "src/CurrentSensor.h"
#include "src/config.h"

#define HEARTBREAT_RATE 500ms

#define CURRENT_SENSOR_BUFFER_SIZE 100
#define CURRENT_SENSOR_READ_DELAY 10ms

#define CRITICAL_VOLTAGE_mV 7000

DigitalOut heartbeat_led(LED1);

Thread read_sensor_thread;
Ticker read_sensor;
CharCircularBuffer<CURRENT_SENSOR_BUFFER_SIZE> busvoltage_samples_mV;
CharCircularBuffer<CURRENT_SENSOR_BUFFER_SIZE> current_samples_raw;
float current_flow_rate = 0;

size_t critical_voltage_samples = 0;

void read_current_task(CurrentSensor *s) {

  printf("[INFO] Starting read_current task\r\n");

  while (true) {
    uint16_t busvoltage = s->read_bus_voltage_mV();

    if (busvoltage > CRITICAL_VOLTAGE_mV) {
      critical_voltage_samples++;
    } else if (critical_voltage_samples > 0) {
      critical_voltage_samples--;
    }

    if (critical_voltage_samples > 10) {
      // power_down sequence
    }

    busvoltage_samples_mV.push(static_cast<int16_t>(busvoltage));
    current_samples_raw.push(s->read_current_raw());

    current_flow_rate = ceil(busvoltage_samples_mV.mean()) / 1000;
    ThisThread::sleep_for(CURRENT_SENSOR_READ_DELAY);
  }
}

int main() {
  printf("==================\r\n");
  printf("[ OK ] Hello World\r\n");

  printf("[INFO] Calibrating current sensor\r\n");
  CurrentSensor current_sensor(INA219_SDA, INA219_SCLK);
  printf("[ OK ] Done!\r\n");

  printf("[INFO] Starting INA219\r\n");
  //   read_sensor.attach(callback(read_current_task, &current_sensor),
  //                      CURRENT_SENSOR_READ_DELAY);
  read_sensor_thread.start(callback(read_current_task, &current_sensor));
  printf("[ OK ] Done!\r\n");

  while (true) {
    ThisThread::sleep_for(HEARTBREAT_RATE);
    heartbeat_led = !heartbeat_led;

    // printf("[INFO] Measured flowrate: %d l/10min\r\n", static_cast<uint16_t>(current_flow_rate * 10));
  }
}