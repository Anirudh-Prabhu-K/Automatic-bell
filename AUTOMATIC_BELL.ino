                                                                        //////FINAL BELL
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <BluetoothSerial.h>

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD address to 0x27 for a 16 chars and 2 line display

const int ledPin = 23; // Define the LED pin
const int relayPin = 23; // Define the relay pin

BluetoothSerial SerialBT;

// Define schedule timings
struct Schedule {
  int numTimings;
  int timings[12][4]; // Maximum of 4 timings (hours, minutes, seconds, duration) for each schedule
};

// Define schedule modes
enum ScheduleMode {
  NORMAL,
  IAT,
  MODEL,
  SEM
};

// Define schedules
Schedule schedules[] = {
  {12, {{8, 30, 0, 7000},{9, 20, 0, 7000}, {10, 10, 0, 7000}, {11,00, 0, 10000},{11, 20, 0, 7000}, {12, 10, 0, 7000}, {13,00, 0, 10000}, {13,40, 0, 7000}, {14,30, 0, 7000},{15, 20, 0, 7000},{16,10, 0, 5000}, {16,29, 0, 15000}}}, // NORMAL schedule 
  {12, {{8, 45, 0, 7000},{9, 45, 0, 3000}, {10, 45, 0, 7000}, {11,00, 0, 10000},{11, 20, 0,7000},{12,10, 0, 7000}, {13,00, 0, 10000}, {13,40, 0, 7000}, {14,30, 0, 7000},{15, 20, 0, 7000},{16,10, 0, 5000},  {16,29, 0, 15000}}}, // IAT schedule 
  {11, {{8, 45, 0, 7000},{9, 45, 0, 3000}, {10, 45, 0, 3000}, {11,45, 0, 10000},{12,10, 0, 7000}, {13,00, 0, 10000}, {13,40, 0, 7000}, {14,30, 0, 7000},{15, 20, 0, 7000},{16,10, 0, 5000},  {16,29, 0, 15000}}}, // MODEL schedule 
  {8, {{10, 00, 0, 5000},{11,00, 0, 3000}, {12,00, 0, 3000}, {13,00, 0, 7000}, {14,00, 0, 5000},{15,00, 0, 3000}, {16,00, 0, 3000},{17, 00, 0, 7000}}}//SEM FN AND AN
};

const int NUM_SCHEDULES = sizeof(schedules) / sizeof(schedules[0]);

ScheduleMode currentMode = NORMAL; // Initial mode is NORMAL
unsigned long ledStartTime = 0; // Time when LED was turned on
bool ledOnDueToSchedule = false; // Flag to track if LED is on due to a schedule trigger

unsigned long previousMillis = 0; // Variable to store the last time the bell was updated
const unsigned long interval = 1000; // Interval at which to update the bell (milliseconds)
unsigned long bellEndTime = 0; // Time when the bell should stop ringing

void setup() {
  Serial.begin(9600);
  SerialBT.begin("MSEC BELL"); // Bluetooth device name

  pinMode(ledPin, OUTPUT); // Initialize the LED pin
  pinMode(relayPin, OUTPUT); // Initialize the relay pin

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  lcd.init();                      // initialize the lcd 
  lcd.backlight();                 // Turn on backlight
}

void loop() {
  DateTime now = rtc.now(); // Get the current time

  // Print time in format "hh:mm:ss" on the first row
  lcd.setCursor(0, 0);
  printTwoDigits(now.hour(), lcd);
  lcd.print(':');
  printTwoDigits(now.minute(), lcd);
  // lcd.print(':');
  // printTwoDigits(now.second(), lcd);

  // Print date in format "dd/mm/yyyy" on the first row
  lcd.print(" ");
  printTwoDigits(now.day(), lcd);
  lcd.print('/');
  printTwoDigits(now.month(), lcd);
  lcd.print('/');
  lcd.print(now.year(), DEC);

  // Print "SCHEDULE:" on the second row
  lcd.setCursor(0, 1);
  lcd.print("SCHEDULE:");

  // Print the name of the current schedule on the second row
  lcd.setCursor(9, 1);
  switch (currentMode) {
    case NORMAL:
      lcd.print("NORMAL    ");
      break;
    case IAT:
      lcd.print("IAT       ");
      break;
    case MODEL:
      lcd.print("MODEL   ");
      break;
    case SEM:
      lcd.print("SEM  ");
      break;
  }

  // Check the current mode and adjust the LED state accordingly
  switch (currentMode) {
    case NORMAL:
    case IAT:
    case MODEL:
    case SEM:
      updateLedState(now.hour(), now.minute(), now.second());
      break;
  }

  // Check for incoming commands from the Bluetooth terminal app
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    command.trim(); // Remove leading/trailing whitespace
    if (command == "*&^%") {
      currentMode = NORMAL;
    } else if (command == "@$$") {
      currentMode = IAT;
    } else if (command == "@$&#!") {
      currentMode = MODEL;
    } else if (command == "!@&$") {
      currentMode = SEM;
    }
  }

  delay(1000); // Update every second
}

void printTwoDigits(int number, LiquidCrystal_I2C &lcd) {
  if (number < 10) {
    lcd.print('0');
  }
  lcd.print(number);
}

void updateLedState(int currentHour, int currentMinute, int currentSecond) {
  unsigned long currentMillis = millis(); // Get the current time in milliseconds

  // Check if it's time to update the LED state based on the interval
  if (currentMillis - previousMillis >= interval) {
    // Save the current time as the new previous time for the next iteration
    previousMillis = currentMillis;

    bool scheduleMatch = false; // Flag to track if the schedule time matches the current time

    for (int i = 0; i < NUM_SCHEDULES; i++) {
      if (currentMode == i) {
        for (int j = 0; j < schedules[i].numTimings; j++) {
          if (currentHour == schedules[i].timings[j][0] && currentMinute == schedules[i].timings[j][1] && currentSecond == schedules[i].timings[j][2]) {
            // Turn on the LED
            digitalWrite(ledPin, LOW);
            // Set the end time for LED blinking
            ledStartTime = currentMillis;
            bellEndTime = currentMillis + schedules[i].timings[j][3]; // Set the duration from the schedule
            scheduleMatch = true; // Mark that the schedule time matches the current time
            break; // Exit the inner loop once a match is found
          }
        }
      }
    }

    // Check if the LED is still blinking and if the duration has elapsed
    if (currentMillis >= bellEndTime) {
      // Turn off the LED
      digitalWrite(ledPin, HIGH);
    }

    // If the schedule time matched and the LED is still blinking, but the duration has elapsed, turn off the LED
    if (scheduleMatch && currentMillis >= bellEndTime) {
      digitalWrite(ledPin, HIGH);
    }
  }
}
