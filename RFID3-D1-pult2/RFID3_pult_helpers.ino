void process_extra(const char *);
void process_message(const char *);

char secret_open[]="1234";    //reverse! real sequence will be 4321
char secret_new[]="x*5544*";   // add card !reverse!
char secret_del[]="x*8899*";   // del card !reverse!
char secret_clear[]="x*0000*"; // del all !reverse!

#define NUM_CODES 16
#define KEY_PIN A0

#define RED D8
#define GREEN D9
#define BLUE D9

#define OPENER D7

#define BUZZER A0
#define NOTE1 NOTE_A2
#define BUZ_TIME 50

#define OPEN_TIME 500
#define NONVALID 90909090L
#define DEBOUNCE 300

#define Waiting 0
#define NewCard 10
#define DelCard 20
#define NewCode 30
#define Clear   40
#define Program 50

#define OPEN_ADDR 500
#define SEQ_LEN 4


static char iobuf[128]="";
static char tmpbuf[128]="";

char mykeymap[4][4]={
  {'1','4','7','*'},
  {'2','5','8','0'},
  {'3','6','9','#'},
  {'A','B','C','D'}
};
const short KEYWRITE[]={A2,A3,A5,A4};
const short KEYREAD[]={3,4,5,6};




void send_it(const char *s){
  digitalWrite(DIRECTION, HIGH);
  delay(10);
  sprintf(iobuf,"%s%s%s%s",STARTER,tmpbuf,s,ENDER);
  Serial.write(iobuf);
  delay(100);
  digitalWrite(DIRECTION, LOW);
  tmpbuf[0]='\0';
  delay(10);
}

void send_it0(const char *s){
  strcat(tmpbuf,s);
}

void send_it(const long int s){
  char buf[16];
  sprintf(buf,"%ld",s);
  send_it(buf);
}

void send_it0(const long int s){
  char buf[16];
  sprintf(buf,"%ld",s);
  send_it0(buf);
}

void send_itH(const long int s){
  char buf[16];
  sprintf(buf,"%lX",s);
  send_it(buf);
}

void send_itH0(const long int s){
  char buf[16];
  sprintf(buf,"%lX",s);
  send_it0(buf);
}


void init_keypad(){
  for(int i=0;i<4;++i){
    pinMode(KEYREAD[i],INPUT_PULLUP);
  }
}

unsigned int getC(int index=-1)                   //determine whether there is 5V input on the analog pins 3-0
{
  int r;
  for(unsigned int i=0;i<4;++i){
    r=digitalRead(KEYREAD[i]);
    if(r==LOW){
      return i;
    }
  }
  return 255;
}

char readdata(void)          //main function
{
  unsigned int j;
  for (int i=0;i<4;i++){               //for loop
    analogWrite(KEYWRITE[i],0);
    j=getC(i);
    analogWrite(KEYWRITE[i],1024);
    if(j<255){
      delay(DEBOUNCE);
      return mykeymap[j][i];
    }   // output the char
  }
  return 'X';                  // if no button is pressed, return X
}

void read_open_seq(){
  for(int i=0;i<SEQ_LEN;++i)
    EEPROM.get(OPEN_ADDR+i,secret_open[i]);
}

void write_open_seq(const char *seq){
  for(int i=0;i<SEQ_LEN;++i)
    EEPROM.put(OPEN_ADDR+i,secret_open[i]);
}

// key pressed, add it to buffer
void add_key(char key){
  //shift!
  for(int i=sizeof(entered)-1;i>0;--i){
    entered[i]=entered[i-1];
  }
  entered[0]=key;
  entered[sizeof(entered)-1]='\0';
}

void leds_off(){
//  analogWrite(IPIN1, 0);
//  analogWrite(IPIN2, 0);
//  analogWrite(IPIN3, 0);  
  analogWrite(LED_PIN, 0);
}

// indicate idle mode
void ind_waiting(){
  leds_off();
  send_it("waiting mode...");
}

// indicate waiting for card index enter (blue led)
void ind_get_index(){
  send_it("Enter index:");
  leds_off();
  analogWrite(BLUE, 255);
}

// indicate success (short green led flash, beep)
void ind_success(){
  send_it("Success!");
  leds_off();
  analogWrite(GREEN, 255);
  tone1();
  delay(300);
  analogWrite(GREEN, 0);
}

// indicate operation cancelling (short magenta flash)
void ind_cancel(){
  send_it("Cancelled.");
  leds_off();
  analogWrite(BLUE, 255);
  analogWrite(RED, 255);
  delay(300);
  leds_off();
}

// indicate open (short green+blue flash)
void ind_opening(){
  send_it("Opening...");
  leds_off();
  analogWrite(GREEN, 255);
  analogWrite(BLUE, 255);
  delay(300);
  leds_off();
}

// play short buzzer tone
void tone1(){
    tone(BUZZER, NOTE1, BUZ_TIME);
    delay(BUZ_TIME);
    noTone(BUZZER);  
}

// indicate fail (red led flash, two beeps)
void ind_failed(){
  send_it("Failed!");
  leds_off();
  analogWrite(RED, 255);
  tone1();
  delay(100);
  tone1();
  delay(500);
  analogWrite(RED, 0);
}

// erase all cards!!!
void clear_all(){
  EEPROM.put(0, 0L);// zero cards
  for(int i=0;i<NUM_CODES;++i){
    EEPROM.put((i+1)*ulen, NONVALID);
  }
  num_codes=0;
}

// add new card
void add_code(unsigned long id){
  char key='\0';
  
  ind_get_index();

  for(key=readdata();key=='X';){
    delay(10);
  }
  tone1();
  if(key=='*' || key=='#'){
    ind_cancel();
    return;
  }
  key-='0';
  if(key>9){
    key=key+'0'-'A'+10;
  }
  key+=1;
  if(key>num_codes)
    num_codes=key;
  EEPROM.put(key*ulen,id);
  ind_success();
  
  send_it0("Added new card: ");
  send_it(key-1);

}

// delete card
void del_code(){
  char key='\0';
  
  send_it("Choose index");

  while(!key){
    //!!!key = keypad.getKey();
    delay(10);
  }
  tone1();
  if(key=='*' || key=='#'){
    ind_cancel();
    return;
  }
  key-='0';
  if(key>9){
    key=key+'0'-'A'+10;
  }
  key+=1;
  EEPROM.put(key*ulen,NONVALID);
  ind_success();
  send_it0("Deleted card: ");
  send_it(key-1);
}

// new open sequence
void new_seq(){
  static char key;
  
  send_it("New open sequence");

  for(int i=SEQ_LEN-1; i>=0; --i){
    key='\0';
    while(!key){
      //!!!key = keypad.getKey();
      delay(100);
    }
    tone1();
    secret_open[i]=key;
  }
  secret_open[SEQ_LEN]='\0'; // for sure...
  write_open_seq(secret_open);
  ind_success();
  send_it0("New reversed sequence: ");
  send_it(secret_open);
}

//// open the door!!!
//void do_open(){
//  ind_opening();
//  digitalWrite(DIRECTION, HIGH);
//  delay(10);
//  Serial.println("@");
//  delay(100);
//  digitalWrite(DIRECTION, LOW);
////  analogWrite(OPENER,255);
////  delay(OPEN_TIME);
////  analogWrite(OPENER,0);
//}


void do_program(){
  if(entered[0]=='*'){  //add card
    //add card
    mode=NewCard;
  }
  else if(entered[0]=='#'){ // del card
    //delete card
    del_code();
    mode=Waiting;
  }
  else if(entered[0]=='0'){ // del all cards
    clear_all();
    mode=Waiting;
  }
  else if(entered[0]=='D'){  // new open sequence
    new_seq();
    mode=Waiting;
  }
  else{
    ind_cancel();
    mode=Waiting;  
  }
}

void check_card(MFRC522 &mfrc522){
  long int uidDec = 0L;

  if( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  //send_it("Card bringed!");

  // read card serial
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    send_it("Bad card");
    return;
  }

   // print card serial
   send_it0("CUID=");
   for (byte i = 0; i < mfrc522.uid.size; i++) {
     // Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     // Serial.print(mfrc522.uid.uidByte[i], HEX);
     uidDecTemp=mfrc522.uid.uidByte[i];
     uidDec=uidDec*256+uidDecTemp;
   } 

   send_itH(uidDec);  
   mfrc522.PICC_HaltA(); // Stop reading
}

void checkInput(){
  static char buf[BUF_SIZE];
  static int i=0;
  static int c;

  for(c=checkChar(); c!=-1; c=checkChar()){
    buf[i]=(char)c;
    i+=1;
    if(c==ENDER){
        buf[i-1]='\0';
        process_message(buf);
        i=0;
    }
    if(c==STARTER){
      if(i>1){
        buf[i-1]='\0';
        process_extra(buf);
        //do_send("zzz");
      }
      i=0;        
    }
  }
}

