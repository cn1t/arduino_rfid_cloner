#include <SPI.h>
#include <MFRC522.h>

int ledPin = 7;
int btnPin = 4;

int redLedPin = 3;
int greenLedPin = 5;
int blueLedPin = 6;

bool btnState = false;
bool prevBtnState = false;

#define SS_PIN 10
#define RST_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;


byte cachedCardData[18] = {0}; // Increased buffer size for MIFARE Classic
bool isCloneMode = false;

void setup() {
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);
  pinMode(btnPin, INPUT_PULLUP);

  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);

  SPI.begin();
  rfid.PCD_Init();

  Serial.println("RFID Cloner Initialized");
  updateLedState();
}

void loop() {
  btnState = !digitalRead(btnPin);

  if (btnState != prevBtnState && btnState == LOW) {
    // Button pressed
    isCloneMode = !isCloneMode;  // Toggle clone mode

    if (isCloneMode) {
      Serial.println("Clone mode activated");
    } else {
      Serial.println("Clone mode deactivated");
    }

    updateLedState();
    delay(50);
  }

  prevBtnState = btnState;

  readRFIDCard();  // Read the card data in both read and clone modes
}

void readRFIDCard() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    if (!isCloneMode) {
      for (byte i = 0; i < 4; i++) {
        cachedCardData[i] = rfid.uid.uidByte[i];  // Store the card data in the cache
      }
      Serial.println("RFID Card read and cached");

      setLedColor(0, 255, 0);
      delay(500);
      setLedColor(0, 0, 0);
    } else {
      MFRC522::StatusCode status;
      byte buffer[18];
      byte bufferSize = sizeof(buffer);

      // Authenticate using key A
      status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(rfid.uid));
      if (status != MFRC522::STATUS_OK) {
        Serial.print("Authentication failed: ");
        Serial.println(rfid.GetStatusCodeName(status));
        return;
      }

      // Write data to block 4
      for (byte i = 0; i < 4; i++) {
        buffer[i] = cachedCardData[i];
      }
      status = rfid.MIFARE_Write(4, buffer, bufferSize);
      if (status != MFRC522::STATUS_OK) {
        Serial.print("Writing failed: ");
        Serial.println(rfid.GetStatusCodeName(status));
        return;
      }
      Serial.println("RFID Card written with cached data");

      setLedColor(0, 255, 0);
      delay(500);
      setLedColor(0, 0, 0);
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    // Reset the card serial communication
    if (!rfid.PICC_IsNewCardPresent()) {
      rfid.PICC_ReadCardSerial();
    }

    Serial.print("Card Data: ");
    for (byte i = 0; i < 4; i++) {
      Serial.print(rfid.uid.uidByte[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  } else if (rfid.PICC_IsNewCardPresent() && !rfid.PICC_ReadCardSerial()) {
    Serial.println("Error: Incompatible card type");

    setLedColor(255, 0, 0);
    delay(500);
    setLedColor(0, 0, 0);
  }
}

void updateLedState() {
  if (isCloneMode == false) {
    digitalWrite(ledPin, LOW);
  } else {
    digitalWrite(ledPin, HIGH);
  }
}

void setLedColor(int redValue, int greenValue, int blueValue) {
  analogWrite(redLedPin, redValue);
  analogWrite(greenLedPin, greenValue);
  analogWrite(blueLedPin, blueValue);
}