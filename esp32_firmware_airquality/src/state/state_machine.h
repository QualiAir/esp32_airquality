#pragma once //include this file only once
#include "states.h"

void sm_init(); //initialize the state machine
void sm_transition(device_state_t new_state); //transition to a new state
device_state_t sm_get_state(); //get the current state of the device
void sm_start(void); //start the state machine, this will be called in main.c to kick off the state machine logic