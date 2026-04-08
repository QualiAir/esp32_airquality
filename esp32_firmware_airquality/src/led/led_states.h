typedef enum {
    LED_MODE_OFF,
    LED_MODE_MQTT_SENDING,   // slow blink on data send
    LED_MODE_WIFI_CONNECTING, // fast blink
    LED_MODE_RESET,          // blink then off
} led_mode_t;