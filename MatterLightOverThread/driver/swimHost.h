#pragma once

#include <stdint.h>
#include <stdbool.h>

void    SWIM_Host_Init(void);
void    SWIM_Host_SendResetSequence(void);
uint8_t SWIM_Host_SendPacket(uint8_t *p_data, uint8_t len);
uint8_t SWIM_Host_ReadPacket(uint8_t *p_buf, uint8_t len);
