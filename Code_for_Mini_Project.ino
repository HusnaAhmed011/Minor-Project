#define BLYNK_TEMPLATE_ID "TMPL3JNDXghUf"
#define BLYNK_TEMPLATE_NAME "Solar Piezo Hybrid Power Charging System"
#define BLYNK_AUTH_TOKEN "aLEwk5d7VIVQan-kqo-HqvmBZklIAzBT"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Blynk
BlynkTimer timer;
const char* ssid = "Galaxy A14";
const char* pass = "123456789";

float solarVoltage = 0;
float piezoVoltage = 0;
float batteryVoltage = 0;
float batteryEfficiency = 0;

int sourceMode = 0;  // 0 solar, 1 piezo, 2 rest

// Pin assignments
const int solarPin = 32;
const int piezoPin = 34;
const int batteryPin = 33;
const int relayPin = 25;

// State variables
bool usingSolar = true;
bool inRestState = false;

// Read voltage from 0–25V sensor
float readVoltage(int pin) {
    int raw = analogRead(pin);
    float volt = (raw / 4095.0) * 25.0;
    return volt;
}

// Battery efficiency mapping
float getBatteryEfficiency(float v) {
    if (v >= 12.6) return 100;
    if (v <= 9.6) return 0;
    return (v - 9.6) * (100.0 / (12.6 - 9.6));
}

// Read from Blynk switch (V0)
BLYNK_WRITE(V0) {
  sourceMode = param.asInt();
}

// Send data to app
void send_data() {
  Blynk.virtualWrite(V1, solarVoltage);
  Blynk.virtualWrite(V2, batteryVoltage);
  Blynk.virtualWrite(V3, batteryEfficiency);
}

void setup() {
    Wire.begin();
    lcd.init();
    lcd.backlight();

    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, HIGH); // default solar

    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    timer.setInterval(1000L, send_data);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" System Start ");
    delay(2000);
}

void loop() {

    Blynk.run();
    timer.run();

    float solar = readVoltage(solarPin);
    float piezo = readVoltage(piezoPin);
    float battery = readVoltage(batteryPin);
    float eff = getBatteryEfficiency(battery);

    solarVoltage = solar;
    piezoVoltage = piezo;
    batteryVoltage = battery;
    batteryEfficiency = eff;

    bool solarDead = (solar < 1.0);
    bool piezoDead = (piezo < 1.0);

    // ------------------------------------------------------
    // 1. SOURCE ON REST (NO SOLAR + NO PIEZO)
    // ------------------------------------------------------
    if (solarDead && piezoDead) {

        if (!inRestState) {
            inRestState = true;

            unsigned long endTime = millis() + 10000;
            while (millis() < endTime) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Source on Rest");
                delay(500);
                lcd.clear();
                delay(500);
            }
        }

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Volt=");
        lcd.print(battery, 2);

        lcd.setCursor(0, 1);
        lcd.print("EF=");
        lcd.print((int)eff);
        lcd.print("%");

        delay(1000);
        return;
    }

    inRestState = false;

    // ------------------------------------------------------
    // 2. NORMAL OPERATION WITH SOLAR / PIEZO SWITCHING
    // ------------------------------------------------------
    if (solar >= 8.0 && !usingSolar) {
        usingSolar = true;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Solar Energy");
        digitalWrite(relayPin, HIGH);
        delay(10000);
    }
    else if (solar < 8.0 && usingSolar && !piezoDead) {
        usingSolar = false;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Piezos Working");
        digitalWrite(relayPin, LOW);
        delay(10000);
    }

    // ------------------------------------------------------
    // 3. SHOW BATTERY EFFICIENCY ALWAYS
    // ------------------------------------------------------
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Volt=");
    lcd.print(battery, 2);

    lcd.setCursor(0, 1);
    lcd.print("EF=");
    lcd.print((int)eff);
    lcd.print("%");

    delay(1000);
}
