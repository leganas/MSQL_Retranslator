#include <Ethernet.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};
// задаем статический IP-адрес
// на тот случай, если у DHCP выдать IP-адрес не получится
IPAddress ip(192, 168, 0, 83);

IPAddress server_addr(192,168,0,5);  // IP of the MySQL *server* here
char user[] = "root";              // MySQL user login username
char password[] = "159357Stas";        // MySQL user login password

EthernetClient client;
MySQL_Connection conn((Client *)&client);
int timeout = 15000;

unsigned long startTime;
unsigned long lastTime;

// Begin reboot code
int num_fails;                  // variable for number of failure attempts
#define MAX_FAILED_CONNECTS 5   // maximum number of failed connects to MySQL

void soft_reset() {
  asm volatile("jmp 0");
}

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  // this check is only needed on the Leonardo:
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Power On");
  Serial.println("Configure Ethernet using DHCP");
  // запускаем Ethernet-соединение:
  if (Ethernet.begin(mac) == 0) {
    // если не удалось сконфигурировать Ethernet при помощи DHCP
    Serial.println("Failed to configure Ethernet using DHCP");
    // продолжать дальше смысла нет, поэтому вместо DHCP
    // попытаемся сделать это при помощи IP-адреса:
    Ethernet.begin(mac, ip);
    Serial.print("Static");
  }

  // получаем  и выводим локальный IP адрес
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
  // даем Ethernet 1 секунду на инициализацию
  delay(1000);

  client.setTimeout(timeout);
  num_fails = 0;
  startTime = millis();
  lastTime = startTime;
}

boolean connectMySQL(){
  Serial.println("Connecting to MySQL...");
  if (conn.connect(server_addr, 3306, user, password)) {
    delay(1000);
    return true;
  }
  else{
    num_fails++;
    Serial.println("Connection MySQL failed.");
    if (num_fails == MAX_FAILED_CONNECTS) {
        Serial.println("Ok, that's it. I'm outta here. Rebooting...");
        delay(2000);
        soft_reset();
    }
    return false;
  }
}

void updateIp()
{
  switch (Ethernet.maintain())
  {
    case 0:
      //renewed fail
//      Serial.println("Not result");
      break;
      
    case 1:
      //renewed fail
      Serial.println("Error: renewed fail");
      break;

    case 2:
      //renewed success
      Serial.println("Renewed success");

      //print your local IP address:
      Serial.println(Ethernet.localIP());
      break;

    case 3:
      //rebind fail
      Serial.println("Error: rebind fail");
      break;

    case 4:
      //rebind success
      Serial.println("Rebind success");

      //print your local IP address:
      Serial.println(Ethernet.localIP());
      break;

    default:
      //nothing happened
      break;
  }
}

// Переменные, создаваемые процессом сборки,
// когда компилируется скетч
extern int __bss_end;
extern void *__brkval;

// Функция, возвращающая количество свободного ОЗУ (RAM)
int memoryFree()
{
   int freeValue;
   if((int)__brkval == 0)
      freeValue = ((int)&freeValue) - ((int)&__bss_end);
   else
      freeValue = ((int)&freeValue) - ((int)__brkval);
   return freeValue;
}

void loop() {
  lastTime = millis();
  if (lastTime - startTime > 2000) {
//    if (!conn.connected()) connectMySQL();
    Serial.println("Ready");
    startTime = lastTime;
  } 

  recvWithEndMarker();
  showNewData();
}

const int numChars = 3000;
char receivedChars[numChars];   // an array to store the received data

boolean newData = false;

void recvWithEndMarker() {
    static int ndx = 0;
    char endMarker = '\n';
    char rc;
    
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();
        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
        }
    }
}

void showNewData() {
    if (newData == true) {
        char str[4];
        str[0] = receivedChars[0];
        str[1] = receivedChars[1];
        str[2] = receivedChars[2];
        str[3] = receivedChars[3];
        String msg(str);   
        if (msg.lastIndexOf("Ping") > -1){
          Serial.println("Pong");
        } else {
          if (connectMySQL() == true) {
            Serial.println(receivedChars);
            Serial.println("Execute MySQL request");
            MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
            cur_mem->execute(receivedChars);
            delete cur_mem;    
            conn.close();
            Serial.println("Done");
          } 
        }
        Serial.println(memoryFree());
        newData = false;
  }
}
