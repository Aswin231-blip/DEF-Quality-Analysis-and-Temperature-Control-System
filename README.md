# DEF Monitoring and Conditioning System

This project is an intelligent DEF (Diesel Exhaust Fluid) monitoring and conditioning system built using an ESP32 microcontroller. It detects the purity of DEF in real-time using multiple sensors and controls subsequent flow and conditioning operations automatically.

---

## ✨ Features

- **Sensor-based Purity Detection**:
  - TDS sensor (temperature compensated)
  - Turbidity sensor
  - Refractometer (light-based concentration estimation)
- **Real-time LCD Readout**
- **Buzzer and LED Indicators**:
  - Green LED for pure DEF
  - Red LED + Buzzer for impure DEF
- **Pump A Control**: Starts 15s after DEF is detected, stops when air is detected (TDS ~ 0 for 10s)
- **Tank C Temperature Management**:
  - DS18B20 temperature sensor for monitoring
  - Heating pads if temp < 15°C
  - Cooling (Peltier + Fan) if temp > 30°C
  - Smart circulation pump logic:
    - Continuous when heating
    - 2min ON, 10min OFF when normal
    - 5s ON, 10s OFF when cooling
- **All logic runs fully automatically upon DEF input detection via limit switch**

---

## 🧰 Hardware Components (BOM)

| Component                | Quantity | Notes                                            |
|--------------------------|----------|--------------------------------------------------|
| ESP32 Dev Board         | 1        | Main controller                                  |
| TDS Sensor              | 1        | Analog, 3.3V                                     |
| Turbidity Sensor        | 1        | Analog, 5V                                       |
| Refractometer (LED+LDR) | 1        | Analog, 5V, custom-built                         |
| DS18B20 Temp Sensor     | 2        | One in Tank A, one in Tank C                     |
| LiquidCrystal_I2C LCD   | 1        | 2x16, I2C Address 0x27                           |
| Relay Module (8ch)      | 1        | For all actuators                               |
| Buzzer                  | 1        | Active, 5V                                       |
| Green LED               | 1        | Indicates pure DEF                               |
| Red LED                 | 1        | Indicates impure DEF                             |
| Pump A (12V, 20W)       | 1        | For transferring DEF                             |
| Pump C (12V)            | 1        | Circulation pump in Tank C                      |
| Heating Pads (12V)      | 2        | Mounted on opposite sides of Tank C             |
| Peltier Module (TEC1-12706) | 1     | For cooling                                      |
| Fan (12V)               | 1        | Paired with Peltier                              |
| Limit Switch            | 1        | Mounted at DEF input nozzle                      |
| 12V 10Ah Battery        | 1        | Power supply                                     |
| Buck Converter (12V to 3.3V) | 1    | For ESP32 and sensors requiring 3.3V             |
| Flyback Diodes          | 2+       | For all inductive loads (pumps, etc.)            |
| Perfboard + Jumper Wires| -        | For wiring                                        |

---

## 🔹 Wiring Table

| Pin Name         | ESP32 GPIO | Connected To                       |
|------------------|------------|------------------------------------|
| Trigger Switch   | 4          | Limit switch (INPUT_PULLUP)        |
| Green LED        | 5          | Green indicator (Active LOW relay) |
| Red LED          | 18         | Red indicator (Active LOW relay)   |
| Buzzer           | 19         | Buzzer (Active LOW relay)          |
| Pump A Relay     | 21         | Transfers DEF                      |
| Heater Relay     | 22         | Heating Pads in Tank C             |
| Peltier Relay    | 23         | Cooling system                     |
| Fan Relay        | 25         | Paired with Peltier                |
| Pump C Relay     | 26         | Circulation pump                   |
| TDS Sensor       | A0 (32)    | Analog Input                       |
| Turbidity Sensor | A1 (33)    | Analog Input                       |
| Refractometer    | A2 (34)    | Analog Input                       |
| Temp Sensor A    | OneWire on GPIO 27 | DS18B20 OneWire             |
| Temp Sensor C    | OneWire on GPIO 15 | DS18B20 OneWire             |
| LCD SDA          | GPIO 21    | I2C Data (shared with relay if needed) |
| LCD SCL          | GPIO 22    | I2C Clock                          |

---

## 🔧 How It Works

1. **Trigger**: The system starts when the input nozzle is engaged, activating a limit switch.
2. **Sensor Sampling**: For 15s, TDS, turbidity, and refractometer readings are monitored.
3. **Decision**:
   - If all three parameters fall within expected range, DEF is deemed **pure**: Green LED lights up.
   - If any parameter is out of range, DEF is **impure**: Red LED + buzzer activate.
4. **Pump A Activation**: Pump starts 15s after trigger, and continues until TDS reads ~0 for 10s (indicating tank is empty).
5. **Tank C Monitoring**:
   - Starts 20s after trigger.
   - Heating or cooling begins if temp is outside 15-30°C range.
   - Pump C operates differently depending on temp range (continuous, 2m/10m, or 5s/10s cycles).

---

## 📖 Backstory: Caterpillar Tech Challenge 2025

This DEF monitoring project was designed and built by Abdullah as a final-year mechatronics showcase and eventually selected for the **Caterpillar Tech Challenge 2025 Finale**. The system demonstrated innovative field-usable diagnostics using real sensor feedback and low-cost automation, drawing praise from engineers and judges for its robustness and adaptability. The event featured challenges like sudden DEF quality changes and variable temperatures, all of which this prototype handled with real-time analysis and appropriate actuator control.

What started as a side experiment with pumps and sensors turned into a competition-worthy solution that stood out for practical utility and creative engineering.

---

## ✨ License

Open-source under MIT License. Feel free to fork, remix, and deploy.

---

## 📈 Contributions

Pull requests and suggestions welcome!

---

