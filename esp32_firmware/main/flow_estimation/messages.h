#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"

#define MEASUREMENTS_MESSAGE_SIZE 16

// Enough for 32 messages
#define MEASUREMENTS_MESSAGE_BUFFER_SIZE 512

// Size = 4 * 2 bytes + 2 * 4 bytes = 16 bytes
typedef struct MeasurementsMessage_t
{
    int16_t busvoltage;
    int16_t power;
    int16_t current;
    int16_t shuntvoltage;
    float cumulative_flow;
    float flow_rate;
} MeasurementsMessage;

void pack_measurements_message(uint8_t *buffer, MeasurementsMessage *msg);
void unpack_measurements_message(uint8_t *buffer, MeasurementsMessage *msg);
void measurements_message_to_str(char *str, MeasurementsMessage *msg);

#endif