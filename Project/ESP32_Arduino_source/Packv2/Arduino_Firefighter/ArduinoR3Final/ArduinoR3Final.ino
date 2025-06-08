#define ENA  7  // PWM điều khiển tốc độ động cơ trái - Left
#define ENB  2  // PWM điều khiển tốc độ động cơ phải - Right
#define IN_1 6  // L298N in1 động cơ trái - Left
#define IN_2 5  // L298N in2 động cơ trái - Left
#define IN_3 4  // L298N in3 động cơ phải - Right
#define IN_4 3  // L298N in4 động cơ phải - Right
#define PUMP_PIN 8 // Chân điều khiển motor bơm nước
void pumpOn();
void pumpOff();

String command;
int speedCar = 255;
int turnSpeed = 200;
int curveSpeedFast = 255;
int curveSpeedSlow = 130;

void setup() {
    pinMode(ENA, OUTPUT);
    pinMode(ENB, OUTPUT);
    pinMode(IN_1, OUTPUT);
    pinMode(IN_2, OUTPUT);
    pinMode(IN_3, OUTPUT);
    pinMode(IN_4, OUTPUT);
    pinMode(PUMP_PIN, OUTPUT);

    pumpOff();

    Serial.begin(115200);
    Serial.println("Robot Ready! (with Pump Control)");
}

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

void pumpOn() {
    digitalWrite(PUMP_PIN, LOW);
    Serial.println("Pump ON");
}

void pumpOff() {
    digitalWrite(PUMP_PIN, HIGH);
    Serial.println("Pump OFF");
}

void loop() {
    if (Serial.available()) {
        String receivedData = Serial.readStringUntil(' ');
        receivedData.trim();

        Serial.print("Command received: [");
        Serial.print(receivedData);
        Serial.println("]");

        if (receivedData == "1") {
            Serial.println("Executing: goAhead");
            goAhead();
        } else if (receivedData == "8") {
            Serial.println("Executing: goBack");
            goBack();
        } else if (receivedData == "6") {
            Serial.println("Executing: goLeft (Hard)");
            goLeft();
        } else if (receivedData == "3") {
            Serial.println("Executing: goRight (Hard)");
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
        }
        else if (receivedData == "P1") {
             Serial.println("Executing: pumpOn");
             pumpOn();
        } else if (receivedData == "P0") {
             Serial.println("Executing: pumpOff");
             pumpOff();
        }
        else {
            Serial.println("Unknown command");
        }
    }
}