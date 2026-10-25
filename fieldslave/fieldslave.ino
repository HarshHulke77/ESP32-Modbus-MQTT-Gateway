// ==========================================
// Arduino Nano + TB6612FNG Motor Test
// ==========================================

// LEFT MOTOR
#define PWMA 11
#define AIN1 12
#define AIN2 13

// RIGHT MOTOR
#define PWMB A0
#define BIN1 A1
#define BIN2 A2

void setup() {

  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);

  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  stopMotors();

  delay(2000);
}

void loop() {

  // -----------------------------
  // 1. LEFT MOTOR FORWARD
  // -----------------------------
  leftMotor(120);
  delay(2000);

  stopMotors();
  delay(1000);


  // -----------------------------
  // 2. LEFT MOTOR REVERSE
  // -----------------------------
  leftMotor(-120);
  delay(2000);

  stopMotors();
  delay(1000);


  // -----------------------------
  // 3. RIGHT MOTOR FORWARD
  // -----------------------------
  rightMotor(120);
  delay(2000);

  stopMotors();
  delay(1000);


  // -----------------------------
  // 4. RIGHT MOTOR REVERSE
  // -----------------------------
  rightMotor(-120);
  delay(2000);

  stopMotors();
  delay(1000);


  // -----------------------------
  // 5. BOTH FORWARD
  // -----------------------------
  leftMotor(120);
  rightMotor(120);
  delay(2000);

  stopMotors();
  delay(1000);


  // -----------------------------
  // 6. BOTH REVERSE
  // -----------------------------
  leftMotor(-120);
  rightMotor(-120);
  delay(2000);

  stopMotors();
  delay(2000);
}


// ==========================================
// LEFT MOTOR
// speed: -255 to +255
// ==========================================

void leftMotor(int speed) {

  speed = constrain(speed, -255, 255);

  if (speed > 0) {

    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);

    analogWrite(PWMA, speed);

  }
  else if (speed < 0) {

    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);

    analogWrite(PWMA, -speed);

  }
  else {

    analogWrite(PWMA, 0);

    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
  }
}


// ==========================================
// RIGHT MOTOR
// speed: -255 to +255
// ==========================================

void rightMotor(int speed) {

  speed = constrain(speed, -255, 255);

  if (speed > 0) {

    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);

    analogWrite(PWMB, speed);

  }
  else if (speed < 0) {

    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);

    analogWrite(PWMB, -speed);

  }
  else {

    analogWrite(PWMB, 0);

    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, LOW);
  }
}


// ==========================================
// STOP BOTH MOTORS
// ==========================================

void stopMotors() {

  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);

  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);

  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
}