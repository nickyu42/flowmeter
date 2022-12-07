<div align="center">
  <h1>Energy Harvesting Flow Meter</h1>

  <img style="box-shadow: 0 4px 8px 0 rgba(0, 0, 0, 0.2), 0 6px 20px 0 rgba(0, 0, 0, 0.19);" src="https://i.imgur.com/pGg8577.png" title="source: imgur.com" width="50%"/>
</div>



## About

This repository contains the hardware design and software for a wireless, energy harvesting flow meter. This is an inexpensive water consumption sensor that does not require external power or cables to transmit data. It uses a turbine as power supply, the power supply is also used as input to estimate water consumption.

This project was done as part of the Advanced Practical IoT and Seminar course given at the Delft University of Technology. If you are interested in the research, an accompanying [paper](paper.pdf) is available in this repository.

## Repository Structure

This project is split up into different components:

- `pcb`
    KiCAD files for the prototype
- `analysis.ipynb`
    Notebook containing all plots and analysis from the dataset in `data`
- `gateway`
  - Firmware for the receiver ESP32
  - UART-to-MQTT script, parses the serial data from the receiver ESP and publishes that to a MQTT server
  - All code for the web dashboard for tracking water usage
- `stm32f4_firmware`
  Code for the testing setup with a STM32, based around [MBed OS](https://os.mbed.com/mbed-os/).
- `esp32_firmware`
  ESP32 firmware for the prototype, transmits the estimated water flow rate and power metrics via ESP-NOW to the receiver. Built using [FreeRTOS](https://www.freertos.org/) and [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html).