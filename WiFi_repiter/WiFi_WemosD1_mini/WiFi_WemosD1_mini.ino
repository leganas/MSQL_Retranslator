#include <ESP8266WiFi.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

class WiFi_Setting {
  public:
    const char* ssid;
    const char* password;

    WiFi_Setting(const char* ssid, const char* password);
};

WiFi_Setting::WiFi_Setting(const char* _ssid, const char* _password) {
  ssid = _ssid;
  password = _password;
};

IPAddress server_addr(18,222,135,198);  // IP of the MySQL *server* here
char user[] = "root";              // MySQL user login username
char password[] = "forever";        // MySQL user login password

WiFiClient client;
MySQL_Connection conn((Client *)&client);
int timeout = 15000;

unsigned long startTime;
unsigned long lastTime;

// Begin reboot code
int num_fails;                  // variable for number of failure attempts
#define MAX_FAILED_CONNECTS 5   // maximum number of failed connects to MySQL

void soft_reset() {
//  asm volatile("jmp 0");
}

void setup_wifi() {
  WiFi_Setting wifiSetting("LS_AP", "foreverls");
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifiSetting.ssid);
  WiFi.begin(wifiSetting.ssid, wifiSetting.password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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

  setup_wifi();
 
  client.setTimeout(timeout);
  num_fails = 0;
  startTime = millis();
  lastTime = startTime;
}

boolean connectMySQL(){
  Serial.println("Connecting to MySQL...");
  if (conn.connect(server_addr, 53306, user, password)) {
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
        Serial.println(receivedChars);
        if (msg.lastIndexOf("Ping") > -1){
          Serial.println("Pong");
        } else {
          if (connectMySQL() == true) {
//            Serial.println(receivedChars);
            Serial.println("Execute MySQL request");
            MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
            cur_mem->execute(receivedChars);
            delete cur_mem;    
            conn.close();
            Serial.println("Done");
          } 
        }
//        Serial.println(memoryFree());
        newData = false;
  }
}
