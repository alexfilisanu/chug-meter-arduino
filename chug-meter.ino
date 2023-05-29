#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include "FSR.h"

#define SS_PIN 10
#define RST_PIN 9

const int buzzerPin = 8;
const int forceSensorPin = A3;
const int forceThreshold = 50;
const int initialForceValue = 200;
const String AUTHORIZED_CARD_UID = "C3 90 57 95";

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal lcd(6, 7, 2, 3, 4, 5);
FSR fsr(forceSensorPin);

volatile unsigned long timerStartTime = 0;
volatile unsigned long lastTimerValue = 0;
volatile bool isForceChangeDetected = false;
bool isAuthorized = false;

void setup()
{
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.begin(16, 2);
  lcd.print("Scan RFID Card");

  pinMode(buzzerPin, OUTPUT);
  pinMode(forceSensorPin, INPUT);

  attachInterrupt(digitalPinToInterrupt(forceSensorPin), forceChangeInterrupt, CHANGE);
}

void loop()
{
  checkForceChange();

  displayTimer();

  checkRFIDCard();
}

void checkForceChange()
{
  // Check if the force value has changed significantly and the timer is not already started
  if ((fsr.getForce() - initialForceValue >= forceThreshold) || (fsr.getForce() - initialForceValue <= forceThreshold) && timerStartTime == 0)
  {
    // Start the timer
    cli();
    timerStartTime = millis();
    sei();
  }
  
  // Check if the force value has changed significantly and the timer is already started
  if (timerStartTime != 0 && (fsr.getForce() - initialForceValue >= forceThreshold) || (fsr.getForce() - initialForceValue <= forceThreshold) && !isForceChangeDetected)
  {
    // Stop the timer and store the last timer value
    cli();
    lastTimerValue = millis() - timerStartTime;
    timerStartTime = 0;
    isForceChangeDetected = true;
    sei();
  }
}

void displayTimer()
{
  // Display the timer on the LCD if the timer is started and action is authorized by RFID
  if (isAuthorized && timerStartTime != 0)
  {
    unsigned long elapsedTime = millis() - timerStartTime;

    // Calculate minutes, seconds, and milliseconds
    unsigned int minutes = elapsedTime / 60000;
    unsigned int seconds = (elapsedTime / 1000) % 60;
    unsigned int milliseconds = elapsedTime % 1000;

    // Construct the timer string
    String timerString = String(minutes) + ":" + (seconds < 10 ? "0" : "") + String(seconds) + "." + (milliseconds < 100 ? "0" : "") + (milliseconds < 10 ? "0" : "") + String(milliseconds);

    // Clear the LCD display and set the cursor position
    lcd.clear();
    lcd.setCursor(0, 0);

    // Display the timer string on the LCD
    lcd.print("Timer: " + timerString);
  }
}

void checkRFIDCard() 
{
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  String content = "";
  // Obtain the UID of the RFID card
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  
  lcd.clear();
  lcd.begin(16, 2);
  lcd.print("Message : ");
  
  content.toUpperCase();
  // Check if the UID matches the authorized card's UID
  if (content.substring(1) == AUTHORIZED_CARD_UID)
  {
    lcd.setCursor(0, 1);
    lcd.print("Authorized");
    activateAuthorizedSound();
    
    cli();
    timerStartTime = 0;
    isForceChangeDetected = false;
    isAuthorized = true;
    sei();
  }
  else
  {
    lcd.setCursor(0, 1);
    lcd.print("Access denied");
    activateDeniedSound();
    delay(2000);
    lcd.clear();
    setup();
    
    cli();
    timerStartTime = 0;
    isForceChangeDetected = false;
    isAuthorized = false;
    sei();
  }
}

void activateAuthorizedSound()
{
  tone(buzzerPin, 1000, 250);
  delay(250);
  tone(buzzerPin, 1500, 250);
  delay(250);
  tone(buzzerPin, 2000, 500);
  delay(500);
}

void activateDeniedSound()
{
  tone(buzzerPin, 400, 1000);
}  

void forceChangeInterrupt()
{
  if (timerStartTime != 0 && !isForceChangeDetected)
  {
    lastTimerValue = millis() - timerStartTime;
    timerStartTime = 0;
    isForceChangeDetected = true;
  }
}
