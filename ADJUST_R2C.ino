#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  
  // If the RTC has lost power and needs to be initialized, uncomment the following line
   rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  // Uncomment the line above only once to set the initial time if necessary
}

void loop() {
  // Check if data is available from the Serial Monitor
  if (Serial.available()) {
    // Read the entered time and date as a string
    String input = Serial.readStringUntil('\n');
    
    // Parse the input string into separate variables for year, month, day, hour, minute, and second
    int year, month, day, hour, minute, second;
    sscanf(input.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
    
    // Set the RTC time and date
    DateTime newTime(2024,9,12, 12, 36, 0);
    rtc.adjust(newTime);
    
    // Print a confirmation message
    Serial.println("Time and date set successfully.");
  }
}
