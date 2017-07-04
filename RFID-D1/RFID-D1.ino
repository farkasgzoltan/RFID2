/*
  Control side.
  Use Wemos D1 of any other ESP8266 module with enough pins
*/
#include "RFID_D1.h"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "SoftwareSerial.h"

#ifdef SOFT_SERIAL
#define RX D1
#define TX D2
#endif

static char buffer[BUF_SIZE];

static int mode=
/*
#define MQTT_SERV "m21.cloudmqtt.com"
#define MQTT_PORT 1883
#define MQTT_USER "123123123"
#define MQTT_PASS "qweqweqwe"
#define MQTT_TOPC "logTopic"
*/

char secret_open[SEQ_LEN]="1234";    //reverse! real sequence will be 4321
//char secret_new[]="*5544*";   // add card !reverse!
//char secret_del[]="*8899*";   // del card !reverse!
//char secret_clear[]="*0000*"; // del all !reverse!

#define NUM_CODES 16
emun fazes={ AddCardStart, AddCardNumber, DelCardStart, DelCardNumber,
             Open, None};
#define BAD_CARD 879961
#define IND_INTERVAL 300


void do_open(){
    digitalWrite(OPEN_PIN, HIGH);
    delay(IMPULSE);
    digitalWrite(OPEN_PIN, LOW);
    do_log("Opened!");
    do_log_send();
}
void do_close(){
    digitalWrite(OPEN_PIN, LOW);
}

void clear_all();
void do_program(const char *entered);
void write_open_seq(const char *seq);
void read_open_seq();

/*
 * *************************************************
 * 
 * 
 * 
 */
 
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
#ifdef SOFT_SERIAL
SoftwareSerial mySerial(RX, TX);
#else
  #define mySerial Serial
#endif

static char log_buffer[1024]="";

void do_log_send(){
  if(!client.publish(MQTT_TOPC, log_buffer)){
    debug("publish fail...");
  }
  log_buffer[0]='\0';
}
void do_log(const char *s){
  strcat(log_buffer,s);
}

void debug(const char *s){
//  Serial.println(s);
}

void debug0(const char *s){
//  Serial.print(s);
}

#define DIR_OUT HIGH
#define DIR_IN  LOW

void set_dir(int d){
  digitalWrite(RE, d);
  digitalWrite(DE, d);  
}

void setup() {
  pinMode(OPEN_PIN,  OUTPUT);
  digitalWrite(OPEN_PIN, LOW);
  do_close();
  pinMode(RE,   OUTPUT);
  pinMode(DE,   OUTPUT);
  set_dir(DIR_IN);

  //Serial.begin(SPEED);
  ESP.wdtDisable();
  ESP.wdtEnable(WDTO_8S);


  ESP.wdtFeed();
  
  setup_wifi();
  client.setServer(MQTT_SERV, MQTT_PORT);
  client.setCallback(callback);
  reconnect();

  ESP.wdtFeed();
  debug("MySerial...");
  mySerial.begin(SPEED);
  //ESP.wdtFeed();
  do_log("Start!\n");
  do_log_send();

  read_open_seq();
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  //Serial.println();
  debug0("Connecting to ");
  debug(WIFI_SSID);
  yield();

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    yield();
    delay(500);
    debug0(".");
  }

  debug("WiFi connected");
  debug0("IP address: ");
  debug(WiFi.localIP().toString().c_str());
}

void callback(char* topic, byte* payload, unsigned int length) {
  debug0("Message arrived [");
  debug0(topic);
  debug0("] ");
  for (int i = 0; i < length; i++) {
    buffer[i]=(char)payload[i];
  }
  buffer[length]='\0';
  debug(buffer);

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    do_open();
  }

}

void reconnect() {
  //return;

  // Loop until we're reconnected
  while (!client.connected()) {
    debug0("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_SERV, MQTT_USER, MQTT_PASS)) {
      debug("connected");
      // Once connected, publish an announcement...
      //!client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      debug0("failed, rc=..");
      //debug0(client.state());
      debug(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    ESP.wdtFeed();
  }
}

// get new char from mySerial if avilable
int checkChar(){
  if(mySerial.available()){
    return mySerial.read();
  }
  return -1;
}

void do_send(const char *s){
  set_dir(DIR_OUT);
  delay(10);
  mySerial.println(s);
  delay(100);
  set_dir(DIR_IN);
  delay(10);
}

void process_extra(const char *s){
  do_log("Extra: ");
  do_log(s);
  do_log_send();
}

void process_message(const char *s){
  char *entered;
  short msg_type=0;
  do_log("Msg: ");
  do_log(s);
  do_log_send();

  if(strncmp(s,"Buffer=",7)==0){
    entered=s+7;
    msg_type=KBD;
  }
  if(strncmp(s,"CUID=",7)==0){
    entered=s+7;
    msg_type=CARD;
  }
     // do action using bringed card and mode
  switch(mode){
    case Waiting:
      if(msg_type==KBD){
        check_kbd(entered);
      }else if(msg_type==CARD){
        check_code(entered);
      }
      break;
    case Program:
      do_program(entered);
    default:
      send_it0("!invalid mode=");
      send_it(mode);
  }
}

void ind_success(){
  do_send("+p+g");
  delay(IND_INTERVAL);
  ind_none();
}

void ind_cancel(){
  do_send("+p+r");
  delay(IND_INTERVAL);
  ind_none();
}

void ind_input(){
  do_send("+r+g");
}

void ind_none(){
  do_send("-r-g-b");
}

void check_kbd(const char *entered){
  if(strncmp(secret_new,entered,strlen(secret_new))==0){
    mode=NewCard;
  }
  else if(strncmp(secret_del,entered,strlen(secret_del))==0){
    del_code();
    mode=Waiting;
  }
  else if(strncmp(secret_clear,entered,strlen(secret_clear))==0){
    clear_all();
    mode=Waiting;
  }
  else if(strncmp(secret_open,entered,strlen(secret_open))==0){
    do_open();
    mode=Waiting;
  }
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

// check card (1=success, 0=fail)
int check_code(const char *s){
  unsigned long id=0;
  int len=strlen(s);
  static unsigned long saved;

  for(int i=0;i<len;++i){
    if(*s<='9' && *s>='0'){
      id=id*16+(int)*s-(int)'0';
    }
    else{
      id=id*16+(int)*s-(int)'A';
    }
  }
  for(int i=1;i<=num_codes; ++i){
    EEPROM.get(i*ulen,saved);
    if(saved==id){
      send_it0("Card index=");
      send_it(i-1);
      do_send("+g");
      do_open();
      delay(1000);
      do_send("-g");
      return 1;
    }
  }
  do_send("+r"); //red
  do_send("+p"); //beep
  delay(1000);
  do_send("-r");

  return 0;
}


void loop() {
  static int c;
  static int count=0;

  yield();
  ESP.wdtFeed();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  delay(DELAY);
  checkInput();

  // if 'program' pressed?
  pressed=analogRead(KEY_PIN);
  if(pressed>100){
    delay(100); // DUMMY DEBOUNCE
    send_it("Program mode");
    mode=Program;
    return;
  }
}

void read_open_seq(){
  for(int i=0;i<SEQ_LEN;++i)
    EEPROM.get(OPEN_ADDR+i,secret_open[i]);
}

void write_open_seq(const char *seq){
  for(int i=0;i<SEQ_LEN;++i)
    EEPROM.put(OPEN_ADDR+i,secret_open[i]);
}


void do_program(const char *entered){
  static int faze=None;
  static int index;

  switch(faze){
    case None:
      switch(entered[0]){
        case '*':  //add card
          //add card
          faze=AddCardStart;
          ind_input();
          break;
        case '#':
          //delete card
          faze=DefCardStart;
          ind_input();
          break;
        case  '0':
          // del all cards
          clear_all();
          mode=Waiting;
          break;
        case 'D': // new open sequence
          faze=Open;
          index=0;
          ind_input();
          break;
        default:
          mode=Waiting;
         // indication...
      }
      break;
    case AddCardStart:
      tone1();
      if(entered[0]=='*' || entered[0]=='#'){
        ind_cancel();
        mode=Waiting;
        faze=None;
        ind_none();
        return;
      }
      index=entered[0]-'0';
      if(index>9){
        index=index+'0'-'A'+10;
      }
      index+=1;
      faze=AddCardNumber;
      break;
    case AddCardNumber:
      if(entered[0]=='*' || entered[0]=='#'){
        ind_cancel();
        mode=Waiting;
        faze=None;
        ind_none();
        return;
      }
      if(index>num_codes)
        num_codes=index;
      EEPROM.put(index*ulen,entered);
      ind_success();
      mode=Waiting;
      faze=None;
    case DelCardStart:
      tone1();
      if(entered[0]=='*' || entered[0]=='#'){
        ind_cancel();
        mode=Waiting;
        faze=None;
        ind_none();
        return;
      }
      index=entered[0]-'0';
      if(index>9){
        index=index+'0'-'A'+10;
      }
      index+=1;
      faze=DelCardNumber;
      break;
    case DelCardNumber:
      if(entered[0]=='*' || entered[0]=='#'){
        ind_cancel();
        mode=Waiting;
        faze=None;
        ind_none();
        return;
      }
      if(index>num_codes)
        num_codes=index;
      EEPROM.put(key*ulen,BAD_CARD);
      ind_success();
      mode=Waiting;
      faze=None;
    case Open: // next leter of new code
      secret_open[index]=entered[0];
      index+=1;
      if(index>=SEQ_LEN){
        mode=Waiting;
        faze=None;
        ind_success();
        write_open_seq(secret_open);
      }
      break;
    default:
      mode=Waiting;
  }
}

// erase all cards!!!
void clear_all(){
  EEPROM.put(0, 0L);// zero cards
  for(int i=0;i<NUM_CODES;++i){
    EEPROM.put((i+1)*ulen, BAD_CARD);
  }
  num_codes=0;
}

