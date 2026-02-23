/*
 * 
 * states.h will be used throughout the project to keep track of the device's state 
 * and manage transitions between states.
 * 
 */
typedef enum{
    STATE_UNPROVISIONED,
    STATE_PROVISIONING,
    STATE_CONNECTING,
    STATE_ONLINE,
    STATE_ERROR
} device_state_t;