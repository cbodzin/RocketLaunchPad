/* 

  Launcher - An ESP32+RFM69 wireless high power rocket launcher

*/

#include <Arduino.h>
#include <RFM69.h>
#include <RFM69_ATC.h>
#include <EEPROM.h>

//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE ************
//*********************************************************************************************
// Address IDs are 10bit, meaning usable ID range is 1..1023
// Address 0 is special (broadcast), messages to address 0 are received by all *listening* nodes (ie. active RX mode)
// Gateway ID should be kept at ID=1 for simplicity, although this is not a hard constraint
//*********************************************************************************************
#define NETWORKID       100  // keep IDENTICAL on all nodes that talk to each other
#define GATEWAYID       1    // "central" node
#define DEFAULT_NODEID  2    // default NODEID
#define MAX_NODEID      8    // arbitrarily set to 8; if you want a 100 launcher controller go for it.

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
#define ENCRYPTKEY    "B0dzinLauncher!!" //exactly the same 16 characters/bytes on all nodes!
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
#define SERIAL_BAUD   115200
#define RST_PIN       5           // This is the pin we'll use to awaken the radio
#define IRQ_PIN       4           // This is the interrupt pin when packets come in
#define LED_PIN       8
#define BUTTON_PIN    9
#define EEPROM_SIZE   1           // define the number of bytes you want to access from SPI Flash



#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

bool spy = false;   //set to 'true' to sniff all packets on the same network
typedef struct {
  int           nodeId; //store this nodeId
  byte          launchCommand; //uptime in ms
} controllerPayload;
controllerPayload controllerData;

// Set default NODEID to 2, but we'll look it up in flash
int NODEID;     

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

}

void loop() {

  //check for any received packets
  if (radio.receiveDone())
  {
    if (radio.DATALEN != sizeof(controllerPayload)) {
      Serial.print("Invalid payload received, not matching Payload struct!");
    } else {
      controllerData = *(controllerPayload*)radio.DATA; //assume radio.DATA actually contains our struct and not something else
    }

    if (radio.ACKRequested())
    {
      radio.sendACK();
      Serial.print(" - ACK sent");
    }
    Serial.println();
  }

}