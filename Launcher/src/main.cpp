/* 

  Launcher - An ESP32+RFM69 wireless high power rocket launcher

*/

#include <Arduino.h>
#include <RFM69.h>
#include <RFM69_ATC.h>
#include <EEPROM.h>
#include "RocketLaunchPad.h"
#include <WiFi.h>
#include <WebServer.h>

//*********************************************************************************************
// Frequency should be set to match the radio module hardware tuned frequency,
// otherwise if say a "433mhz" module is set to work at 915, it will work but very badly.
// Moteinos and RF modules from LowPowerLab are marked with a colored dot to help identify their tuned frequency band,
// see this link for details: https://lowpowerlab.com/guide/moteino/transceivers/
// The below examples are predefined "center" frequencies for the radio's tuned "ISM frequency band".
// You can always set the frequency anywhere in the "frequency band", ex. the 915mhz ISM band is 902..928mhz.
//*********************************************************************************************
//#define FREQUENCY   RF69_433MHZ
//#define FREQUENCY   RF69_868MHZ
#define FREQUENCY     RF69_915MHZ
//#define FREQUENCY_EXACT 916000000 // you may define an exact frequency/channel in Hz
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
//*********************************************************************************************
//Auto Transmission Control - dials down transmit power to save battery
//Usually you do not need to always transmit at max output power
//By reducing TX power even a little you save a significant amount of battery power
//This setting enables this gateway to work with remote nodes that have ATC enabled to
//dial their power down to only the required level (ATC_RSSI)
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI      -80
//*********************************************************************************************

// Set up some constants
#define RST_PIN       5           // This is the pin we'll use to awaken the radio
#define IRQ_PIN       4           // This is the interrupt pin when packets come in
#define LED_PIN       8
#define EEPROM_SIZE   1           // define the number of bytes you want to access from SPI Flash

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

bool spy = false;   //set to 'true' to sniff all packets on the same network

// For communications with RFM69
controllerPayload controllerData;

// Webserver setup
const char* ssid = "BODZINLC";  // Enter SSID here
const char* wifiPwd = "12345678";  //Enter Password here
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
WebServer server(80);

// Set default NODEID to 2, but we'll look it up in flash
int NODEID;

// Change the node ID
void changeNodeID() {
  NODEID++;
  if (NODEID > MAX_NODEID) {
    Serial.printf("NODEID above maximum value of %d, resetting to to %d.\n", MAX_NODEID, DEFAULT_NODEID);
    NODEID = DEFAULT_NODEID;
  }
  Serial.printf("Saving node ID as %d\n", NODEID);
  EEPROM.write(0,NODEID);
  EEPROM.commit();
  
  // Now re-initialize the radio 
  radio.setAddress(NODEID);
}

bool getContinuity() {
  // Stubbed out for now
  return true;
}

void SendHTML(int nodeID, bool continuityState) {
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Launcher Status</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #3498db;}\n";
  ptr +=".button-on:active {background-color: #2980b9;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>Launcher Node ";
  ptr += NODEID;
  ptr +="</h1>\n";
  ptr +="<h3>Continuity: ";
  ptr += (continuityState ? "TRUE": "FALSE");
  ptr += "</h3>\n";
  
  ptr +="<p>Change Node ID</p><a class=\"button button-on\" href=\"/changeNode\">Next</a>\n";
  // if(led1stat)
  // {ptr +="<p>LED1 Status: ON</p><a class=\"button button-off\" href=\"/led1off\">OFF</a>\n";}
  // else
  // {ptr +="<p>LED1 Status: OFF</p><a class=\"button button-on\" href=\"/led1on\">ON</a>\n";}

  server.send(200, "text/html", ptr);
}

void handle_OnConnect() {
  Serial.println("Connection requuest received...");
  SendHTML(NODEID, getContinuity());
}

void handle_changeNode() {
  changeNodeID();
  handle_OnConnect();
}

void handle_NotFound(){
  Serial.println("Invalid requuest received...");
  server.send(404, "text/plain", "Not found");
}

/*
      Setup for ESP32
*/
void setup() {
  Serial.begin(SERIAL_BAUD);
  while (!Serial); // Wait for the Serial Interface to stabilize
  
  // A brief high signal is enough to wake up the radio
  Serial.println("Waking up radio...");
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN,HIGH);
  delay(100);
  digitalWrite(RST_PIN,LOW);
  delay(10);
  Serial.println("Finished waking up.");

  // Let's get our NODEID from Flash at spot 0
  EEPROM.begin(EEPROM_SIZE);
  NODEID = EEPROM.read(0);
  Serial.print("Node ID in flash is ");
  Serial.println(NODEID);
  // If for whatever reason we read an invalid value then just reset to default
  if ((NODEID < DEFAULT_NODEID) || (NODEID > MAX_NODEID)) {
    NODEID = DEFAULT_NODEID;
    Serial.print("Resetting to default node ID of ");
    Serial.println(NODEID);
    EEPROM.write(0,NODEID);
    EEPROM.commit();
  }

  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif

#ifdef ENCRYPTKEY
  radio.encrypt(ENCRYPTKEY);
#endif

#ifdef FREQUENCY_EXACT
  radio.setFrequency(FREQUENCY_EXACT); //set frequency to some custom frequency
#endif
  
//Auto Transmission Control - dials down transmit power to save battery (-100 is the noise floor, -90 is still pretty good)
//For indoor nodes that are pretty static and at pretty stable temperatures (like a MotionMote) -90dBm is quite safe
//For more variable nodes that can expect to move or experience larger temp drifts a lower margin like -70 to -80 would probably be better
//Always test your ATC mote in the edge cases in your own environment to ensure ATC will perform as you expect
#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
  Serial.println("RFM69_ATC Enabled (Auto Transmission Control)\n");

#endif

  // Set the listening pin
  radio.setIrq(IRQ_PIN);
  radio.spyMode(spy);
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  
  WiFi.softAP(ssid, wifiPwd);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  
  server.on("/", handle_OnConnect);
  server.on("/changeNode", handle_changeNode);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");  

}

/*
      Main loop
*/
int loopCounter = 0;
void loop() {

  // loopCounter++;
  // Serial.printf("Start of loop %d...\n", loopCounter);

  // Have the webserver do any stuff needed
  server.handleClient();

  //check for any received packets
  if (radio.receiveDone())
  {
    if (radio.DATALEN != sizeof(controllerPayload)) {
      Serial.print("Invalid payload received, not matching proper structructure!");
    } else {
      controllerData = *(controllerPayload*)radio.DATA; //assume radio.DATA actually contains our struct and not something else
    }

    // OK, let's do something!
    switch (controllerData.launchCommand) {
      case LC_CHECK_CONTINUITY:
        Serial.println("Checking continuity...");
        break;
      case LC_ARM:
        Serial.println("Arming...");
        break;
      case LC_FIRE_IGNITER:
        Serial.println("Launching...");
        break;
      case LC_DISARM:
        Serial.println("Disarming...");
        break;
      default:
        break;
    }

    if (radio.ACKRequested())
    {
      radio.sendACK();
      Serial.print(" - ACK sent");
      Serial.print("Done!")
    }
    Serial.println();
  }

  // delay(1000);
  // Serial.printf("End of loop %d...\n", loopCounter);

}