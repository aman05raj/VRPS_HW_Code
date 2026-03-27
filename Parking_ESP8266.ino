#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

// Motor Driver Pins
#define IN1 D1
#define IN2 D2
#define ENA D3  // PWM pin

// RFID Module Pins
#define SS_PIN D4
#define RST_PIN D0

// RFID Setup
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Parking Slot Storage (ID + OTP)
const int MAX_SLOTS = 4;
struct ParkingSlot {
    String rfid;
    String otp;
};
ParkingSlot parkingSlots[MAX_SLOTS];
int currentSlots = 0;

// PWM Speed (0 - 255)
int motorSpeed = 255;  // Full speed

// Function to Start Motor
void startMotor() {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, motorSpeed);
    Serial.println("Motor running...");
}

// Function to Stop Motor
void stopMotor() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 0);
    Serial.println("Motor stopped.");
}

// Function to Read RFID
String readRFID() {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        return "";  // No card detected
    }
    
    String id = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        id += String(mfrc522.uid.uidByte[i], HEX);
    }
    
    mfrc522.PICC_HaltA();
    return id;
}

// Function to Park Vehicle
void parkVehicle() {
    if (currentSlots >= MAX_SLOTS) {
        Serial.println("Parking lot is full!");
        return;
    }

    Serial.println("Place your RFID card...");
    while (true) {
        String rfid = readRFID();
        if (rfid != "") {
            Serial.print("RFID detected: ");
            Serial.println(rfid);

            // Check if slot is empty
            bool found = false;
            for (int i = 0; i < currentSlots; i++) {
                if (parkingSlots[i].rfid == rfid) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                stopMotor();
                Serial.println("Enter OTP for this slot:");
                while (Serial.available() == 0); // Wait for input
                String otp = Serial.readStringUntil('\n');
                otp.trim();

                parkingSlots[currentSlots++] = {rfid, otp};
                Serial.println("Vehicle parked successfully!");
                return;
            } else {
                Serial.println("This RFID is already occupied. Try another card.");
            }
        }
        delay(500);
    }
}

// Function to Retrieve Vehicle
void retrieveVehicle() {
    if (currentSlots == 0) {
        Serial.println("No vehicles to retrieve.");
        return;
    }

    Serial.println("Enter your OTP:");
    while (Serial.available() == 0);
    String otp = Serial.readStringUntil('\n');
    otp.trim();

    Serial.println("Place your RFID card...");
    while (true) {
        String rfid = readRFID();
        if (rfid != "") {
            Serial.print("RFID detected: ");
            Serial.println(rfid);

            // Check if RFID & OTP match
            for (int i = 0; i < currentSlots; i++) {
                if (parkingSlots[i].rfid == rfid && parkingSlots[i].otp == otp) {
                    Serial.println("Vehicle found. Opening gate...");
                    stopMotor();

                    // Remove entry from slots
                    for (int j = i; j < currentSlots - 1; j++) {
                        parkingSlots[j] = parkingSlots[j + 1];
                    }
                    currentSlots--;

                    Serial.println("Vehicle retrieved successfully!");
                    return;
                }
            }
            Serial.println("Invalid RFID or OTP. Try again.");
        }
        delay(500);
    }
}

// Setup Function
void setup() {
    Serial.begin(115200);

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(ENA, OUTPUT);
    
    SPI.begin();
    mfrc522.PCD_Init();

    Serial.println("Smart Parking System Ready!");
}

// Loop Function
void loop() {
    Serial.println("Enter 'p' to park, 'r' to retrieve:");
    while (Serial.available() == 0);
    char action = Serial.read();

    if (action == 'p') {
        startMotor();
        parkVehicle();
    } else if (action == 'r') {
        startMotor();
        retrieveVehicle();
    } else {
        Serial.println("Invalid option. Use 'p' or 'r'.");
    }
}
