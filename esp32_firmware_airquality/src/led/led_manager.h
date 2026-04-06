#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include "led_states.h"
#include "driver/gpio.h"

void led_manager_init(void);
void led_manager_set_state(led_mode_t new_state);
led_mode_t led_manager_get_state(void);
#define LED_PIN GPIO_NUM_26

#endif