# NT131 - Wireless Embedded Network Systems (LAB & Project)

This repository contains materials related to the labs and project for the NT131 - Wireless Embedded Network System course from the University of Information Technology (UIT).

## LAB

The content in the LAB folder includes the code required for LAB5 and LAB6. 

##

## Project: AUTONOMOUS FIRETRUCK - POSEIDON & CONTROL APPLICATION
Nowadays, advances in science and technology are constantly improving the quality of human life and opening up new breakthroughs for pressing issues. In particular, the combination of embedded microprocessors and wireless communication capabilities has laid the groundwork for applying this technology in fields demanding high precision and safety.

Our team recognizes that integrating embedded and wireless network technology in hazardous areas, such as firefighting, not only helps optimize human resources and minimize risks for firefighters but also provides the ability to perform tasks remotely with flexibility and efficiency.

The team uses an **ESP32** module to act as the main processor, sending and receiving information from sensors and establishing a connection with Firebase for remote control. An **Arduino R3** module is used to receive signals from the ESP32, assisting the ESP32 in controlling and powering the motors.

## Goal
The main goal of this project is to build a vehicle system capable of **automatic fire suppression** and **remote control** via an application designed in **Android Studio**, while also providing user assistance features. The product aims not only to **improve the quality of human life** but also to contribute to promoting the **application of automation in many new fields.**

## Key features
**Remote Control**: The system supports remote control via Wi-Fi, allowing users to operate the vehicle, toggle features, and switch between modes using a mobile device.

**Voice Control**: Utilizing the Google-to-Text API, the system recognizes and converts user voice commands into text. Intent processing then converts these text strings into specific vehicle actions (e.g., move forward, backward, turn left, turn right).

**Collision Avoidance**: Equipped with ultrasonic and infrared sensors, the vehicle can detect obstacles and determine their distance. This allows it to limit its movement to avoid collisions, reducing the risk of accidents. The use of a servo motor enables the ultrasonic sensor to rotate in different directions, optimizing the obstacle detection angle with just a single sensor.

**Obstacle/Heat Source Tracking**: Poseidon can actively locate and follow specific objects using radar and infrared sensors combined with a servo motor. This includes following a guide or a heat source (in real firefighting scenarios, a heat obstacle to be monitored). This optimizes response capabilities in rescue situations and minimizes manual intervention from the operator, thereby enhancing operational efficiency.

**Automatic Fire Suppression**: Poseidon can autonomously extinguish flames in controlled experimental conditions. The vehicle operates independently based on its programmed logic and sensor data, without requiring user control.

**High Scalability and Customization**: The system is designed for expandability, making it easy to integrate new features or upgrade hardware without significant difficulty. The use of the ESP32 module, which is compatible with various sensors and devices, opens up numerous avenues for future development and improvements.
## Contributor (Project)

**Dang Nguyen Le Nhat - 23520231**

**Dat Huynh Minh - 23520249**

## Demo

[Video]()
