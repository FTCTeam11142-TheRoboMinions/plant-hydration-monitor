#include <ESP8266WiFi.h>
#define ONBOARD_LED 2

// NOTE: Change these values to the WiFi values for your personal WiFi
const char SSID[] =  "SSID"; // Your current WiFi network SSID (can be hidden)
const char PASS[] =  "XXXXXXXXXX"; // Your current WiFi network password
const char *SERVER = "192.168.0.0"; // Your IP address on the WiFi network
const int SERVER_PORT = 3000;

// Main loop process:
//   1) Wake from sleep
//   2) Read from the sensor NUM_READS times, READ_DELAY_APART second apart
//   3) Average readings for a value and POST back to the server
//   4) Sleep for SLEEP_MS
const int NUM_READS = 10;
const int READ_DELAY_MS = 100;
// 5 minutes:
//const int SLEEP_US = 5 * 60 * 1000000;
// 15 seconds (useful when troubleshooting / in development):
const int SLEEP_US = 15 * 1000000;
WiFiClient client;

/**
 * Check for WiFi configuration values in the EEPROM.
 */
bool availableConnectionInfo()
{
  // TODO
  return false;
}

/** 
 * Parse a configuration string from the Serial interface, or return
 * false if the proper format was not used. In the case of a failure,
 * a repeated command will succeed if the format is corrected.
 *
 * This function expects that the Serial interface will receive a
 * command with the following format:
 *    "SSID:<name_of_ssid>\0PASS:<WPA2_or_3_password>\0SERVER:<IP_address_of_web_app>\n"
 *
 * NOTE: Because of the challenge of sending a null character ('\0') 
 * with the Arduino IDE -- there is a workaround where you can uncomment
 * the "specialDelimiter" line in the function to allow it to accept a 
 * second delimiter instead of the '\0' char. This allows you to manually
 * send a configuration to the device without disconnecting the Serial
 * Monitor in the IDE. However, most ASCII printable characters (those
 * supported by the IDE Serial Monitor) are valid SSID and WPA2/3 characters
 * as well and might interfere with your current local configuration. In
 * the case of a conflict, just be sure to change the "specialDelimiter" to
 * a character that isn't used by your SSID or WPA2/3 password.
 */
bool saveConfigData(int length)
{
  // Grab all available data up to the newline
  char incoming[length];
  for (int i = 0; i < length; i++)
  {
    // Accumulate 1 byte at a time
    incoming[i] = Serial.read();
  }
  incoming[length - 1] = '\0';

  // TEMP
  Serial.printf("Received (%i) bytes: ", length);
  Serial.println(incoming);
  // end TEMP

  // Attempt to parse the config string by the delimiters
  // TODO: Improve with strtok() ?
  char delimiter = '\0', specialDelimiter = '\0';
  // Uncomment to make it easier to use Serial Monitor
  specialDelimiter = '|'; 
  int stop1 = 0, stop2 = 0, stop3 = length;
  for (int pos = 0; pos < length; pos++)
  {
    if (stop1 == 0 && (incoming[pos] == delimiter || incoming[pos] == specialDelimiter))
    {
      stop1 = pos;
      incoming[pos] = '\0';
    }
    else if (stop2 == 0 && (incoming[pos] == delimiter || incoming[pos] == specialDelimiter))
    {
      stop2 = pos;
      incoming[pos] = '\0';
    }
  }
  incoming[length - 1] = '\0';

  if (stop1 == 0 || stop2 == 0 || stop3 == 0)
  {
    return false;
  }
  else
  {
    // Check each setting for the proper prefix
    char ssid[stop1 + 1], pass[stop2 - stop1 + 1], server[stop3 - stop2 + 1];
    for (int i = 0; i < length; i++)
    {
      if (i <= stop1) ssid[i] = incoming[i];
      else if (i > stop1 && i <= stop2) pass[i - stop1 - 1] = incoming[i];
      else if (i > stop2) server[i - stop2 - 1] = incoming[i];
    }
    Serial.printf("SSID: %s and PASS: %s and SERVER: %s", ssid, pass, server);
    return true;
  }
}

/**
 * Slow poll (3s) the Serial interface until a configuration is supplied.
 * Announce that we are awaiting a config on the serial interface every 
 * 1 minute, while blinking the onboard LED every 3 seconds;
 */
void awaitConfigFromSerial()
{
  bool configured = false;
  int count = 0;
  while (!configured)
  {
    if (count % 20 == 0) Serial.println("Awaiting configuration...");
    digitalWrite(ONBOARD_LED, LOW);
    delay(500);
    digitalWrite(ONBOARD_LED, HIGH);
    delay(2500);

    // Check for activity on the serial interface
    int available = Serial.available();
    if (available > 0)
    {
      // Parse and Save data for our configuration (might fail)
      if (saveConfigData(available) == false)
      {
        Serial.println("Could not parse configuration string... Ignoring.");
        configured = false;
      }
      else
      {
        configured = true;
      }
    }
    count++;
  }
}

/**
 * Initialization for module after first power-on.
 */
void setup()
{
  // Turn on serial communication for logging
  // TODO: when in production mode we will want to disable serial output to save energy
  Serial.begin(115200);
  delay(10);

  // Onboard LED indicator for monitoring
  pinMode(ONBOARD_LED, OUTPUT);

  if (!availableConnectionInfo())
  {
    awaitConfigFromSerial();
  }

  Serial.printf("\n\nConnecting to: %s\n", SSID);
  // Turn this on for lots more debug output from the ESP8266 library (noisy)
  //Serial.setDebugOutput(true);

  int status = WL_IDLE_STATUS;

  // Connect to WiFi, checking status every .5 seconds
  // Feedback via LED (needs documentation)
  int count = 0;
  WiFi.mode(WIFI_STA);
  status = WiFi.begin(SSID, PASS);
  while (status != WL_CONNECTED)
  {
    status = WiFi.status();
    Serial.print(".");
    digitalWrite(ONBOARD_LED, LOW);
    delay(100);
    digitalWrite(ONBOARD_LED, HIGH);
    delay(400);
    count++;

    // Every 10 seconds, report trouble connecting
    if (count % 20 == 0) 
    {
      Serial.println("\n\nTrouble connecting. Current Diagnostics:");
      WiFi.printDiag(Serial);

      // Rapidly flash the LED 5 times when unable to connect
      for (int i = 0; i < 5; i++) 
      {
        digitalWrite(ONBOARD_LED, LOW);  // On
        delay(100);
        digitalWrite(ONBOARD_LED, HIGH); // Off
        delay(100);
      }
    }
  }
  Serial.println("\nWiFi connected");

  // Setup the A0 pin to read the sensor analog value using ADC
  pinMode(A0, INPUT);
}

void loop()
{
  // Start our read process and turn on the LED
  Serial.println("Starting soil moisture measurement...");
  digitalWrite(ONBOARD_LED, LOW);
  // Read 10 values from the sensor, 1 second apart 
  int totSum = 0;
  for (int k = 0; k < NUM_READS; k++){
    totSum += analogRead(A0);
    // TODO: Should we go to deeper sleep here?
    delay(READ_DELAY_MS);
  }
  // This smooths out the sensor readings ten times with one second intervals
  // TODO: Let's review the reasoning behind this smoothing, not sure I understand 
  //   what we are doing here (or why). At a minimum we should get rid of the magic
  //   numbers so we can change the number of reads
  int moisture = ((totSum / NUM_READS) / 900) * 100; 
  Serial.printf("Done: %i\n", moisture);

  // Open a basic HTTP connection to the server
  Serial.printf("Attempted to report moisture value of '%i' to server at: %s:%i\n", moisture, SERVER, SERVER_PORT);
  if (client.connect(SERVER, SERVER_PORT))  
  {
    // Create our POST request message Body content
    String postStr = "sensorVal=";
    postStr += String(moisture);

    // Send our POST request
    client.print("POST /saturation HTTP/1.1\r\n");
    client.print("Host: localhost\r\n");
    client.print("Connection: close\r\n");
    client.print("Content-Type: application/x-www-form-urlencoded\r\n");
    client.print("Content-Length: " + String(postStr.length()) + "\r\n");
    client.print("\r\n");
    client.print(postStr);

    // Close our HTTP connection
    client.stop();
    Serial.println("Hooray! The request was sucessfully processed!");

    // Rapid flash of the Onboard LED
    digitalWrite(ONBOARD_LED, HIGH); // Off
    delay(100);
    digitalWrite(ONBOARD_LED, LOW);  // On
    delay(100);
  }
  else
  {
    // Something went wrong
    Serial.println("The request could not be processed or timed out.");

    // Rapidly flash the LED 3 times when an error occurs
    for (int i = 0; i < 3; i++) 
    {
      digitalWrite(ONBOARD_LED, HIGH); // Off
      delay(100);
      digitalWrite(ONBOARD_LED, LOW); // On
      delay(100);
    }
  }
  // Turn off the LED
  digitalWrite(ONBOARD_LED, HIGH);
  
  // Wait in Deep Sleep before repeating the measurement (to save battery)
  // SEE ALSO: https://randomnerdtutorials.com/esp8266-deep-sleep-with-arduino-ide/
  // NOTE: Be sure to physically connect GPIO16 to RST or the device will not be
  //   able to wake itself up.
  Serial.printf("Dropping to Deep Sleep for %i microseconds...\n", SLEEP_US);  
  ESP.deepSleep(SLEEP_US); 
}
