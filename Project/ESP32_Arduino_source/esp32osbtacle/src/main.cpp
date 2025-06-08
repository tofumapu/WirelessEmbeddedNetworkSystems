// Radar HC-SR04
#define TRIG_PIN 5
#define ECHO_PIN 18

// Servo
#define SERVO_PIN 23 

// IR Sensors
#define IR_LEFT_PIN 26
#define IR_RIGHT_PIN 27


#include <WiFi.h>
#include <ESP32Servo.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// WiFi info
#define WIFI_SSID "gimmesourcecode"
#define WIFI_PASSWORD "12345678"

// Firebase Project info
#define API_KEY "AIzaSyD2RvzEQfQexsIyIp-BF3nHIZHY5lkVG7w"
#define DATABASE_URL "esp32-car-6552a-default-rtdb.firebaseio.com"

// WebServer
#include <WebServer.h>
WebServer server(80);

// Firebase Object
FirebaseData fbdo;
FirebaseData stream_fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson jsonData;

void connectFirebase();

// Servo setup
Servo radarServo;
int servoAngle = 90;


float currentDistanceCm = 0.0;
unsigned long lastFirebaseUpdateMillis = 0;
const unsigned long firebaseUpdateInterval = 1500;


// Motor Commands
#define CMD_FORWARD "1"
#define CMD_BACKWARD "8"
#define CMD_LEFT "6"
#define CMD_RIGHT "3"
#define CMD_STOP "0"
#define CMD_LEFT_CURVE "5"
#define CMD_RIGHT_CURVE "2"

// Switch controls
volatile bool obstacleAvoidanceEnabled = false;
volatile bool followObjectModeEnabled = false;
volatile bool FireFighterModeEnabled = false;

// Obstacle Avoidance Parameters
#define OBSTACLE_THRESHOLD_FRONT 20.0f
#define OBSTACLE_THRESHOLD_SIDE 25.0f
#define OA_SHORT_STOP_DURATION 100
#define OA_BACKWARD_DURATION 300 
#define OA_SERVO_SCAN_DELAY 600
#define OA_TURN_DURATION 700
#define OA_SCAN_ANGLE_LEFT 160
#define OA_SCAN_ANGLE_RIGHT 20
#define OA_SERVO_CENTER 90

// State for Obstacle Avoidance
enum ObstacleAvoidanceState {
    OA_IDLE_FORWARD,
    OA_STOPPED_OBSTACLE,
    OA_MOVING_BACKWARD,
    OA_SCANNING_LEFT,
    OA_SCANNING_RIGHT,
    OA_DECIDING_TURN,
    OA_TURNING_LEFT,
    OA_TURNING_RIGHT
};
ObstacleAvoidanceState currentOaState = OA_IDLE_FORWARD;
unsigned long oaActionStartTime = 0;
float distanceLeftScan = 999.0, distanceRightScan = 999.0;

// Object Following Parameters
#define FOLLOW_TOO_CLOSE_LIMIT 10.0f
#define FOLLOW_STOP_RANGE_MIN 15.0f
#define FOLLOW_STOP_RANGE_MAX 25.0f
#define FOLLOW_MAX_ENGAGE_DISTANCE 50.0f
#define FOLLOW_IR_TURN_COMMIT_DURATION 250
unsigned long foActionStartTime = 0;
bool foWasTurningOnIr = false;


// Function
void sendMotorCommand(const char* cmd) {
    Serial2.print(String(cmd) + " ");
    Serial.print("Motor CMD -> Arduino: "); Serial.println(cmd);
}

void setServoToAngle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    radarServo.write(angle);
    servoAngle = angle;
}

float calculateDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration == 0 || duration >= 30000) return 999.9;
    return duration * 0.0343 / 2.0;
}

void readIrSensors(bool &leftDetected, bool &rightDetected) {
    leftDetected = (digitalRead(IR_LEFT_PIN) == LOW);
    rightDetected = (digitalRead(IR_RIGHT_PIN) == LOW);
}

void connectFirebase() {
    Serial.println("Attempting to connect to Firebase...");
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    if (Firebase.signUp(&config, &auth, "", "")) {
        Serial.println("Firebase sign-in successful!");
    } else {
        Serial.printf("Firebase sign-in failed: %s\n", config.signer.signupError.message.c_str());
    }
}

void handleControl() {
    if (server.hasArg("dir")) {
        if (followObjectModeEnabled || obstacleAvoidanceEnabled) {
            Serial.println("AUTO Mode ON - Web control ignored.");
            server.send(200, "text/plain", "AUTO Mode Active. Web control ignored.");
            return;
        }
        String direction = server.arg("dir");
        Serial.print("Web Cmd Rcvd: "); Serial.println(direction);
        sendMotorCommand(direction.c_str());
        server.send(200, "text/plain", "OK: " + direction);
    } else {
        server.send(400, "text/plain", "Missing 'dir' parameter");
    }
}

void setupServo() {
    radarServo.attach(SERVO_PIN, 500, 2400);
    setServoToAngle(OA_SERVO_CENTER);
    Serial.println("Servo initialized at 90 degrees.");
}

void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime < 15000)) {
        Serial.print("."); delay(500);
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP Address: "); Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi connection failed! Starting AP+STA mode.");
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP("ESP32_Car_AP", "123456789");
        Serial.print("ESP32 AP SSID: ESP32_Car_AP, IP: "); Serial.println(WiFi.softAPIP());
    }
}

// --- LOGIC FOR OBSTACLE AVOIDANCE ---
void handleObstacleAvoidanceLogic() {
    unsigned long currentMillis = millis();
    float frontDistance;

    switch (currentOaState) {
        case OA_IDLE_FORWARD:
            setServoToAngle(OA_SERVO_CENTER);
            frontDistance = calculateDistance();

            sendMotorCommand(CMD_FORWARD);
            if (frontDistance < OBSTACLE_THRESHOLD_FRONT) { 
                Serial.print("OA: Obstacle Detected! Dist: "); Serial.print(frontDistance); Serial.println(" cm. Stopping.");
                sendMotorCommand(CMD_STOP);
                currentOaState = OA_STOPPED_OBSTACLE;
                oaActionStartTime = currentMillis;
            }
            break;

        case OA_STOPPED_OBSTACLE:
            if (currentMillis - oaActionStartTime > OA_SHORT_STOP_DURATION) {
                Serial.println("OA: Moving backward.");
                sendMotorCommand(CMD_BACKWARD);
                currentOaState = OA_MOVING_BACKWARD;
                oaActionStartTime = currentMillis;
            }
            break;

        case OA_MOVING_BACKWARD:
            sendMotorCommand(CMD_BACKWARD);
            delay(100);
            if (currentMillis - oaActionStartTime > OA_BACKWARD_DURATION) {
                Serial.println("OA: Stopped backing up. Scanning left.");
                sendMotorCommand(CMD_STOP);
                setServoToAngle(OA_SCAN_ANGLE_LEFT);
                currentOaState = OA_SCANNING_LEFT;
                oaActionStartTime = currentMillis;
            }
            break;

        case OA_SCANNING_LEFT:
            if (currentMillis - oaActionStartTime > OA_SERVO_SCAN_DELAY) {
                distanceLeftScan = calculateDistance();
                Serial.print("OA: Distance Left Scan: "); Serial.println(distanceLeftScan);
                Serial.println("OA: Scanning right.");
                setServoToAngle(OA_SCAN_ANGLE_RIGHT);
                currentOaState = OA_SCANNING_RIGHT;
                oaActionStartTime = currentMillis;
            }
            break;

        case OA_SCANNING_RIGHT:
            if (currentMillis - oaActionStartTime > OA_SERVO_SCAN_DELAY) {
                distanceRightScan = calculateDistance();
                Serial.print("OA: Distance Right Scan: "); Serial.println(distanceRightScan);
                setServoToAngle(OA_SERVO_CENTER);
                currentOaState = OA_DECIDING_TURN;
                oaActionStartTime = currentMillis;
            }
            break;

        case OA_DECIDING_TURN:
             if (currentMillis - oaActionStartTime > (OA_SERVO_SCAN_DELAY / 2)) { 

                bool leftPathClearEnough = (distanceLeftScan > OBSTACLE_THRESHOLD_SIDE);
                bool rightPathClearEnough = (distanceRightScan > OBSTACLE_THRESHOLD_SIDE);
                
                float moderateClearance = OBSTACLE_THRESHOLD_FRONT + 5.0f;

                if (leftPathClearEnough && rightPathClearEnough) {
                    if (distanceLeftScan > distanceRightScan + 10.0f) {
                        sendMotorCommand(CMD_LEFT);
                        currentOaState = OA_TURNING_LEFT;
                    } else if (distanceRightScan > distanceLeftScan + 10.0f) {
                        sendMotorCommand(CMD_RIGHT);
                        currentOaState = OA_TURNING_RIGHT;
                    } else {
                        sendMotorCommand(CMD_RIGHT);
                        currentOaState = OA_TURNING_RIGHT;
                    }
                } else if (leftPathClearEnough) {
                    sendMotorCommand(CMD_LEFT);
                    currentOaState = OA_TURNING_LEFT;
                } else if (rightPathClearEnough) {
                    sendMotorCommand(CMD_RIGHT);
                    currentOaState = OA_TURNING_RIGHT;
                } else {
                    bool leftModeratelyOpen = (distanceLeftScan > moderateClearance);
                    bool rightModeratelyOpen = (distanceRightScan > moderateClearance);

                    if (leftModeratelyOpen && distanceLeftScan > distanceRightScan + 10.0f) {
                        sendMotorCommand(CMD_LEFT);
                        currentOaState = OA_TURNING_LEFT;
                    } else if (rightModeratelyOpen && distanceRightScan > distanceLeftScan + 10.0f) {
                        sendMotorCommand(CMD_RIGHT);
                        currentOaState = OA_TURNING_RIGHT;
                    } else {
                        sendMotorCommand(CMD_BACKWARD);
                        currentOaState = OA_MOVING_BACKWARD;
                    }
                }
                oaActionStartTime = currentMillis;
            }
            break;

        case OA_TURNING_LEFT:
            sendMotorCommand(CMD_LEFT);     
            delay(500);
            if (currentMillis - oaActionStartTime > OA_TURN_DURATION) {
                sendMotorCommand(CMD_STOP);
                currentOaState = OA_IDLE_FORWARD;
            }
            break;
        case OA_TURNING_RIGHT:
            sendMotorCommand(CMD_RIGHT);
            delay(500);
            if (currentMillis - oaActionStartTime > OA_TURN_DURATION) {
                sendMotorCommand(CMD_STOP);
                currentOaState = OA_IDLE_FORWARD;
            }
            break;
    }
}

// --- LOGIC FOR OBJECT FOLLOWING ---
void handleFollowObjectModeLogic() {
    setServoToAngle(OA_SERVO_CENTER);

    bool irLeftDetected, irRightDetected;
    readIrSensors(irLeftDetected, irRightDetected);

    if (currentDistanceCm < FOLLOW_TOO_CLOSE_LIMIT) {
        sendMotorCommand(CMD_BACKWARD);
        foWasTurningOnIr = false;
    } else if (currentDistanceCm >= FOLLOW_STOP_RANGE_MIN && currentDistanceCm <= FOLLOW_STOP_RANGE_MAX) {
        sendMotorCommand(CMD_STOP);
        foWasTurningOnIr = false;
    } else if (currentDistanceCm > FOLLOW_STOP_RANGE_MAX && currentDistanceCm <= FOLLOW_MAX_ENGAGE_DISTANCE) {
        sendMotorCommand(CMD_FORWARD);
        foWasTurningOnIr = false;
    } else if (currentDistanceCm > FOLLOW_TOO_CLOSE_LIMIT && currentDistanceCm < FOLLOW_STOP_RANGE_MIN) {
        sendMotorCommand(CMD_FORWARD);
        foWasTurningOnIr = false;
    } else {
        if (irLeftDetected && !irRightDetected) {
            sendMotorCommand(CMD_LEFT_CURVE);
            foActionStartTime = millis();
            foWasTurningOnIr = true;
        } else if (!irLeftDetected && irRightDetected) {
            sendMotorCommand(CMD_RIGHT_CURVE);
            foActionStartTime = millis();
            foWasTurningOnIr = true;
        } else { 
            if (foWasTurningOnIr && (millis() - foActionStartTime < FOLLOW_IR_TURN_COMMIT_DURATION)) {
            } else {
                sendMotorCommand(CMD_STOP);
                foWasTurningOnIr = false;
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("\n--- ESP32 Car Controller ---");
    Serial2.begin(115200, SERIAL_8N1, 16, 17);

    setupWiFi();
    setupServo(); 

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(IR_LEFT_PIN, INPUT_PULLUP);
    pinMode(IR_RIGHT_PIN, INPUT_PULLUP);

    Serial.println("Configuring Firebase...");
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    connectFirebase();

    if (Firebase.ready()) {
        Serial.println("Firebase ready.");
        if (!Firebase.RTDB.beginStream(&stream_fbdo, "/ControlSettings")) {
            Serial.printf("Stream begin error: %s\n", stream_fbdo.errorReason().c_str());
        } else {
            Serial.println("Firebase stream for /ControlSettings initiated.");
        }
        if (Firebase.RTDB.getBool(&fbdo, "/ControlSettings/ObstacleAvoidance")) {
            if (fbdo.dataTypeEnum() == fb_esp_rtdb_data_type_boolean) obstacleAvoidanceEnabled = fbdo.to<bool>();
        }
        Serial.print("Initial ObstacleAvoidance: "); Serial.println(obstacleAvoidanceEnabled ? "ON" : "OFF");

        if (Firebase.RTDB.getBool(&fbdo, "/ControlSettings/FollowObjectMode")) {
            if (fbdo.dataTypeEnum() == fb_esp_rtdb_data_type_boolean) followObjectModeEnabled = fbdo.to<bool>();
        }
        Serial.print("Initial FollowObjectMode: "); Serial.println(followObjectModeEnabled ? "ON" : "OFF");
        if (Firebase.RTDB.getBool(&fbdo, "/ControlSettings/FireFighterMode")) {
            if (fbdo.dataTypeEnum() == fb_esp_rtdb_data_type_boolean) FireFighterModeEnabled = fbdo.to<bool>();
        }
        Serial.print("Initial FireFighterMode: "); Serial.println(FireFighterModeEnabled ? "ON" : "OFF");
    } else {
        Serial.println("Firebase NOT Ready. Check network, API Key, DB URL.");
    }

    server.on("/control", HTTP_GET, handleControl);
    server.begin();
    Serial.println("ESP32 Web Server Started.");
    Serial.println("---------------------------------------------");
}

void loop() {
    unsigned long currentMillis = millis();

    if (WiFi.status() == WL_CONNECTED) {
        server.handleClient();
        if (Firebase.RTDB.readStream(&stream_fbdo)) {
            if (stream_fbdo.streamAvailable()) {
                Serial.printf("Stream Polling: Path: %s, Event: %s, ", stream_fbdo.dataPath().c_str(), stream_fbdo.eventType().c_str());
                if (stream_fbdo.dataTypeEnum() == fb_esp_rtdb_data_type_boolean) {
                    bool value = stream_fbdo.to<bool>();
                    Serial.println(value ? "true" : "false");
                   if (stream_fbdo.dataPath() == "/ObstacleAvoidance") {
                        obstacleAvoidanceEnabled = value;
                        Serial.print(">> OA Mode Updated to: "); Serial.println(obstacleAvoidanceEnabled ? "ON" : "OFF");
                        if (obstacleAvoidanceEnabled) {
                            sendMotorCommand("A");
                        } else {
                            sendMotorCommand("D");
                        }
                        currentOaState = OA_IDLE_FORWARD;
                    } else if (stream_fbdo.dataPath() == "/FollowObjectMode") {
                        followObjectModeEnabled = value;
                        Serial.print(">> Follow Mode Updated to: "); Serial.println(followObjectModeEnabled ? "ON" : "OFF");
                        if (!followObjectModeEnabled) {
                            sendMotorCommand(CMD_STOP);
                        }
                    } else if (stream_fbdo.dataPath() == "/FireFighterMode") {
                        FireFighterModeEnabled = value;
                        Serial.print(">> FireFighter Mode Updated to: "); Serial.println(FireFighterModeEnabled ? "ON" : "OFF");
                        if (FireFighterModeEnabled) {
                            sendMotorCommand("P1"); 
                        } else {
                            sendMotorCommand("P0");
                        }
                    }
                } else {
                    Serial.println(stream_fbdo.stringData().c_str());
                }
            }
        }
    }

    currentDistanceCm = calculateDistance();

    if (currentMillis - lastFirebaseUpdateMillis >= firebaseUpdateInterval) {
        lastFirebaseUpdateMillis = currentMillis;
        if (WiFi.status() == WL_CONNECTED && Firebase.ready()) {
            jsonData.clear();
            float distanceMeters = (currentDistanceCm >= 999.0 || currentDistanceCm < 1.0) ? 4.5 : (currentDistanceCm / 100.0f);
            jsonData.set("angle", String(servoAngle));
            jsonData.set("distance", String(distanceMeters, 2));
            if(!Firebase.RTDB.setJSON(&fbdo, "/RadarData", &jsonData)){
                Serial.print("Failed to set /RadarData: "); Serial.println(fbdo.errorReason());
            }
        }
    }
    if (followObjectModeEnabled) {
        if (obstacleAvoidanceEnabled) {
            Serial.println("Follow Mode ON, OA Mode turned OFF automatically.");
             if (currentOaState != OA_IDLE_FORWARD) {
                sendMotorCommand(CMD_STOP);
                delay(50);
                currentOaState = OA_IDLE_FORWARD;
            }
        }
        handleFollowObjectModeLogic();
    } else if (obstacleAvoidanceEnabled) {
        handleObstacleAvoidanceLogic();
    } else {
        if (currentOaState != OA_IDLE_FORWARD) {
            currentOaState = OA_IDLE_FORWARD;
        }
        setServoToAngle(OA_SERVO_CENTER);
    }
    delay(20);
}