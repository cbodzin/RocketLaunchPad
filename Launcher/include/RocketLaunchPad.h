// **********************************************************************************
// Definitions and constants for RocketLaunchPad
// **********************************************************************************

//
// For RFM69 networking
//
//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE ************
//*********************************************************************************************
// Address IDs are 10bit, meaning usable ID range is 1..1023
// Address 0 is special (broadcast), messages to address 0 are received by all *listening* nodes (ie. active RX mode)
// Gateway ID should be kept at ID=1 for simplicity, although this is not a hard constraint
//*********************************************************************************************
#define GATEWAYID       1    // "central" node
#define DEFAULT_NODEID  2    // default NODEID
#define MAX_NODEID      8    // arbitrarily set to 8; if you want a 100 launcher controller go for it.
#define NETWORKID       100  // keep IDENTICAL on all nodes that talk to each other
#define ENCRYPTKEY      "B0dzinLauncher!!" //exactly the same 16 characters/bytes on all nodes!

// Other common definitions
#define SERIAL_BAUD   115200

// A structure for the RFM69s to exchange data
typedef struct {
  int           nodeId;
  byte          launchCommand;
  byte          launchResult;
} controllerPayload;

// launchCommand 
#define LC_CHECK_CONTINUITY         1
#define LC_ARM                      2
#define LC_FIRE_IGNITER             3
#define LC_DISARM                   4

// launchResult
#define LC_SUCCESS                  1
#define LC_FAIL                     0