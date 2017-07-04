/*
 * Digital lock with keyboard (4x4) and NFC.
 * 16 cards are supported. To add card, press 'program' key,
 * then '*' on keyboard, bring the card, them press number,
 * corresponding to this card index.
 * To delete card, press 'program' key, then '#', then card index.
 * To delete ALL cards, press 'program', then '0'.
 * To write open sequence, press 'program', then 'D',
 * then 4 keys of sequence (length can be changed in SEQ_LEN).
 * 
 * Card index can be 0-9 and A-D.
 * 
 * You can change values of secret_*** variables to set sequences
 * for add/del cards without 'program' key. It is INSECURE and not
 * recommended.
 * 
 * Connections:
 * MFRC522:
 *  NSS = 10
 *  SCK = 13
 *  MOSI = 11
 *  MISO = 12
 * 
 * Keyboard:
 *  pins *..D -> A2-A5,D2-D5
 * 
 * D6 - serial direction
 */

//#define DEBUG

#include "pitches.h"
#include <EEPROM.h>

#include <SPI.h>
#include <MFRC522.h>
//
// NSS = 10
// SCK = 13
// MOSI = 11
// MISO = 12
// 

#define SPEED 9600

#define SS_PIN 10
#define RST_PIN 9

MFRC522 mfrc522(SS_PIN, RST_PIN);       // объект MFRC522
unsigned long uidDec, uidDecTemp;       // для отображения номера карточки в десятичном формате
byte bCounter, readBit;
unsigned long ticketNumber;

#define DIRECTION 7
#define DIR2 7

#define ENTERED_LEN 8
char entered[ENTERED_LEN+1]="x";

#define LED_PIN A0

#define STARTER "_"
#define ENDER "~"

unsigned long codes[NUM_CODES];
unsigned long num_codes;
short int mode=Waiting;

const int ulen=sizeof(unsigned long);
//void read_open_seq();

void setup() {
  pinMode(DIRECTION, OUTPUT);
  digitalWrite(DIRECTION, LOW);
  pinMode(DIR2, OUTPUT);
  digitalWrite(DIR2, LOW);
  
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN,0);
  
  Serial.begin(SPEED);     
  SPI.begin();            // init SPI
  mfrc522.PCD_Init();     // init MFRC522
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  send_it("Starting...");

  EEPROM.get(0, num_codes);
  if(num_codes>NUM_CODES)
    num_codes=NUM_CODES;

  // read all cards
  for(int i=0;i<num_codes;++i){
    EEPROM.get((i+1)*ulen, codes[i]);
  }

  init_keypad();

  read_open_seq();

  // salute by leds!
  tone1();
  analogWrite(IPIN1, 256);
  delay(300);
  analogWrite(IPIN1, 0);
  analogWrite(IPIN2, 256);
  delay(300);
  analogWrite(IPIN2, 0);
  analogWrite(IPIN3, 256);
  delay(300);
  analogWrite(IPIN3, 0);
  send_it("start");
}

/******************************************************************
 * 
 *****************************************************************/

void loop() {
  static char key;
  static int pressed;

//  if(Serial.available()){
//    char b[128]="Got: ";
//    int i=4;
//    while(Serial.available()){
//      b[i]=Serial.read();
//      i++;
//      delay(20);
//    }
//    b[i]='\0';
//    send_it(b);
//  }
  key=readdata();
  if (key!='X'){
    //Serial.println(key);
    //tone1();
    char str[2]=".";
    add_key(key);
    send_it0("Entered: ");
    send_it0(key);
    str[0]=key;
    send_it0("=");
    send_it(str);
    send_it0("Buffer=");
    send_it(entered);
    
  }
  //return;
  // check for card bringing

  check_card();
}

void turn_red(int on){
  digitalWrite(LED_RED, on==0 ? LOW : HIGH);
}
void turn_green(int on){
  digitalWrite(LED_GREEN, on==0 ? LOW : HIGH);
}
void turn_blue(int on){
  digitalWrite(LED_BLUE, on==0 ? LOW : HIGH);
}
void do_beep(int on){
  digitalWrite(BEEPER, HIGH);
  delay(BEEP_DELAY);
  digitalWrite(BEEPER, LOW);
}

void process_extra(const char *m){
  
}

void process_message(const char *m){
  if(strlen(m)!=2){
    send_it("!Bad msg");
    return;
  }
  switch(m[1]){
    case 'r':
      turn_red(m[0]=='+' ? 1 : 0);
    case 'g':
      turn_green(m[0]=='+' ? 1 : 0);
    case 'b':
      turn_blue(m[0]=='+' ? 1 : 0);
    case 'p': //beep
      do_beep();    
  }
}

