#include "messages.h"

#include <stdio.h>

MessageBufferHandle_t measurements_msg_buffer;

void pack_measurements_message(uint8_t *buffer, MeasurementsMessage *msg)
{
    buffer[0] = msg->busvoltage & 0xff;
    buffer[1] = (msg->busvoltage >> 8) & 0xff;

    buffer[2] = msg->power & 0xff;
    buffer[3] = (msg->power >> 8) & 0xff;

    buffer[4] = msg->current & 0xff;
    buffer[5] = (msg->current >> 8) & 0xff;

    buffer[6] = msg->shuntvoltage & 0xff;
    buffer[7] = (msg->shuntvoltage >> 8) & 0xff;

    uint32_t cumulative_flow = *((uint32_t *)&msg->cumulative_flow);
    buffer[8] = cumulative_flow & 0xff;
    buffer[9] = (cumulative_flow >> 8) & 0xff;
    buffer[10] = (cumulative_flow >> 16) & 0xff;
    buffer[11] = (cumulative_flow >> 24) & 0xff;

    uint32_t flow_rate = *((uint32_t *)&msg->flow_rate);
    buffer[12] = flow_rate & 0xff;
    buffer[13] = (flow_rate >> 8) & 0xff;
    buffer[14] = (flow_rate >> 16) & 0xff;
    buffer[15] = (flow_rate >> 24) & 0xff;
}

void unpack_measurements_message(uint8_t *buffer, MeasurementsMessage *msg)
{
    msg->busvoltage = ((int16_t)buffer[1] << 8) | buffer[0];
    msg->power = ((int16_t)buffer[3] << 8) | buffer[2];
    msg->current = ((int16_t)buffer[5] << 8) | buffer[4];
    msg->shuntvoltage = ((int16_t)buffer[7] << 8) | buffer[6];

    uint32_t cumulative_flow = (((uint32_t)buffer[8] << 0) |
                                ((uint32_t)buffer[9] << 8) |
                                ((uint32_t)buffer[10] << 16) |
                                ((uint32_t)buffer[11] << 24));

    msg->cumulative_flow = *((float *)&cumulative_flow);

    uint32_t flow_rate = (((uint32_t)buffer[12] << 0) |
                          ((uint32_t)buffer[13] << 8) |
                          ((uint32_t)buffer[14] << 16) |
                          ((uint32_t)buffer[15] << 24));

    msg->flow_rate = *((float *)&flow_rate);
}

void measurements_message_to_str(char *str, MeasurementsMessage *msg)
{
    sprintf(
        str,
        "{\"busvoltage\":%d,\"power\":%d,\"current\":%d,\"shuntvoltage\":%d,\"cumulative_flow\":%f,\"flow_rate\":%f}",
        msg->busvoltage,
        msg->power,
        msg->current,
        msg->shuntvoltage,
        msg->cumulative_flow,
        msg->flow_rate);
}