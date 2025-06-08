#include <WiFi.h>
#include <ESP32Servo.h>
#include <WebServer.h>

// Thông tin WiFi
#define WIFI_SSID "gimmesourcecode"
#define WIFI_PASSWORD "12345678"

// Radar
#define TRIG_PIN 5
#define ECHO_PIN 18
// Servo
#define SERVO_PIN 23

// Flame Sensor
#define FLAME_SENSOR_FRONT_LEFT_PIN  14
#define FLAME_SENSOR_FRONT_RIGHT_PIN 15
#define FLAME_SENSOR_MID_LEFT_PIN    25
#define FLAME_SENSOR_MID_RIGHT_PIN   22
#define FLAME_SENSOR_REAR_LEFT_PIN   2
#define FLAME_SENSOR_REAR_RIGHT_PIN  13

// Setting threshold
const float FIRE_TOO_CLOSE_CM = 10.0;
const float FIRE_MIN_SPRAY_DISTANCE_CM = 15.0;
const float FIRE_TARGET_SPRAY_DISTANCE_CM = 25.0;
const float FIRE_MAX_APPROACH_DISTANCE_CM = 40.0;

// Firebase status
volatile bool fireFightingModeEnabled = false;

// Firefighter status
enum FireFightingState {
    FF_IDLE,                       // Chờ phát hiện lửa
    FF_FIRE_DETECTED_STOP_AND_ASSESS,  // Dừng khi phát hiện lửa và đánh giá hướng
    FF_TURNING_TO_FIRE,            // Xoay về hướng lửa
    FF_ADJUSTING_DISTANCE,         // Điều chỉnh khoảng cách tới lửa (tiến/lùi)
    FF_SPRAYING                    // Bơm nước
};
WebServer server(80);


volatile bool manualControlOverride = false;

volatile float currentDistanceCm = 999.9;

// Servo status
Servo radarServo;
int servoAngle = 90;
const int SERVO_MIN_ANGLE = 0;
const int SERVO_MAX_ANGLE = 180;


volatile FireFightingState currentFfState = FF_IDLE;
unsigned long ffStateTimer = 0;
int fireDirectionAngle = 90;
const long SPRAY_DURATION_MS = 5000;
const long FF_TURN_TIMEOUT_MS = 8000;
const long FF_ADJUST_TIMEOUT_MS = 7000;

// Command to Arduino
const char CMD_STOP[] = "0";
const char CMD_FORWARD[] = "1";
const char CMD_TURN_RIGHT_HARD[] = "3";
const char CMD_TURN_LEFT_HARD[] = "6";
const char CMD_BACK[] = "8";
const char CMD_PUMP_ON[] = "P1";
const char CMD_PUMP_OFF[] = "P0";

// Function prototypes
void setServoToAngle(int targetAngle);
float calculateDistance();
void handleControl();
void setupServo();
void setupWiFi();

bool isFireDetected(int pin);
bool isFireDetectedFront();
bool isFireDetectedLeftSide();
bool isFireDetectedRightSide();
bool isFireDetectedRear();
void determineFireDirection();
void handleFireFightingLogic();
void sendMoveCommand(const char* cmd);
void sendPumpCommand(const char* cmd);
void printFlameSensorStatus();


// Implementations

bool isFireDetected(int pin) {
    return digitalRead(pin) == LOW;
}

void printFlameSensorStatus() {
    Serial.print("Flame Sensors: FL["); Serial.print(isFireDetected(FLAME_SENSOR_FRONT_LEFT_PIN) ? "1" : "0"); Serial.print("] ");
    Serial.print("FR["); Serial.print(isFireDetected(FLAME_SENSOR_FRONT_RIGHT_PIN) ? "1" : "0"); Serial.print("] ");
    Serial.print("ML["); Serial.print(isFireDetected(FLAME_SENSOR_MID_LEFT_PIN) ? "1" : "0"); Serial.print("] ");
    Serial.print("MR["); Serial.print(isFireDetected(FLAME_SENSOR_MID_RIGHT_PIN) ? "1" : "0"); Serial.print("] ");
    Serial.print("RL["); Serial.print(isFireDetected(FLAME_SENSOR_REAR_LEFT_PIN) ? "1" : "0"); Serial.print("] ");
    Serial.print("RR["); Serial.print(isFireDetected(FLAME_SENSOR_REAR_RIGHT_PIN) ? "1" : "0"); Serial.println("]");
}

bool isFireDetectedFront() {
    return isFireDetected(FLAME_SENSOR_FRONT_LEFT_PIN) || isFireDetected(FLAME_SENSOR_FRONT_RIGHT_PIN);
}

bool isFireDetectedLeftSide() {
    if (isFireDetected(FLAME_SENSOR_MID_LEFT_PIN)) return true;
    if (isFireDetected(FLAME_SENSOR_FRONT_LEFT_PIN) 
        && !isFireDetected(FLAME_SENSOR_FRONT_RIGHT_PIN)
        && !isFireDetected(FLAME_SENSOR_MID_RIGHT_PIN)) return true;
    return false;
}

bool isFireDetectedRightSide() {
    if (isFireDetected(FLAME_SENSOR_MID_RIGHT_PIN)) return true;
    if (isFireDetected(FLAME_SENSOR_FRONT_RIGHT_PIN) 
    && !isFireDetected(FLAME_SENSOR_FRONT_LEFT_PIN)
    && !isFireDetected(FLAME_SENSOR_MID_LEFT_PIN)) return true;
    return false;
}

bool isFireDetectedRear() {
    return isFireDetected(FLAME_SENSOR_REAR_LEFT_PIN) || isFireDetected(FLAME_SENSOR_REAR_RIGHT_PIN);
}

void determineFireDirection() {
    printFlameSensorStatus();

    bool frontFire = isFireDetectedFront();
    bool leftSideFire = isFireDetectedLeftSide();
    bool rightSideFire = isFireDetectedRightSide();
    bool rearFire = isFireDetectedRear();

    if (frontFire) {
        fireDirectionAngle = 90;
    } else if (leftSideFire && !rightSideFire && !rearFire) {
        fireDirectionAngle = 45;
    } else if (rightSideFire && !leftSideFire && !rearFire) {
        fireDirectionAngle = 135;
    } else if (rearFire && !leftSideFire && !rightSideFire) {
        fireDirectionAngle = 0;
    } else if (leftSideFire && rightSideFire && !rearFire) { 
        fireDirectionAngle = 45;
    }
    else if (frontFire && leftSideFire && !rightSideFire) {
        fireDirectionAngle = 75;
    } else if (frontFire && rightSideFire && !leftSideFire) {
        fireDirectionAngle = 105;
    }
    else {
        fireDirectionAngle = -1;
    }
}

void sendMoveCommand(const char* cmd) { // Tín hiệu di chuyển
    Serial.print("Sending Move Cmd to Arduino (Serial2): "); Serial.println(cmd);
    Serial2.print(cmd); Serial2.print(" ");
}

void sendPumpCommand(const char* cmd) { // Tín hiệu bơm nước
    Serial.print("Sending Pump Cmd to Arduino (Serial2): "); Serial.println(cmd);
    Serial2.print(cmd); Serial2.print(" ");
}

void handleFireFightingLogic() {
    unsigned long currentMillis = millis();
    setServoToAngle(90);
    currentDistanceCm = calculateDistance();

    switch (currentFfState) {
        case FF_IDLE:
            sendPumpCommand(CMD_PUMP_OFF);
            if (isFireDetectedFront() || isFireDetectedLeftSide() || isFireDetectedRightSide() || isFireDetectedRear()) {
                sendMoveCommand(CMD_STOP);
                currentFfState = FF_FIRE_DETECTED_STOP_AND_ASSESS;
                ffStateTimer = millis();
            }
            break;

        case FF_FIRE_DETECTED_STOP_AND_ASSESS:
            sendMoveCommand(CMD_STOP);
            sendPumpCommand(CMD_PUMP_OFF);
            if (currentMillis - ffStateTimer > 700) {
                determineFireDirection();
                if (fireDirectionAngle == 90) {
                    currentFfState = FF_ADJUSTING_DISTANCE;
                } else if (fireDirectionAngle != -1) {
                    currentFfState = FF_TURNING_TO_FIRE;
                } else {
                    currentFfState = FF_IDLE;
                }
                ffStateTimer = millis();
            }
            break;

        case FF_TURNING_TO_FIRE:
            sendPumpCommand(CMD_PUMP_OFF);
            if (isFireDetectedFront()) {
                sendMoveCommand(CMD_STOP);
                currentFfState = FF_ADJUSTING_DISTANCE;
                ffStateTimer = millis();
                break;
            }

            if (currentMillis - ffStateTimer > FF_TURN_TIMEOUT_MS) {
                sendMoveCommand(CMD_STOP);
                currentFfState = FF_IDLE; 
                break;
            }

            if (fireDirectionAngle == 0 || (fireDirectionAngle > 0 && fireDirectionAngle < 88) ) {
                sendMoveCommand(CMD_TURN_LEFT_HARD);
            } else if (fireDirectionAngle > 92 && fireDirectionAngle <= 180) {
                sendMoveCommand(CMD_TURN_RIGHT_HARD);
            } else {
                sendMoveCommand(CMD_STOP);
                currentFfState = FF_FIRE_DETECTED_STOP_AND_ASSESS;
                ffStateTimer = millis();
            }
            break;

        case FF_ADJUSTING_DISTANCE:
            sendPumpCommand(CMD_PUMP_OFF);
            if (!isFireDetectedFront()) {
                sendMoveCommand(CMD_STOP);
                currentFfState = FF_FIRE_DETECTED_STOP_AND_ASSESS;
                ffStateTimer = millis();
                break;
            }
            if (currentDistanceCm < FIRE_TOO_CLOSE_CM) {
                sendMoveCommand(CMD_BACK);
            } else if (currentDistanceCm > FIRE_MAX_APPROACH_DISTANCE_CM) {
                currentFfState = FF_SPRAYING;
            } else if (currentDistanceCm > FIRE_TARGET_SPRAY_DISTANCE_CM + 3.0) {
                currentFfState = FF_SPRAYING;
                } else if (currentDistanceCm < FIRE_MIN_SPRAY_DISTANCE_CM && currentDistanceCm >= FIRE_TOO_CLOSE_CM) {
                 sendMoveCommand(CMD_BACK);
            }
            else if (currentDistanceCm >= FIRE_MIN_SPRAY_DISTANCE_CM && currentDistanceCm <= FIRE_TARGET_SPRAY_DISTANCE_CM + 3.0) {
                sendMoveCommand(CMD_STOP);
                currentFfState = FF_SPRAYING;
                ffStateTimer = millis();
            } else {
                 sendMoveCommand(CMD_STOP);
            }

            if (currentMillis - ffStateTimer > FF_ADJUST_TIMEOUT_MS && currentFfState == FF_ADJUSTING_DISTANCE) {
                sendMoveCommand(CMD_STOP);
                if(isFireDetectedFront()){
                    currentFfState = FF_SPRAYING;
                } else {
                    currentFfState = FF_FIRE_DETECTED_STOP_AND_ASSESS;
                }
                ffStateTimer = millis();
            }
            break;

        case FF_SPRAYING:
            sendMoveCommand(CMD_STOP); 
            if (!isFireDetectedFront()) {
                sendPumpCommand(CMD_PUMP_OFF);
                currentFfState = FF_IDLE;
                break;
            }

            sendPumpCommand(CMD_PUMP_ON);

            if (currentMillis - ffStateTimer > SPRAY_DURATION_MS) {
                sendPumpCommand(CMD_PUMP_OFF);
                if (isFireDetectedFront()) {
                    currentFfState = FF_FIRE_DETECTED_STOP_AND_ASSESS;
                } else {
                    currentFfState = FF_IDLE;
                }
                ffStateTimer = millis();
            }
            break;
    }
}

void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi for WebServer");
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime < 10000)) {
        Serial.print(".");
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected for WebServer!");
        Serial.print("IP Address: "); Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi STA connection failed! Starting AP mode as fallback for WebServer.");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("ESP32_FireBot_AP", "123456789");
        Serial.print("ESP32 AP SSID: ESP32_FireBot_AP, IP: "); Serial.println(WiFi.softAPIP());
    }
}

float calculateDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 25000);
    if (duration == 0 || duration >= 25000) return 999.9;
    
    float dist = duration * 0.0343 / 2.0;
    return (dist < 2.0) ? 999.9 : dist; 
}

void handleControl() {
    if (!server.hasArg("dir")) {
        server.send(400, "text/plain", "Missing 'dir' parameter");
        return;
    }
    String direction = server.arg("dir");

    if (direction == "FF_ON") {
        fireFightingModeEnabled = true;
        manualControlOverride = false;
        currentFfState = FF_IDLE;
        sendMoveCommand(CMD_STOP);
        sendPumpCommand(CMD_PUMP_OFF);
        Serial.println("Web CMD: Fire Fighting Mode ENABLED.");
        server.send(200, "text/plain", "Fire Fighting Mode ENABLED");
        return;
    } else if (direction == "FF_OFF") {
        fireFightingModeEnabled = false;
        manualControlOverride = true;
        sendMoveCommand(CMD_STOP);
        sendPumpCommand(CMD_PUMP_OFF);
        Serial.println("Web CMD: Fire Fighting Mode DISABLED. Manual control override ENABLED.");
        server.send(200, "text/plain", "Fire Fighting Mode DISABLED. Manual control override ENABLED.");
        return;
    }

    if (fireFightingModeEnabled && !manualControlOverride) {
        Serial.println("Fire Fighting Mode ON - Manual web command '" + direction + "' ignored. Use FF_OFF to enable manual control.");
        server.send(200, "text/plain", "FF Mode ON. Manual command '" + direction + "' ignored. Use /control?dir=FF_OFF to enable manual control.");
        return;
    }

    if (manualControlOverride) {
        Serial.print("Web Cmd Rcvd (Manual Override Active): "); Serial.println(direction);
        if (direction == "P1") {
            sendPumpCommand(CMD_PUMP_ON);
        } else if (direction == "P0") {
            sendPumpCommand(CMD_PUMP_OFF);
        } else {
            sendMoveCommand(direction.c_str());
        }
        String response = "OK (Manual Override): "; response += direction;
        server.send(200, "text/plain", response);
    } else {
       Serial.println("FF Mode is OFF but manual override is not active. Send /control?dir=FF_OFF first.");
       server.send(200, "text/plain", "FF Mode is OFF but manual override not active. Send /control?dir=FF_OFF to enable manual commands.");
    }
}

void setupServo() {
    radarServo.attach(SERVO_PIN, 500, 2400);
    setServoToAngle(90);
    Serial.println("Servo initialized at 90 degrees.");
}

void setServoToAngle(int targetAngle) {
    servoAngle = constrain(targetAngle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
    radarServo.write(servoAngle);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    Serial.println("\n--- ESP32 Fire Fighting Robot (Fixed Nozzle Logic v3) ---");

    Serial2.begin(115200, SERIAL_8N1, 16, 17);

    setupWiFi();
    setupServo();

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    pinMode(FLAME_SENSOR_FRONT_LEFT_PIN, INPUT_PULLUP);
    pinMode(FLAME_SENSOR_FRONT_RIGHT_PIN, INPUT_PULLUP);
    pinMode(FLAME_SENSOR_MID_LEFT_PIN, INPUT_PULLUP);
    pinMode(FLAME_SENSOR_MID_RIGHT_PIN, INPUT_PULLUP);
    pinMode(FLAME_SENSOR_REAR_LEFT_PIN, INPUT_PULLUP);
    pinMode(FLAME_SENSOR_REAR_RIGHT_PIN, INPUT_PULLUP);

    fireFightingModeEnabled = true;
    manualControlOverride = false;
    Serial.print("Initial FireFightingMode: "); Serial.println(fireFightingModeEnabled ? "ON" : "OFF");
    Serial.print("Initial ManualControlOverride: "); Serial.println(manualControlOverride ? "ON" : "OFF");

    server.on("/control", HTTP_GET, handleControl);
    server.begin();

    sendMoveCommand(CMD_STOP);
    sendPumpCommand(CMD_PUMP_OFF);
    currentFfState = FF_IDLE;
}

void loop() {
    if (WiFi.status() == WL_CONNECTED || WiFi.softAPgetStationNum() > 0) {
        server.handleClient();
    } else if (millis() % 20000 == 0) {
        if(WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi STA disconnected. Attempting reconnect for WebServer...");
            WiFi.reconnect();
        }
    }

    if (fireFightingModeEnabled && !manualControlOverride) {
        handleFireFightingLogic();
    } else if (manualControlOverride) {
        if(millis() % 3000 == 0) {
             Serial.println("System in MANUAL CONTROL OVERRIDE. Waiting for web commands via /control?dir=CMD");
        }
    } else {
        sendMoveCommand(CMD_STOP);
        sendPumpCommand(CMD_PUMP_OFF);
        if(millis() % 3000 == 0) {
            Serial.println("FF Mode OFF, Manual Override OFF. System IDLE. Send FF_ON or FF_OFF via web.");
        }
    }
    delay(50);
}