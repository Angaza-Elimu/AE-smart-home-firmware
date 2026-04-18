#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <Keypad.h>

// Pin Definitions
const int PUSH_BUTTON_PIN = 4;
const int WATER_LEVEL_PIN = 5;
const int LDR_PIN = 6; // Light Sensor
const int DHT_PIN = 7;
const int ONBOARD_LED_PIN = 9;
const int BUZZER_PIN = 15;
const int INA_PIN = 38;
const int INB_PIN = 35;
const int MQ2_PIN = 34; // Smoke Sensor
const int PIR_PIN = 39;
const int SERVO_PIN_1 = 20; // Door Servo
const int SERVO_PIN_2 = 3;  // Window Servo
const int RED_PIN = 16;
const int BLUE_PIN = 19;
const int GREEN_PIN = 33;
const int I2C_SCL = 2;
const int I2C_SDA = 42;

// OLED Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // Change this to 0x3D if your display uses that address

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// DHT Sensor
#define DHT_SENSOR_TYPE DHT22
DHT dht_sensor(DHT_PIN, DHT_SENSOR_TYPE);

// PIR Sensor
volatile bool motionDetected = false;

// Servo Motors
Servo doorServo;
Servo windowServo;

// Keypad Setup
#define ROW_NUM     4 // four rows
#define COLUMN_NUM  4 // four columns

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// connect the pins from right to left to pins 1, 10, 9, 11, 12, 14, 21, 34
byte pin_rows[ROW_NUM] = {11,9,10,1}; //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {34,21,14,12}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

const String password = "7890"; // change your password here
String input_password;

// Interrupt for Motion Detection
void IRAM_ATTR handleMotion() {
  motionDetected = digitalRead(PIR_PIN);
}

// Function to update OLED display with keypad input
void updateOLEDWithKeypad(String input) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Keypad Input: ");
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print(input);
  display.display();
}

// Function to sound the buzzer with a specific pattern
void soundBuzzer(int beepDuration, int beepCount, int delayBetweenBeeps) {
  for (int i = 0; i < beepCount; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(beepDuration);
    digitalWrite(BUZZER_PIN, LOW);
    delay(delayBetweenBeeps);
  }
}

// Function to display "Access Granted" on OLED
void displayAccessGranted() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("Access");
  display.setCursor(0, 30);
  display.print("Granted!");
  display.display();
  delay(3000); // Keep "Access Granted" on the screen for 3 seconds
}

// Function to display "Access Denied" on OLED
void displayAccessDenied() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("Access");
  display.setCursor(0, 30);
  display.print("Denied!");
  display.display();
  delay(3000); // Keep "Access Denied" on the screen for 3 seconds
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  // Initialize OLED Display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Sensor & Actuator Setup
  pinMode(INA_PIN, OUTPUT);
  pinMode(INB_PIN, OUTPUT);
  dht_sensor.begin();

  pinMode(PIR_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(MQ2_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // Attach Servos
  doorServo.attach(SERVO_PIN_1);
  windowServo.attach(SERVO_PIN_2);
  doorServo.write(0);
  windowServo.write(0);

  attachInterrupt(digitalPinToInterrupt(PIR_PIN), handleMotion, CHANGE);

  // Keypad Setup
  input_password.reserve(32); // maximum input characters is 33, change if needed
}

void loop() {
  static unsigned long lastUpdate = 0;
  const unsigned long updateInterval = 5000; // Update every 5 seconds

  // Update sensors and OLED every updateInterval milliseconds
  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();

    // Read Temperature & Humidity
    float humi = dht_sensor.readHumidity();
    float tempC = dht_sensor.readTemperature();

    if (isnan(tempC) || isnan(humi)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Fan Control
    if (tempC > 30) {
      digitalWrite(INA_PIN, LOW);
      digitalWrite(INB_PIN, HIGH);
      Serial.println("Fan spinning");
    } else {
      digitalWrite(INA_PIN, LOW);
      digitalWrite(INB_PIN, LOW);
      Serial.println("Fan stopped");
    }

    // OLED Display Update
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Temperature: ");
    display.setTextSize(2);
    display.setCursor(0, 16);
    display.print(tempC);
    display.print(" C");

    display.setTextSize(1);
    display.setCursor(0, 35);
    display.print("Humidity: ");
    display.setTextSize(2);
    display.setCursor(0, 45);
    display.print(humi);
    display.print(" %");

    display.display();

    // Motion Detection
    if (motionDetected) {
      Serial.println("Motion detected!");
      digitalWrite(RED_PIN, HIGH);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, LOW);
    } else {
      Serial.println("Motion stopped!");
      digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, LOW);
    }

    // Smoke Sensor (MQ2)
    int gasLevel = analogRead(MQ2_PIN);
    Serial.print("Gas Level: ");
    Serial.println(gasLevel);

    if (gasLevel > 1000) {
      Serial.println("Gas detected! Buzzer ON");
      digitalWrite(BUZZER_PIN, HIGH); // Buzzer ON
    } else {
      Serial.println("No gas detected.");
      digitalWrite(BUZZER_PIN, LOW); // Buzzer OFF
    }

    // Water Level & Servo Control
    int waterState = digitalRead(WATER_LEVEL_PIN);
    if (waterState == HIGH) {
      Serial.println("Water detected! Closing window...");
      windowServo.write(180);
    } else {
      Serial.println("No water detected. Opening window...");
      windowServo.write(0);
    }

    // Light Sensor (LDR)
    int lightValue = analogRead(LDR_PIN);
    Serial.print("Light Level: ");
    Serial.println(lightValue);

    if (lightValue < 10) { // Adjust threshold as needed
      Serial.println("It's dark! Turning RGB LED green.");
      digitalWrite(RED_PIN, LOW);
      digitalWrite(BLUE_PIN, LOW);
      digitalWrite(GREEN_PIN, HIGH);
    } else {
      Serial.println("It's bright! Turning RGB LED off.");
      digitalWrite(RED_PIN, LOW);
      digitalWrite(BLUE_PIN, LOW);
      digitalWrite(GREEN_PIN, LOW);
    }
  }

  // Keypad Logic
  char key = keypad.getKey();

  if (key) {
    Serial.println(key);

    if (key == '*') {
      input_password = ""; // clear input password
      updateOLEDWithKeypad(input_password);
    } else if (key == '#') {
      if (password == input_password) {
        Serial.println("The password is correct, ACCESS GRANTED!");
        soundBuzzer(500, 2, 200); // Two long beeps for access granted
        displayAccessGranted(); // Display "Access Granted" on OLED
      } else {
        Serial.println("The password is incorrect, ACCESS DENIED!");
        soundBuzzer(200, 3, 100); // Three short beeps for access denied
        displayAccessDenied(); // Display "Access Denied" on OLED
      }
      input_password = ""; // clear input password
      updateOLEDWithKeypad(input_password);
    } else {
      input_password += key; // append new character to input password string
      updateOLEDWithKeypad(input_password);
    }
  }
}
