/* servoControl.h 
 *
 * Initialize and control continuous and 180 degree servos
*/

#ifndef DISPENSECONTROLX_H
#define DISPENSECONTROLX_H

#include "stm32h5xx_hal.h"
#include <stdint.h>

typedef enum {
  one = 1,
  two = 2,
  three = 3,
  four = 4
} channel_e;

int dispensePills(uint32_t channel, uint32_t numberPills);

int openGate();
int closeGate();

#endif
