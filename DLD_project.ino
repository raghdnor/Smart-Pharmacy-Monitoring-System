#include <DHT.h>
#include <Servo.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "HX711.h"

// PIN DEFINITIONS
#define DHTPIN 4
#define DHTTYPE DHT22
#define MQ2PIN A0
#define LDRPIN A1
#define SERVO_PIN 9
#define LED_TEMP 5
#define LED_HUM 6
#define LED_SMOKE 7
#define BUZZER 8
#define LOADCELL_DOUT 2
#define LOADCELL_SCK 3

// THRESHOLDS
const int SMOKE_THRESHOLD = 400; // Adjust based on environment
const float HUMIDITY_LIMIT = 70.0;
const char* SECRET_CODE = "1234";

// INITIALIZATION
DHT dht(DHTPIN, DHTTYPE);
Servo doorServo;
LiquidCrystal_I2C lcd(0x27, 16, 2); // Address 0x27 or 0x3F
HX711 scale;

// KEYPAD SETUP
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {22, 23, 24, 25};
byte colPins[COLS] = {26, 27, 28, 29};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String inputCode = "";
bool smokeActive = false;

void setup() {
  Serial.begin(9600);
  dht.begin();
  doorServo.attach(SERVO_PIN);
  doorServo.write(0); // Door Closed
  
  pinMode(LED_TEMP, OUTPUT);
  pinMode(LED_HUM, OUTPUT);
  pinMode(LED_SMOKE, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  lcd.init();
  lcd.backlight();
  
  scale.begin(LOADCELL_DOUT, LOADCELL_SCK);
  scale.set_scale(420.0); // Calibrate this value for your specific load cell
  scale.tare();           // Reset scale to 0
  
  lcd.print("System Ready");
  delay(1000);
}

void loop() {
  // 1. READ SENSORS
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int smokeVal = analogRead(MQ2PIN);
  long weight = scale.get_units(5); // Average of 5 readings
  if (weight < 0) weight = 0;

  // 2. SMOKE LOGIC (Priority)
  if (smokeVal > SMOKE_THRESHOLD) {
    smokeActive = true;
    digitalWrite(LED_SMOKE, HIGH);
    digitalWrite(BUZZER, HIGH);
    doorServo.write(90); // Emergency Open
    lcd.setCursor(0, 0);
    lcd.print("SMOKE DETECTED!");
  } else {
    smokeActive = false;
    digitalWrite(LED_SMOKE, LOW);
    // Buzzer might still be on due to high temp, handled below
  }

  // 3. TEMPERATURE & HUMIDITY LOGIC
  // Temperature LED
  if (t > 30) digitalWrite(LED_TEMP, HIGH);
  else digitalWrite(LED_TEMP, LOW);

  // Humidity LED
  if (h > HUMIDITY_LIMIT) digitalWrite(LED_HUM, HIGH);
  else digitalWrite(LED_HUM, LOW);

  // Temperature Buzzer (Above 35)
  if (t > 35 || smokeActive) digitalWrite(BUZZER, HIGH);
  else digitalWrite(BUZZER, LOW);

  // 4. WEIGHT LOGIC (LCD Display)
  lcd.setCursor(0, 1);
  if (weight < 100) lcd.print("W: Empty   ");
  else if (weight < 1000) lcd.print("W: Low     ");
  else if (weight < 4000) lcd.print("W: Good    ");
  else lcd.print("W: Max     ");

  // 5. KEYPAD LOGIC (Door Control)
  char key = keypad.getKey();
  if (key) {
    if (key == '#') { // Press # to submit
      if (inputCode == SECRET_CODE) {
        if (!smokeActive) {
          lcd.setCursor(0,0);
          lcd.print("Access Granted ");
          doorServo.write(90);
          delay(3000);
          doorServo.write(0);
        }
      } else {
        lcd.setCursor(0,0);
        lcd.print("Wrong Password ");
        delay(1000);
      }
      inputCode = "";
    } else if (key == '*') { // Clear
      inputCode = "";
    } else {
      inputCode += key;
    }
  }

  // Debugging to Serial Monitor
  Serial.print("Temp: "); Serial.print(t);
  Serial.print(" Hum: "); Serial.print(h);
  Serial.print(" Smoke: "); Serial.println(smokeVal);
  
  delay(2000); // Small stability delay
}