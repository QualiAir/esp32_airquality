#pragma once //include this file only once
#include "states.h"

void sm_init(); //initialize the state machine
void sm_transition(device_state_t new_state); //transition to a new state
device_state_t sm_get_state(); //get the current state of the device