## Pin ESP32-S mapping on breadboard

### ADC sensors
| Sensor      | Signal | GPIO | ADC Channel | Breadboard Row |
|-------------|--------|------|-------------|----------------|
| GP2Y1010    | VO     | 35   | ADC1_7      | J18 |
| GP2Y1010    | LED    | 32   | —           | J17 |
| MQ136       | AO     | 34   | ADC1_6      | J19 |
| MQ137       | AO     | 39   | ADC1_3      | J20 |

### I2C Sensors
| Sensor  | Signal | GPIO | Breadboard Row |
|---------|--------|------|----------------|
| BME680  | SDA    | 21   | a18 |
| BME680  | SCL    | 22   | a21 |

### Power
| Rail  | Breadboard Row |
|-------|----------------|
| 5V    | J5 |
| 3.3V  | J23 |
| GND   | J10 |
