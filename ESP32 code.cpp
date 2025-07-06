#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- CONFIGURABLE CONSTANTS ---
const float BASE_TDS_PPM = 450.0;
const unsigned long debounceDelay = 50;
const int turbidityMinADC = 4080;
const int turbidityMaxADC = 4095;
const int refractMinADC = 3000;
const int refractMaxADC = 4095;
const float tdsZeroThreshold = 5.0;
const unsigned long tdsZeroDelay = 10000;
const unsigned long tempMonitoringDelay = 20000;
const unsigned long heatingThreshold = 10;
const unsigned long coolingThreshold = 33;

// DS18B20 setup
#define ONE_WIRE_BUS_C 15
#define ONE_WIRE_BUS_A 4
OneWire oneWireC(ONE_WIRE_BUS_C);
OneWire oneWireA(ONE_WIRE_BUS_A);
DallasTemperature tempSensorC(&oneWireC);
DallasTemperature tempSensorA(&oneWireA);

// Pin assignments
const int TRIGGER_SWITCH_PIN = 4;
const int GREEN_LED = 5;
const int RED_LED = 18;
const int BUZZER = 19;
const int PUMP_A_RELAY = 21;
const int HEATER_RELAY = 22;
const int PELTIER_RELAY = 23;
const int FAN_RELAY = 25;
const int PUMP_C_RELAY = 26;

// Analog pins
const int TDS_PIN = 32;
const int TURBIDITY_PIN = 33;
const int REFRACT_PIN = 34;

// State and timing
unsigned long triggerTime = 0;
unsigned long lastDebounceTime = 0;
unsigned long tdsZeroStartTime = 0;
unsigned long lastPumpCycleTime = 0;
bool tdsZeroTimerStarted = false;
bool triggerActivated = false;
bool heating = false;
bool cooling = false;
bool pumpCState = false;

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Interpolation helper
const int tempSteps[] = {0, 5, 10, 15, 20, 25, 30, 35};
const float tdsFactors[] = {0.50, 0.60, 0.70, 0.80, 0.90, 1.00, 1.10, 1.20};

float getExpectedRawTDS(float tdsAt25C, float tempC) {
  if (tempC <= 0) return tdsAt25C * tdsFactors[0];
  if (tempC >= 35) return tdsAt25C * tdsFactors[7];
  int i;
  for (i = 0; i < 7; i++) {
    if (tempC < tempSteps[i + 1]) break;
  }
  float t1 = tempSteps[i];
  float t2 = tempSteps[i + 1];
  float f1 = tdsFactors[i];
  float f2 = tdsFactors[i + 1];
  float factor = f1 + ((tempC - t1) / (t2 - t1)) * (f2 - f1);
  return tdsAt25C * factor;
}

// Read sensors
float readTDS() { return analogRead(TDS_PIN); }
float readTurbidity() { return analogRead(TURBIDITY_PIN); }
float readRefract() { return analogRead(REFRACT_PIN); }
float readTempTankA() { tempSensorA.requestTemperatures(); return tempSensorA.getTempCByIndex(0); }
float readTempTankC() { tempSensorC.requestTemperatures(); return tempSensorC.getTempCByIndex(0); }

// LCD helper
String lastLine0 = "";
String lastLine1 = "";

void updateLCD(String l0, String l1) {
  if (l0 != lastLine0) { lcd.setCursor(0,0); lcd.print("                "); lcd.setCursor(0,0); lcd.print(l0); lastLine0 = l0; }
  if (l1 != lastLine1) { lcd.setCursor(0,1); lcd.print("                "); lcd.setCursor(0,1); lcd.print(l1); lastLine1 = l1; }
}

// Tank C logic
void handleTankCTemperature(float tempC) {
  if (tempC < heatingThreshold) {
    digitalWrite(HEATER_RELAY, LOW);
    digitalWrite(PELTIER_RELAY, HIGH);
    digitalWrite(FAN_RELAY, HIGH);
    digitalWrite(PUMP_C_RELAY, LOW);
    heating = true;
    cooling = false;
  } else if (tempC > coolingThreshold) {
    digitalWrite(HEATER_RELAY, HIGH);
    digitalWrite(PELTIER_RELAY, LOW);
    digitalWrite(FAN_RELAY, LOW);
    heating = false;
    cooling = true;
    if (millis() - lastPumpCycleTime >= (pumpCState ? 5000 : 10000)) {
      pumpCState = !pumpCState;
      digitalWrite(PUMP_C_RELAY, pumpCState ? LOW : HIGH);
      lastPumpCycleTime = millis();
    }
  } else {
    digitalWrite(HEATER_RELAY, HIGH);
    digitalWrite(PELTIER_RELAY, HIGH);
    digitalWrite(FAN_RELAY, HIGH);
    heating = false;
    cooling = false;
    if (millis() - lastPumpCycleTime >= (pumpCState ? 120000 : 600000)) {
      pumpCState = !pumpCState;
      digitalWrite(PUMP_C_RELAY, pumpCState ? LOW : HIGH);
      lastPumpCycleTime = millis();
    }
  }
}

// Setup
void setup() {
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);
  lcd.init(); lcd.backlight();
  tempSensorC.begin();
  tempSensorA.begin();

  pinMode(TRIGGER_SWITCH_PIN, INPUT_PULLUP);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(PUMP_A_RELAY, OUTPUT);
  pinMode(PUMP_C_RELAY, OUTPUT);
  pinMode(HEATER_RELAY, OUTPUT);
  pinMode(PELTIER_RELAY, OUTPUT);
  pinMode(FAN_RELAY, OUTPUT);
}

// Main loop
void loop() {
  static bool lastTriggerState = LOW;
  bool reading = digitalRead(TRIGGER_SWITCH_PIN);

  if (reading != lastTriggerState) lastDebounceTime = millis();
  if ((millis()-lastDebounceTime) > debounceDelay && reading == LOW && !triggerActivated) {
    triggerActivated = true;
    triggerTime = millis();
  }
  lastTriggerState = reading;

  if (!triggerActivated) return;

  float tds = readTDS();
  float turb = readTurbidity();
  float refract = readRefract();
  float tempA = readTempTankA();
  float tempC = readTempTankC();

  float expectedTDSRaw = getExpectedRawTDS(BASE_TDS_PPM, tempA);

  // Decision at 15s
  if (millis() - triggerTime >= 15000 && digitalRead(PUMP_A_RELAY)==HIGH) {
    bool isPure = (tds <= expectedTDSRaw && turb >= turbidityMinADC && turb <= turbidityMaxADC && refract >= refractMinADC && refract <= refractMaxADC);
    if (isPure) {
      digitalWrite(GREEN_LED, HIGH);
      updateLCD("PURE DEF", "Pump Running");
    } else {
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BUZZER, HIGH);
      updateLCD("IMPURE DEF", "Abort Flow");
    }
    digitalWrite(PUMP_A_RELAY, LOW);
  }

  // Check TDS zero
  if (tds < tdsZeroThreshold) {
    if (!tdsZeroTimerStarted) { tdsZeroStartTime = millis(); tdsZeroTimerStarted = true; }
    if (millis()-tdsZeroStartTime >= tdsZeroDelay) {
      digitalWrite(PUMP_A_RELAY, HIGH);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BUZZER, LOW);
      triggerActivated = false;
      updateLCD("COMPLETE", "");
    }
  } else { tdsZeroTimerStarted = false; }

  if (millis()-triggerTime >= tempMonitoringDelay) handleTankCTemperature(tempC);
}
