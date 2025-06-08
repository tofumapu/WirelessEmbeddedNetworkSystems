#define ENA 7 // PWM for Left Motor (Note: Pin 7 is not a standard PWM pin on Uno, consider pins 3, 5, 6, 9, 10, 11)
#define ENB 2 // PWM for Right Motor (Note: Pin 2 is not a standard PWM pin on Uno, consider pins 3, 5, 6, 9, 10, 11)
#define IN_1 6 // L298N in1 Left Motor
#define IN_2 5 // L298N in2 Left Motor
#define IN_3 4 // L298N in3 Right Motor
#define IN_4 3 // L298N in4 Right Motor

#define TRIG_PIN A0
#define ECHO_PIN A1

#include <Servo.h>
#define SERVO_PIN 8
Servo radarServo;
int servoAngle = 90;

String command;
int speedCar = 170;
int turnSpeed = 170;
int curveSpeedFast = 170;
int curveSpeedSlow = 103;

float distanceCm = 0;
float distanceLeftScan = 0;
float distanceRightScan = 0;

#define OBSTACLE_THRESHOLD 15
#define SERVO_SCAN_LEFT_ANGLE 160
#define SERVO_SCAN_RIGHT_ANGLE 20
#define SERVO_CENTER_ANGLE 90

bool obstacleAvoidanceMode = false;

void goAhead() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  analogWrite(ENA, speedCar);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);
  analogWrite(ENB, speedCar);
}
void goBack() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  analogWrite(ENA, speedCar);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);
  analogWrite(ENB, speedCar);
}
void goLeft() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  analogWrite(ENA, turnSpeed);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);
  analogWrite(ENB, turnSpeed);
}
void goRight() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  analogWrite(ENA, turnSpeed);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);
  analogWrite(ENB, turnSpeed);
}
void goAheadLeft() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  analogWrite(ENA, curveSpeedSlow);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);
  analogWrite(ENB, curveSpeedFast);
}
void goAheadRight() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  analogWrite(ENA, curveSpeedFast);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);
  analogWrite(ENB, curveSpeedSlow);
}
void goBackLeft() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  analogWrite(ENA, curveSpeedSlow);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);
  analogWrite(ENB, curveSpeedFast);
}
void goBackRight() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  analogWrite(ENA, curveSpeedFast);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);
  analogWrite(ENB, curveSpeedSlow);
}
void stopRobot() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, LOW);
  analogWrite(ENA, 0);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, LOW);
  analogWrite(ENB, 0);
}

int ultrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(4);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  long cm = duration / 58;
  return cm;
}

void setServoAngle(int angle) {
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;
  radarServo.write(angle);
  servoAngle = angle;
  delay(15);
}
void ObstacleAvoidance() {
  setServoAngle(SERVO_CENTER_ANGLE);
  distanceCm = ultrasonic();
  Serial.print("Distance: ");
  Serial.print(distanceCm);
  Serial.println(" cm");

  if (distanceCm <= OBSTACLE_THRESHOLD) {
    Serial.println("Obstacle detected! Initiating avoidance maneuvers.");
    stopRobot(); delay(100);
    goBack(); delay(300);
    stopRobot(); delay(100);
    setServoAngle(SERVO_SCAN_LEFT_ANGLE); delay(500);
    distanceLeftScan = ultrasonic();
    Serial.print("Left Scan Distance: "); Serial.println(distanceLeftScan);
    setServoAngle(SERVO_SCAN_RIGHT_ANGLE); delay(500);
    distanceRightScan = ultrasonic();
    Serial.print("Right Scan Distance: "); Serial.println(distanceRightScan);
    setServoAngle(SERVO_CENTER_ANGLE); delay(200);
    if (distanceLeftScan > distanceRightScan) {
      Serial.println("Turning left to avoid.");
      goLeft(); delay(500);
      stopRobot(); delay(100);
    } else if (distanceRightScan > distanceLeftScan) {
      Serial.println("Turning right to avoid.");
      goRight(); delay(500);
      stopRobot(); delay(100);
    } else {
      Serial.println("Both sides blocked or equal. Backing up more.");
      goRight(); delay(500);
      stopRobot(); delay(100);
    }
  } else {
    goAhead();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- Arduino Uno Car Controller ---");

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);
  pinMode(IN_3, OUTPUT);
  pinMode(IN_4, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  radarServo.attach(SERVO_PIN);
  setServoAngle(SERVO_CENTER_ANGLE);
  Serial.println("Servo initialized at 90 degrees.");

  Serial.println("Robot Ready!");
  Serial.println("---------------------------------------------");
  Serial.println("Send '1' to go forward, '0' to stop.");
  Serial.println("Send 'A' to enable Obstacle Avoidance mode.");
  Serial.println("Send 'D' to disable Obstacle Avoidance mode.");
}

void loop() {
  if (Serial.available()) {
    String receivedData = Serial.readStringUntil(' ');
    receivedData.trim();

    Serial.print("Command received: [");
    Serial.print(receivedData);
    Serial.println("]");

    if (receivedData == "A") {
      obstacleAvoidanceMode = true;
      Serial.println("Obstacle Avoidance Mode: ON");
    } else if (receivedData == "D") {
      obstacleAvoidanceMode = false;
      stopRobot();
      setServoAngle(SERVO_CENTER_ANGLE);
      Serial.println("Obstacle Avoidance Mode: OFF");
    }

    else if (!obstacleAvoidanceMode) {
      if (receivedData == "1") {
        Serial.println("Executing: goAhead");
        goAhead();
      } else if (receivedData == "8") {
        Serial.println("Executing: goBack");
        goBack();
      } else if (receivedData == "6") {
        Serial.println("Executing: goLeft");
        goLeft();
      } else if (receivedData == "3") {
        Serial.println("Executing: goRight");
        goRight();
      } else if (receivedData == "5") {
        Serial.println("Executing: goAheadLeft");
        goAheadLeft();
      } else if (receivedData == "2") {
        Serial.println("Executing: goAheadRight");
        goAheadRight();
      } else if (receivedData == "7") {
        Serial.println("Executing: goBackLeft");
        goBackLeft();
      } else if (receivedData == "4") {
        Serial.println("Executing: goBackRight");
        goBackRight();
      } else if (receivedData == "0") {
        Serial.println("Executing: stopRobot");
        stopRobot();
      } else {
        Serial.println("Unknown command or command ignored in auto mode.");
      }
    } else {
      Serial.println("Manual command ignored: Obstacle Avoidance mode is ON.");
    }
  }

  if (obstacleAvoidanceMode) {
    ObstacleAvoidance();
  } else {
    
  }
}