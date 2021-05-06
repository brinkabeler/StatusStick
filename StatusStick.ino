#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Arduino.h>
#include <M5StickC.h>
//StatusStick
#include "Adafruit_SGP30.h"

//Adafruit SGP30 - TVOC sensor init
Adafruit_SGP30 sgp;
int i = 15;
long last_millis = 0;
int co2critical = 900; // Negative health effects start at 1000

//WiFi credentials
const char* ssid = "SSID";
const char* password = "PASSWORD";

// Port for HTTP server
WiFiServer server(80);

// Variable to store the HTTP request
String header;
const char* PARAM_INPUT_1 = "input1";

// variables to store the current output state
String busy_state = "free";
String co2_state = "good";
String notify_state = "off";

// strings to be displayed on stick
const char* busy_string = "Busy";
const char* free_string = "Free";
const char* freshairpls = "Open a window!";

// Assign output variables to GPIO pins
const int LED_PIN = 10; // LED is on pin 10 for M5StickC

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
   
  Serial.begin(115200);
  M5.begin(true, true, true);
  Wire.begin(32,33); // Init pins for SGP30 sensor
   
  // Display setup
  M5.Lcd.setRotation(3);
  M5.Axp.ScreenBreath(10);
  M5.Lcd.setTextColor(TFT_WHITE);  

  //SGP30 sensor initialisation
    if (! sgp.begin()){
    Serial.println("Sensor not found :(");
    while (1);
  }
  Serial.print("Found SGP30 serial #");
  Serial.print(sgp.serialnumber[0], HEX);
  Serial.print(sgp.serialnumber[1], HEX);
  Serial.println(sgp.serialnumber[2], HEX);

  // LED Pin stuff
  pinMode(LED_PIN, OUTPUT);      // set the LED pin mode for RemoteLED  
  digitalWrite(LED_PIN, HIGH);   // set startup LED state - HIGH is off on M5StickC


  // Wifi and Server setup
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin(); // start webserver
 

  // Port defaults to 3232
ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("ArduinoOTA ready");
  
busy_set(0);
notify(0);
}



void loop() {

    M5.update();
      
  // Toggle busy state and display with Button A
  if (M5.BtnA.wasPressed()){
            if (busy_state =="free") {
              busy_set(1);
            } else {
              busy_set(0);
            }
      }

    // Toggle notification LED with Button B
    if (M5.BtnB.wasPressed()){
            if (notify_state =="on") {
              notify(0);
            } else {
              notify(1);
            }
      }
    

  // Webpage section
  WiFiClient client = server.available();   // listen for incoming clients

   if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Status ändern
            if (header.indexOf("GET /10/on") >= 0) {
                notify(1);
            } else if (header.indexOf("GET /10/off") >= 0) {
                notify(0);
            } else if (header.indexOf("GET /shutdown") >= 0) {
                M5.Axp.PowerOff();
            } else if (header.indexOf("input1=") >= 0) {
                M5.Lcd.fillScreen(TFT_NAVY);
                M5.Lcd.setCursor(10, 10, 4);
                M5.Lcd.println(header.substring(header.indexOf("input1=") + 7, header.indexOf("HTTP") - 1)) ; //, header.indexOf("HTTP")
                notify(1);
            }

              
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" >");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Stick #1 Status</h1>");
            client.println("<br>");
            
            // Display current state for busy  
            if (busy_state == "free") {
               client.println("<h2>Free.</h2>");
            } else {
               client.println("<h2>Do not disturb.</h2>");
            } 

            // Display co2 state depending on urgency
            if (co2_state == "bad") {
               client.print("<h2>Opening a window would be good:"); client.print(sgp.eCO2); client.print(" ppm</h2>");
            } else {
               client.print("Current CO2 value: "); client.print(sgp.eCO2); client.print(" ppm.<br>");
            }
            

            client.println("<br>");
            
            // Send message to stick
            client.println("<h2>Send Message</h2>");
            client.println("<form action=\"/get\"><input type=\"text\" name=\"input1\"><input type=\"submit\" value=\"Send\"></form><br>");

            // Button for notify_state  
            if (notify_state == "off") {
              client.println("<p><a href=\"/10/on\"><button class=\"button\">Switch on notification LED</button></a></p>");
            } else {
              client.println("<p><a href=\"/10/off\"><button class=\"button button2\">Switch off notification LED</button></a></p>");
            } 

            
            client.println("<br>");

            // Button to shut down stick
            client.println("<p><a href=\"/shutdown\"><button class=\"button button2\">Switch off Stick</button></a></p>");
             
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
      // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }


   // Arduino OTA section 
  ArduinoOTA.handle();

  //TVOC Loop
  if (! sgp.IAQmeasure()) {
    Serial.println("Measurement failed");
    return;
  }
  Serial.print("TVOC "); Serial.print(sgp.TVOC); Serial.print(" ppb\t");
  Serial.print("eCO2 "); Serial.print(sgp.eCO2); Serial.println(" ppm");

  if (sgp.eCO2 > co2critical) { 
    co2_notify(1);
    } else if ( co2_state == "bad" && sgp.eCO2 < co2critical ) {
    co2_state = "good";
    Serial.println("CO2 okay");
    if ( notify_state == "on" ) {
      notify(1);
    }
    if ( busy_state == "busy" ) {
      busy_set(1);
    }
    if ( busy_state == "free" ) {
      busy_set(0);
    }
  }
    
  delay(1000);
  
}
// END OF LOOP

// CO2 notification routine
int co2_notify(int x){
    Serial.println("CO2 bad");
    co2_state = "bad";
    M5.Lcd.fillScreen(TFT_RED);
    M5.Lcd.setCursor(10, 30, 4);
    M5.Lcd.println(freshairpls);
    
    digitalWrite(LED_PIN, LOW);
    delay(300);
    digitalWrite(LED_PIN, HIGH);
    delay(300);
}

// busy status routine
int busy_set(int x){
  if (x == 1) {
    Serial.println("going busy");
    busy_state = "busy";
    M5.Lcd.fillScreen(TFT_RED);
    M5.Lcd.setCursor(20, 30, 4);
    M5.Lcd.println(busy_string);
  } else {
    Serial.println("going free");
    busy_state = "free";
    M5.Lcd.fillScreen(TFT_DARKGREEN);
    M5.Lcd.setCursor(20, 30, 4);
    M5.Lcd.println(free_string);
  }
}

// LED notification routine
int notify(int x){
  if (x == 1) {
    notify_state = "on";
    Serial.println("LED on");
    digitalWrite(LED_PIN, LOW);
  } else {
    notify_state = "off";
    Serial.println("LED off");
    digitalWrite(LED_PIN, HIGH);
  }
}
