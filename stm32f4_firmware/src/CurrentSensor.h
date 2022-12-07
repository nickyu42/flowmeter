#pragma once

#include "INA219.hpp"

class CurrentSensor : public INA219 {

public:
  CurrentSensor(PinName sda, PinName scl) : INA219(sda, scl){};

  int16_t read_bus_voltage_raw() {
    return (int16_t)read_register_u16(INA219_REG_BUSVOLTAGE);
  }

  int16_t read_shunt_voltage_raw() {
    return (int16_t)read_register_u16(INA219_REG_SHUNTVOLTAGE);
  }

  int16_t read_power_raw() {
    return (int16_t)read_register_u16(INA219_REG_POWER);
  }

  int16_t read_bus_voltage_mV() {
    // The Bus Voltage register bits are not right-aligned. In order to
    // compute the value of the Bus Voltage, Bus Voltage Register contents
    // must be shifted right by three bits. This shift puts the BD0 bit in
    // the LSB position so that the contents can be multiplied by the Bus
    // Voltage LSB of 4-mV to compute the bus voltage measured by the device.
    return (read_bus_voltage_raw() >> 3) * 4;
  }
};