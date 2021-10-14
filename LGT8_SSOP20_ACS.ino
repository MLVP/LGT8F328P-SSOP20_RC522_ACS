
/*
 * RFID ACS using MFRC522
 * coded by AWPStar(TheBitProgress). tdr2004@list.ru
 * free to use
 */

/*
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino         LGT8F328P-SSOP20
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST             9 pin
 * SPI SS      SDA(SS)      10            53        D10        10               10              18(PIN_A4)
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16              11
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14              12
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15              13
 */

#define _SERIAL_CONTROL_  0       // Comment if you won't user Serial      
#define _DIMM_PIN_  13            // To dimm annoying LED  

#ifdef _SERIAL_CONTROL_
#define SERIAL_BAUDRATE   (9600)
#include <avr/wdt.h>
#endif

#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>


#define PIN_RST         9          // Some pin for RC522 RST
#define PIN_SS          PIN_A4     // SS/SDA

#define PIN_OUT         (2)        // Pin that opens something
#define PIN_BTN_SET     (3)        // Control Button pin
//#define PIN_BTN_OPEN    (5)        // Open Button pin

#define MARK            (177)      // Just some byte number
#define MAX_TAGS        (10)       // Maximum of tags
#define RST_INTERVAL    (60000)    // RC522 reset interval ms
#define OPEN_TIME       (250)      // ms

MFRC522 mfrc522(PIN_SS, PIN_RST);  // Create MFRC522 instance



uint32_t tags[MAX_TAGS];
uint8_t  last_record;

// First boot.
void genTags() {
  last_record = 0;
  EEPROM.write(0, MARK); // JUST mark
  EEPROM.write(1, last_record); // oldest record index
  // fill nulls
  for (int8_t i = 0; i < MAX_TAGS*4; i++) {
      tags[i] = 0;
      EEPROM.write(2+i, 0);
  }
}

// Loading TAGS and oldest record index from EEPROM
void loadTags() {
  uint8_t buf[4];
  uint8_t j=2;
  last_record = EEPROM.read(1);
  for (uint8_t i=0; i<MAX_TAGS; i++) {
    buf[0] = EEPROM.read(j+0);
    buf[1] = EEPROM.read(j+1);
    buf[2] = EEPROM.read(j+2);
    buf[3] = EEPROM.read(j+3);
    tags[i] = *(uint32_t *)(&buf[0]);
    j+=4;
  }
}

// Looking for user Tag
int8_t findTag(uint32_t uid) {
  for (int8_t i = 0; i < MAX_TAGS; i++) {
    if (tags[i]==uid) return i;
  }
  return (-1);
}

// Looking for Free Tag or remove OLDest
int8_t findFreeTag(uint32_t uid) {
  int8_t i;
  for (i = 0; i < MAX_TAGS; i++) {
    if (tags[i]==0) return i;
  }
  // remove oldest and free slot
  i = last_record;
  last_record = (last_record+1) % MAX_TAGS;
  EEPROM.write(1, last_record);
  return (i);
}

// When BUTTON pressed
// Put in new tag or remove exists.  
void processTag(uint32_t uid){
    int8_t i = findTag(uid);
    uint8_t buf[4];
    if (i != -1) {
      // user exists / remove user
      #ifdef _SERIAL_CONTROL_
        Serial.print("REMOVE ");
        Serial.print(tags[i]);
        Serial.println();
      #endif
      tags[i] = 0;
    }else{
      // new user
      i = findFreeTag(uid);
      tags[i] = uid;
      #ifdef _SERIAL_CONTROL_
        Serial.print("NEW ");
        Serial.print(tags[i]);
        Serial.println();
      #endif
    }
    *(uint32_t *)buf = tags[i];
    EEPROM.write(2+i*4+0, buf[0]);
    EEPROM.write(2+i*4+1, buf[1]);
    EEPROM.write(2+i*4+2, buf[2]);
    EEPROM.write(2+i*4+3, buf[3]);
}

void resetRC522() {
    digitalWrite(PIN_RST, HIGH);
    delay(4);
    digitalWrite(PIN_RST, LOW);
    delay(4);
    mfrc522.PCD_Init();
    delay(4);  
}

// What we do if Access Granted
void accessGranted(uint32_t uid) {
    #ifdef _SERIAL_CONTROL_
      Serial.print("PASS ");
      Serial.print(uid);
      Serial.println();
    #endif
    digitalWrite(PIN_OUT, HIGH);
    delay(OPEN_TIME);
    digitalWrite(PIN_OUT, LOW);
}

// What we do if Access Denied
void accessDenied(uint32_t uid) {
    #ifdef _SERIAL_CONTROL_
      Serial.print("DENIED ");
      Serial.print(uid);
      Serial.println();
    #endif
}


#ifdef _SERIAL_CONTROL_
void EXEC(char *cmd) {
  
  if (!strcmp(cmd, "OPEN")) {
    accessGranted(0);
    
  }else if (!strcmp(cmd, "REBOOT")) {
    Serial.println("REBOOT");
    wdt_disable();
    wdt_enable(WDTO_15MS);
    
  }else if (!strcmp(cmd, "CLEAR")) {
    genTags();
  }
  
}
#endif


// ============================= MAIN PART ==============================

//                             === SETUP ===
void setup() {
  #ifdef _SERIAL_CONTROL_
    Serial.begin(SERIAL_BAUDRATE);   // Initialize serial communications with the PC
    while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  #endif
  
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme

  #ifdef _DIMM_PIN_
    pinMode(_DIMM_PIN_, OUTPUT); // LED
    digitalWrite(_DIMM_PIN_, LOW);
  #endif

  #ifdef PIN_BTN_OPEN
    pinMode(PIN_BTN_OPEN, INPUT);
    digitalWrite(PIN_BTN_OPEN, LOW);
  #endif
  
  pinMode(PIN_BTN_SET, INPUT);
  digitalWrite(PIN_BTN_SET, LOW);

  pinMode(PIN_OUT, OUTPUT);
  digitalWrite(PIN_OUT, LOW);
  
  // first use check
  if (EEPROM.read(0) != MARK) {    
    genTags();
  }else{
    loadTags();
  }
  #ifdef _SERIAL_CONTROL_
    Serial.println("START"); //F("START[]")
  #endif
}

//                             === LOOP ===
void loop() {
  static uint32_t rst_time=0;
  static uint32_t read_time=0;
  static uint32_t last_uid=0;
  uint32_t uid;
  int button;

  #ifdef PIN_BTN_OPEN
    static int obut=0;
    int but;
    but = digitalRead(PIN_BTN_OPEN);
    if (but==1 && obut==0) {
      accessGranted(0);
    }
    obut=but;
  #endif

  button = digitalRead(PIN_BTN_SET);
  if (mfrc522.PICC_IsNewCardPresent()) {
    if (mfrc522.PICC_ReadCardSerial()) {
        uid = *(uint32_t*)(&mfrc522.uid.uidByte[0]);
        if ( (last_uid!=uid) || ((millis()-read_time)>500) ) {
          if (!button) {
            // access
            if (findTag(uid)!=-1) {
              // Known user
              accessGranted(uid);
            }else{
              // Unknown user
              accessDenied(uid);
            }
          }else{
             // add new of delete button
            processTag(uid);
          }
        }
        read_time = millis();
        last_uid = uid;
    }
  }

  if ((millis()-rst_time) > RST_INTERVAL) {
    rst_time = millis();
    resetRC522();
  }
  
  // Read commands
  #ifdef _SERIAL_CONTROL_
    //static String cmd="";
    static char cmd[9] = {'\0'};
    char c;
    int sl;
    if (Serial.available() > 0) {
      c = Serial.read();
      if (c=='\n') return;
      strncat(cmd, &c, 1);
      sl = strlen(cmd);
      if ( sl==8 || c=='\r' ) {
        char *fch = strchr(cmd,'\r');
        if (fch) *fch = '\0';
        
        EXEC(cmd);

        cmd[0] = '\0';
      }
    }
  #endif
}


// delay time
void yield() {
  // nothing
}
