#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include "FSR.h"

#define SS_PIN 10
#define RST_PIN 9

const int buzzerPin = 8;           // Pin for the buzzer
const int forceSensorPin = A3;     // Pin for the force sensor
const unsigned long maxTimerDuration = 300000;  // Maximum timer duration in milliseconds (5 minutes)
const int forceThreshold = 50; // Threshold for significant force change

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal lcd(6, 7, 2, 3, 4, 5);
FSR fsr(forceSensorPin);
volatile unsigned long timerStartTime = 0;  // Variable to store the timer start time
volatile unsigned long lastTimerValue = 0;  // Variable to store the last timer value
int initialForceValue = 200; // Initial force value for comparison
bool isAuthorized = false; // Flag to track card authorization
volatile bool isForceChangeDetected = false;  // Flag to track significant force change
bool hasPlayedVictoryMelody = false;
volatile int isSecondForceChangeDetected = 0;

void setup()
{
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.begin(16, 2);
  lcd.print("Scan RFID Card");

  pinMode(buzzerPin, OUTPUT);          // Initialize the buzzer pin as output
  pinMode(forceSensorPin, INPUT);      // Initialize the force sensor pin as input

  attachInterrupt(digitalPinToInterrupt(forceSensorPin), forceChangeInterrupt, CHANGE);
}

void loop()
{
  // Check if the timer has expired
  if (timerStartTime != 0 && millis() - timerStartTime >= maxTimerDuration)
  {
    lcd.clear();
    lcd.begin(16, 2);
    lcd.print("Timer expired");
    // Perform any actions you want when the timer expires here
    timerStartTime = 0;  // Reset the timer start time
  }

  // Check if the force value exceeds the threshold and the timer is not already started
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

  // Display the timer on the LCD and perform other actions if the timer is started and authorized
  if (isAuthorized && timerStartTime != 0)
  {
    // Calculate elapsed time
    unsigned long elapsedTime = millis() - timerStartTime;

    // Check if the elapsed time exceeds the maximum duration
    if (elapsedTime >= maxTimerDuration)
    {
      lcd.clear();
      lcd.begin(16, 2);
      lcd.print("Timer reached 5 min");
      // Perform any actions you want when the timer reaches 5 minutes here
      timerStartTime = 0; // Reset the timer start time
    }

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

  if (!mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  lcd.clear();
  lcd.begin(16, 2);
  lcd.print("UID tag :");
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    lcd.setCursor(0, 1);
    lcd.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    lcd.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  lcd.clear();
  lcd.begin(16, 2);
  lcd.print("Message : ");
  content.toUpperCase();
  if (content.substring(1) == "C3 90 57 95")  // Please change to your card's UID
  {
    lcd.setCursor(0, 1);
    lcd.print("Authorized");
    activateAuthorizedSound();  // Activate the buzzer for 1 second
    cli();
    timerStartTime = 0;
    isForceChangeDetected = false;
    isAuthorized = true; // Set the authorization flag to true
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
    isAuthorized = false; // Set the authorization flag to false
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
