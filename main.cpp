#include "mbed.h"

#include "src/config.h"
#include "src/ina219.h"

#define HEARTBREAT_RATE     500ms

DigitalOut heartbeat_led(LED1);

int main()
{
    puts("Hello World");

    printf("Calibration current sensor - ");
    CurrentSensor current_sensor(INA219_SDA, INA219_SCLK);
    printf("Done!\r\n");

    while (true) {
        printf(
            "I=%d Vb=%d (mV) Vs=%d\r\n", 
            current_sensor.read_current_raw(), 
            current_sensor.read_bus_voltage_mV(), 
            current_sensor.read_shunt_voltage_raw()
        );
        
        ThisThread::sleep_for(1s);
        heartbeat_led = !heartbeat_led;
    }
}