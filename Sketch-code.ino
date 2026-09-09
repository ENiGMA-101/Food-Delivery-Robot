#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// 16x2 I2C LCD Setup (Address 0x27)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// L298N Motor Driver Pins (আপনার দেওয়া পিন অনুযায়ী)
const int IN1 = 4;
const int IN2 = 5;
const int IN3 = 6;
const int IN4 = 7;
const int ENA = 15;
const int ENB = 16;

// 5x IR Line Sensor Pins (আপনার দেওয়া পিন অনুযায়ী)
const int IR_FAR_LEFT   = 1;
const int IR_LEFT       = 2;
const int IR_CENTER     = 3;
const int IR_RIGHT      = 10;
const int IR_FAR_RIGHT  = 11;

// HC-SR04 Ultrasonic Sensor Pins (আপনার দেওয়া পিন অনুযায়ী)
const int TRIG_PIN = 12;
const int ECHO_PIN = 13;

// Safety & Speed Thresholds
const int SAFE_DISTANCE = 15; // cm
const int MOTOR_SPEED   = 180;

void setup() {
  // Serial Monitor Initialization
  Serial.begin(115200);
  delay(1000);
  Serial.println("==========================================");
  Serial.println("   ESP32-S3 ROBOT SYSTEM DIAGNOSTICS      ");
  Serial.println("==========================================");

  // ESP32-S3 I2C Pins (SDA = GPIO 8, SCL = GPIO 9)
  Wire.begin(8, 9);
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print(" Diagnostic Mode");
  lcd.setCursor(0, 1);
  lcd.print("Check Serial Mon");

  // Motor Pins Configuration
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  // 5x IR Pins Configuration
  pinMode(IR_FAR_LEFT, INPUT);
  pinMode(IR_LEFT, INPUT);
  pinMode(IR_CENTER, INPUT);
  pinMode(IR_RIGHT, INPUT);
  pinMode(IR_FAR_RIGHT, INPUT);

  // Ultrasonic Pins Configuration
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  stopMotors();
  Serial.println("[SYSTEM SETUP COMPLETED]");
  Serial.println("Starting Real-Time Sensor & Motor Monitoring...\n");
}

void loop() {
  // ১. আল্ট্রাসোনিক সেন্সর রিডিং
  long distance = readUltrasonicDistance();

  // ২. ৫টি IR সেন্সর রিডিং
  int sFarLeft  = digitalRead(IR_FAR_LEFT);
  int sLeft     = digitalRead(IR_LEFT);
  int sCenter   = digitalRead(IR_CENTER);
  int sRight    = digitalRead(IR_RIGHT);
  int sFarRight = digitalRead(IR_FAR_RIGHT);

  String motorState = "STOPPED";
  String pathAction = "IDLE";

  // ৩. লজিক চেক ও মোটর ড্রাইভিং
  if (distance > 0 && distance <= SAFE_DISTANCE) {
    stopMotors();
    motorState = "STOPPED (OBSTACLE DETECTED)";
    pathAction = "BRAKE APPLIED";

    lcd.setCursor(0, 0);
    lcd.print("Status: OBSTACLE");
    lcd.setCursor(0, 1);
    lcd.print("Dist: ");
    lcd.print(distance);
    lcd.print(" cm   ");
  } 
  else {
    // সব সেন্সর হাই হলে (Junction / Destination)
    if (sFarLeft == HIGH && sLeft == HIGH && sCenter == HIGH && sRight == HIGH && sFarRight == HIGH) {
      stopMotors();
      motorState = "STOPPED (AT JUNCTION)";
      pathAction = "TABLE REACHED";

      lcd.setCursor(0, 0);
      lcd.print("Status: JUNCTION");
      lcd.setCursor(0, 1);
      lcd.print("Table Reached  ");
    }
    // শুধু মাঝখানের সেন্সর লাইনে থাকলে সোজা চলবে
    else if (sCenter == HIGH && sLeft == LOW && sRight == LOW) {
      moveForward();
      motorState = "FORWARD";
      pathAction = "FOLLOWING LINE";

      lcd.setCursor(0, 0);
      lcd.print("Status: MOVING ");
      lcd.setCursor(0, 1);
      lcd.print("Path: FORWARD  ");
    }
    // সামান্য বামে সরলে সংশোধন
    else if (sLeft == HIGH && sCenter == HIGH) {
      turnLeft();
      motorState = "TURNING LEFT";
      pathAction = "CORRECTING LEFT";

      lcd.setCursor(0, 0);
      lcd.print("Status: MOVING ");
      lcd.setCursor(0, 1);
      lcd.print("Path: LEFT     ");
    }
    // সামান্য ডানে সরলে সংশোধন
    else if (sRight == HIGH && sCenter == HIGH) {
      turnRight();
      motorState = "TURNING RIGHT";
      pathAction = "CORRECTING RIGHT";

      lcd.setCursor(0, 0);
      lcd.print("Status: MOVING ");
      lcd.setCursor(0, 1);
      lcd.print("Path: RIGHT    ");
    }
    // তীব্র বামে মোড় (Sharp Left)
    else if (sFarLeft == HIGH || sLeft == HIGH) {
      turnLeft();
      motorState = "SHARP LEFT";
      pathAction = "HARD TURN LEFT";

      lcd.setCursor(0, 0);
      lcd.print("Status: MOVING ");
      lcd.setCursor(0, 1);
      lcd.print("Path: SHARP LEFT");
    }
    // তীব্র ডানে মোড় (Sharp Right)
    else if (sFarRight == HIGH || sRight == HIGH) {
      turnRight();
      motorState = "SHARP RIGHT";
      pathAction = "HARD TURN RIGHT";

      lcd.setCursor(0, 0);
      lcd.print("Status: MOVING ");
      lcd.setCursor(0, 1);
      lcd.print("Path: SHARP RIGHT");
    }
    // লাইন না পেলে স্টপ
    else {
      stopMotors();
      motorState = "STOPPED";
      pathAction = "NO LINE DETECTED";

      lcd.setCursor(0, 0);
      lcd.print("Status: SEARCHING");
      lcd.setCursor(0, 1);
      lcd.print("No Line Found  ");
    }
  }

  // --- SERIAL MONITOR DETAILED PRINTING ---
  Serial.println("------------------ SENSOR STATUS ------------------");
  Serial.print("IR Sensors [FL | L | C | R | FR] : ");
  Serial.print("[ ");
  Serial.print(sFarLeft);   Serial.print(" | ");
  Serial.print(sLeft);      Serial.print(" | ");
  Serial.print(sCenter);    Serial.print(" | ");
  Serial.print(sRight);     Serial.print(" | ");
  Serial.print(sFarRight);  Serial.println(" ]");

  Serial.print("Ultrasonic Distance        : ");
  if (distance == 999) {
    Serial.println("Out of Range (> 400 cm)");
  } else {
    Serial.print(distance);
    Serial.println(" cm");
  }

  Serial.print("Motor Status               : ");
  Serial.println(motorState);
  Serial.print("Current Robot Action       : ");
  Serial.println(pathAction);
  Serial.println("---------------------------------------------------\n");

  delay(50); // স্মুথ রেসপন্সের জন্য ৫০ মিলিসেকেন্ড ডিলে
}

// ----------------- Helper Functions -----------------

long readUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999;
  return (duration * 0.034) / 2;
}

void moveForward() {
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnLeft() {
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight() {
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
