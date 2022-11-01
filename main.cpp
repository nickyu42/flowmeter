#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "BUILD/NUCLEO_F446RE/ARMC6/mbed_config.h"
#include "BufferedSerial.h"
#include "EventFlags.h"
#include "PinNameAliases.h"
#include "ThisThread.h"
#include "Thread.h"
#include "Ticker.h"
#include "mbed.h"
#include "rtos.h"

#include "src/CharCircularBuffer.h"
#include "src/CurrentSensor.h"
#include "src/config.h"

using namespace std::chrono;

#define HEARTBREAT_RATE 500ms

#define CURRENT_SENSOR_READ_FLAG (1UL << 0)
#define POWER_DOWN_FLAG (1UL << 2)

#define CURRENT_SENSOR_BUFFER_SIZE 20
#define CURRENT_SENSOR_READ_DELAY 250ms

// Voltage level at which the system powers down
#define CRITICAL_VOLTAGE_mV 7000

EventFlags event_flags;

// RTOS objects
Thread read_sensor_thread;
Ticker read_ticker;

// Current sensor readings
CharCircularBuffer<CURRENT_SENSOR_BUFFER_SIZE> busvoltage_samples_mV;
CharCircularBuffer<CURRENT_SENSOR_BUFFER_SIZE> current_samples_raw;
CharCircularBuffer<CURRENT_SENSOR_BUFFER_SIZE> power_samples;
float current_flow_rate = 0;

double estimate_flowrate(int16_t power_mean) {
  double a = 148.450205494820409057865617796779;
  double b = -826.204954144229304802138358354568;
  return (power_mean - b) / a;
}

void set_read_flag() { event_flags.set(CURRENT_SENSOR_READ_FLAG); }

void read_current_task(CurrentSensor *s) {
  printf("[INFO] Starting read_current task\r\n");

  size_t critical_voltage_samples = 0;

  while (true) {
    event_flags.wait_any(CURRENT_SENSOR_READ_FLAG);

    int16_t busvoltage = s->read_bus_voltage_mV();
    int16_t current = s->read_current_raw();
    int16_t shuntvoltage = s->read_shunt_voltage_raw();
    int16_t power = s->read_power_raw();

    if (busvoltage > CRITICAL_VOLTAGE_mV) {
      critical_voltage_samples++;
    } else if (critical_voltage_samples > 0) {
      critical_voltage_samples--;
    }

    if (critical_voltage_samples > 10) {
      event_flags.set(POWER_DOWN_FLAG);
    }

    power_samples.push(power);
    current_flow_rate = estimate_flowrate(power_samples.mean());

    printf("%d,%d,%d,%d,%f\r\n", busvoltage, current, shuntvoltage, power,
           current_flow_rate);
  }
}

int main() {
  printf("==================\r\n");
  printf("[ OK ] Hello World\r\n");

  printf("[INFO] Calibrating current sensor\r\n");
  CurrentSensor current_sensor(INA219_SDA, INA219_SCLK);
  printf("[ OK ] Done!\r\n");

  printf("[INFO] Starting INA219\r\n");
  read_sensor_thread.start(callback(read_current_task, &current_sensor));
  read_ticker.attach(set_read_flag, CURRENT_SENSOR_READ_DELAY);
  printf("[ OK ] Done!\r\n");

  DigitalOut heartbeat_led(LED1);

  while (true) {
    ThisThread::sleep_for(HEARTBREAT_RATE);
    heartbeat_led = !heartbeat_led;
  }
}