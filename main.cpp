#include "ThisThread.h"
#include "mbed.h"
#include <cstdio>

#include "src/config.h"
#include "src/ina219.h"

#define HEARTBREAT_RATE 500ms

Thread read_sensor;

DigitalOut heartbeat_led(LED1);

void read_sensor_task(CurrentSensor *s) 
{
    while (true)
    {
        printf(
            "%d,%d,%d,%d\r\n", 
            s->read_current_raw(), 
            s->read_bus_voltage_mV(), 
            s->read_shunt_voltage_raw(),
            s->read_power_raw()
        );

        ThisThread::sleep_for(100ms);
    }
}

int main()
{
    puts("Hello World");

    printf("Calibration current sensor - ");
    CurrentSensor current_sensor(INA219_SDA, INA219_SCLK);
    printf("Done!\r\n");

    read_sensor.start(callback(read_sensor_task, &current_sensor));

    while (true) {
        ThisThread::sleep_for(HEARTBREAT_RATE);
        heartbeat_led = !heartbeat_led;
    }
}