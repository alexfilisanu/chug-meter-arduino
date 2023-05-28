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
unsigned long timerStartTime = 0;  // Variable to store the timer start time
unsigned long lastTimerValue = 0;  // Variable to store the last timer value
int initialForceValue = 200; // Initial force value for comparison
bool isAuthorized = false; // Flag to track card authorization
bool isForceChangeDetected = false;  // Flag to track significant force change
bool hasPlayedVictoryMelody = false;
int isSecondForceChangeDetected = 0;

const int melody[] = {659, 659, 659, 523, 659, 783, 392, 523, 392, 330, 440, 494, 466, 440, 392, 659, 784, 880, 698, 784, 659, 523, 587, 494, 523, 392};
const int noteDurations[] = {250, 250, 500, 250, 250, 500, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 500};


void setup()
{
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.begin(16, 2);
  lcd.print("Scan RFID Card");

  pinMode(buzzerPin, OUTPUT);          // Initialize the buzzer pin as output
  pinMode(forceSensorPin, INPUT);      // Initialize the force sensor pin as input
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
    timerStartTime = millis();
  }

  // Check if the force value has changed significantly and the timer is already started
  if (timerStartTime != 0 && (fsr.getForce() - initialForceValue >= forceThreshold) || (fsr.getForce() - initialForceValue <= forceThreshold) && !isForceChangeDetected)
  {
    // Stop the timer and store the last timer value
    lastTimerValue = millis() - timerStartTime;
    timerStartTime = 0;
    isForceChangeDetected = true;
  //  if (isForceChangeDetected && !hasPlayedVictoryMelody && isAuthorized)
  // {
  //   // playVictoryMelody();  // Play the victory melody
  //   hasPlayedVictoryMelody = true;  // Set the flag to true
  //    activateAuthorizedSound();
  // }   
  Serial.print(lastTimerValue);
  isSecondForceChangeDetected++;
    if (isAuthorized && isSecondForceChangeDetected == 1 && lastTimerValue != timerStartTime) {
    isSecondForceChangeDetected = true;
    
    // activateAuthorizedSound();
  }
  }
isSecondForceChangeDetected = 0;
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
    // timerStartTime = millis();  // Start the timer
    timerStartTime = 0;
    isForceChangeDetected = false;
    isAuthorized = true; // Set the authorization flag to true
  }
  else
  {
    lcd.setCursor(0, 1);
    lcd.print("Access denied");
    activateDeniedSound();
    float forceValue = fsr.getForce();
    // lcd.setCursor(0, 1);
    // lcd.print("Force: ");
    // lcd.print(forceValue);
    // // initialForceValue = forceValue; // Update the initial force value for comparison
    delay(2000);
    lcd.clear();
    setup();
    timerStartTime = 0;
    isForceChangeDetected = false;
    isAuthorized = false; // Set the authorization flag to false
  }
}

// void activateBuzzer(int duration)
// {
//   tone(buzzerPin, 1000);  // Generate a tone of 1000Hz on the buzzer pin
//   delay(duration);       // Wait for the specified duration
//   noTone(buzzerPin);     // Stop the tone
// }

void activateAuthorizedSound()
{
  // tone(buzzerPin, 1000, 1000);  // Generate a tone of 1000Hz on the buzzer pin for 1 second
  // Play an interesting sound for authorized
  tone(buzzerPin, 1000, 250);
  delay(250);
  tone(buzzerPin, 1500, 250);
  delay(250);
  tone(buzzerPin, 2000, 500);
  delay(500);
}

void activateDeniedSound()
{
  // tone(buzzerPin, 2000, 1000);  // Generate a tone of 2000Hz on the buzzer pin for 1 second
  // Play the denied sound effect
  tone(buzzerPin, 400, 1000);
}

void playVictoryMelody() {
  for (int i = 0; i < sizeof(melody) / sizeof(melody[0]); i++) {
    int noteDuration = 1000 / noteDurations[i];
    tone(buzzerPin, melody[i], noteDuration);
    delay(noteDuration * 1.3);   // Add a small delay between notes for better separation
    noTone(buzzerPin);
  }
}
