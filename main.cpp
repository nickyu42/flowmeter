#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "BUILD/NUCLEO_F446RE/ARMC6/mbed_config.h"
#include "BufferedSerial.h"
#include "PinNameAliases.h"
#include "ThisThread.h"
#include "Thread.h"
#include "mbed.h"

#include "src/CharCircularBuffer.h"
#include "src/CurrentSensor.h"
#include "src/config.h"

using namespace std::chrono;

#define HEARTBREAT_RATE 500ms

#define CURRENT_SENSOR_BUFFER_SIZE 20
#define CURRENT_SENSOR_READ_DELAY 500000us

// 4 * int16_t    = 8 bytes
// 4 comma chars  = 4 bytes
// 1 double (fr)  = 8 bytes
// \r\n           = 2 bytes
#define CURRENT_SENSOR_PACKET_SIZE (4 * 2 + 4 + 8 + 2)

#define CRITICAL_VOLTAGE_mV 7000


DigitalOut heartbeat_led(LED1);

Thread read_sensor_thread;
Ticker read_sensor;
CharCircularBuffer<CURRENT_SENSOR_BUFFER_SIZE> busvoltage_samples_mV;
CharCircularBuffer<CURRENT_SENSOR_BUFFER_SIZE> current_samples_raw;
CharCircularBuffer<CURRENT_SENSOR_BUFFER_SIZE> power_samples;
float current_flow_rate = 0;

size_t critical_voltage_samples = 0;

BufferedSerial console(USBTX, USBRX, MBED_CONF_PLATFORM_STDIO_BAUD_RATE);


uint8_t data_buffer[CURRENT_SENSOR_PACKET_SIZE] = {0};

Timer read_timer;

FileHandle* mbed::mbed_override_console(int)
{
    return &console;
}

double estimate_flowrate(int16_t power_mean) {
  double a = 148.450205494820409057865617796779;
  double b = -826.204954144229304802138358354568;
  return (power_mean - b) / a;
}

void read_current_task(CurrentSensor *s) {

  printf("[INFO] Starting read_current task\r\n");

  while (true) {
    read_timer.reset();
    read_timer.start();

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
      // power_down sequence
    }

    // busvoltage_samples_mV.push(static_cast<int16_t>(busvoltage));
    // current_samples_raw.push(s->read_current_raw());
    power_samples.push(power);

    // current_flow_rate = ceil(busvoltage_samples_mV.mean()) / 1000;
    current_flow_rate = estimate_flowrate(power_samples.mean());

    // data_buffer[0] = static_cast<uint8_t>(busvoltage & 0xff);
    // data_buffer[1] = static_cast<uint8_t>((busvoltage >> 8) & 0xff);

    // data_buffer[3] = static_cast<uint8_t>(current & 0xff);
    // data_buffer[4] = static_cast<uint8_t>((current >> 8) & 0xff);

    // data_buffer[6] = static_cast<uint8_t>(shuntvoltage & 0xff);
    // data_buffer[7] = static_cast<uint8_t>((shuntvoltage >> 8) & 0xff);

    // data_buffer[9] = static_cast<uint8_t>(power & 0xff);
    // data_buffer[10] = static_cast<uint8_t>((power >> 8) & 0xff);

    // uint64_t current_flow_rate_uint = static_cast<uint64_t>(current_flow_rate);
    // for (size_t i = 0; i < 8; i++) {
    //   data_buffer[12 + i] = static_cast<uint8_t>((current_flow_rate_uint >> (8 * i)) & 0xff);
    // }

    printf("%d,%d,%d,%d,%f\r\n", busvoltage, current, shuntvoltage, power, current_flow_rate);

    read_timer.stop();

    printf("time taken: %llu us\r\n", duration_cast<microseconds>(read_timer.elapsed_time()).count());

    ThisThread::sleep_for(duration_cast<milliseconds>(
        CURRENT_SENSOR_READ_DELAY - read_timer.elapsed_time() ));
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

  data_buffer[2] = ',';
  data_buffer[5] = ',';
  data_buffer[8] = ',';
  data_buffer[11] = ',';
  data_buffer[CURRENT_SENSOR_PACKET_SIZE - 2] = '\r';
  data_buffer[CURRENT_SENSOR_PACKET_SIZE - 1] = '\n';

  while (true) {
    ThisThread::sleep_for(HEARTBREAT_RATE);
    heartbeat_led = !heartbeat_led;

    // printf("[INFO] Measured flowrate: %d l/10min\r\n",
    // static_cast<uint16_t>(current_flow_rate * 10));
  }
}