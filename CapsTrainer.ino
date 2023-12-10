#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Preferences.h>
#include "SPIFFS.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// create servo object to control a servo
Servo servoBase;  
Servo servoArm1;
Servo servoArm2;
Servo servoHead;

//ppreferences added to "save" positions between launch
Preferences preferences;

// Search for parameter in HTTP POST request
const char* PARAM_WIFI_INPUT_1 = "ssid";
const char* PARAM_WIFI_INPUT_2 = "pass";
const char* PARAM_WIFI_INPUT_3 = "ip";
const char* PARAM_WIFI_INPUT_4 = "gateway";


//Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

IPAddress localIP;
//IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
IPAddress localGateway;
//IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 0, 0);

// Parameters for servo movement
const char* PARAM_INPUT_1 = "value";
const char* PARAM_INPUT_2 = "servo";

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

int posServo = 0;
int posServoBase = 90;
int posServoArm1 = 90;
int posServoArm2 = 90;
int posServoHead = 90;

int elecAimDev = 25;
int elecAimAD = 26;
int elecAimAG = 27;

String inputValue;
String inputServo;

bool MOVE_SERVO = false;
bool SAVE_SLEEP_POSITIONS = false;
bool SAVE_LOAD_POSITIONS = false;
bool LOAD_SLEEP_POSITIONS = false;
bool LOAD_LOAD_POSITIONS = false;
bool RELOAD_CAPS = false;

bool SERVO_BASE = false;
bool SERVO_ARM1 = false;
bool SERVO_ARM2 = false;
bool SERVO_HEAD = false;

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

// Initialize WiFi
bool initWiFi() {
  if(ssid=="" || ip==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());


  if (!WiFi.config(localIP, localGateway, subnet)){
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());
  return true;
}

// Replaces placeholder with LED state value
String processor(const String& var) {
  if(var == "POSITION_SERVO_BASE") {
    return String(posServoBase);
  }
  if(var == "POSITION_SERVO_ARM1") {
    return String(posServoArm1);
  }
  if(var == "POSITION_SERVO_ARM2") {
    return String(posServoArm2);
  }
  if(var == "POSITION_SERVO_HEAD") {
    return String(posServoHead);
  }
  return String();
}

void initServo() {
  servoBase.attach(18);  // attaches the servo on the servoPin to the servo object
  servoArm1.attach(19);
  servoBase.write(90);
  servoArm2.attach(21);
  servoHead.attach(5);
}

void initAimant() {
  pinMode(elecAimDev, OUTPUT);
  digitalWrite(elecAimDev, HIGH);
  pinMode(elecAimAD, OUTPUT);
  digitalWrite(elecAimAD, HIGH);
  pinMode(elecAimAG, OUTPUT);
  digitalWrite(elecAimAG, HIGH);
}

void saveServoLoadPositions() {   
  preferences.putInt("servoBaseLoad", posServoBase);
  preferences.putInt("servoArm1Load", posServoArm1);
  Serial.println("Saving servo 2 value : " + String(posServoArm2));
  preferences.putInt("servoArm2Load", posServoArm2);
  preferences.putInt("servoHeadLoad", posServoHead);
}

void loadServoLoadPositions() {
  
  moveServo(servoBase, posServoBase, preferences.getInt("servoBaseLoad", 0));
  posServoBase = preferences.getInt("servoBaseLoad", 0);

  moveServo(servoArm1, posServoArm1, preferences.getInt("servoArm1Load", 0)); 
  posServoBase = preferences.getInt("servoArm1Load", 0);

  moveServo(servoArm2, posServoArm2, preferences.getInt("servoArm2Load", 0));
  posServoBase = preferences.getInt("servoArm2Load", 0);

  moveServo(servoHead, posServoHead, preferences.getInt("servoHeadLoad", 0));
  posServoBase = preferences.getInt("servoHeadLoad", 0);
}

void saveServoSleepPositions() {   
  preferences.putInt("servoBaseSleep", posServoBase);
  preferences.putInt("servoArm1Sleep", posServoArm1);
  preferences.putInt("servoArm2Sleep", posServoArm2);
  preferences.putInt("servoHeadSleep", posServoHead);
}

void loadServoSleepPositions() {
 
  moveServo(servoBase, posServoBase, preferences.getInt("servoBaseSleep", 0));
  posServoBase = preferences.getInt("servoBaseSleep", 0);

  moveServo(servoArm1, posServoArm1, preferences.getInt("servoArm1Sleep", 0));  
  posServoArm1 = preferences.getInt("servoArm1Sleep", 0);
 
  moveServo(servoArm2, posServoArm2, preferences.getInt("servoArm2Sleep", 0));   
  posServoArm2 = preferences.getInt("servoArm2Sleep", 0);
 
  moveServo(servoHead, posServoHead, preferences.getInt("servoHeadSleep", 0));
  posServoHead = preferences.getInt("servoHeadSleep", 0); 
}

void reloadCaps() {
  loadServoLoadPosition();
  delay(2000);
  elecAimOff();
  delay(1000);
  LoadServoSleepPosition();
  delay(500);
  elecAimOn();  
}

void elecAimOff() {
  digitalWrite(elecAimDev, LOW);
  digitalWrite(elecAimAD, LOW);
  digitalWrite(elecAimAG, LOW);
}

void elecAimOn() {
  digitalWrite(elecAimDev, HIGH);
  digitalWrite(elecAimAD, HIGH);
  digitalWrite(elecAimAG, HIGH);
}

void engageServo(String servo) {

  if (servo == "servoBase") {
      posServo = posServoBase;
      SERVO_BASE = true;
  }
  else if (servo == "servoArm1") { 
      posServo = posServoArm1;
      SERVO_ARM1 = true;
  }
  else if (servo == "servoArm2") {
      posServo = posServoArm2; 
      SERVO_ARM2 = true;     
  }
  else if (servo == "servoHead") {
      posServo = posServoHead;
      SERVO_HEAD = true;
  }
}

void moveServo(Servo &servo, int basePos, int newPos) {

  if (basePos <= newPos) {
    for(int i = basePos; i <= newPos; i += 1) {
      servo.write(i);
      delay(15);
    } 
  }
  else{
    for(int i = basePos; i >= newPos; i -= 1) {
      servo.write(i);  
      delay(15);
    }
  }
}

void writeNewPos(String servo, int value) {
  if (servo == "servoBase") {
      posServoBase = value;
  }
  else if (servo == "servoArm1") { 
      posServoArm1 = value;
  }
  else if (servo == "servoArm2") {
      posServoArm2 = value; 
      Serial.println("Servo 2 new value = " + String(value));
  }
  else if (servo == "servoHead") {
      posServoHead = value;
  }
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  preferences.begin("capsTrainer", false);
  initSPIFFS();
  initServo();
  initAimant();
  // Set GPIO 2 as an OUTPUT
  
  // Load values saved in SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile (SPIFFS, gatewayPath);
  Serial.println(ssid);
  Serial.println(ip);
  Serial.println(gateway);

  if(initWiFi()) {
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });
    server.serveStatic("/", SPIFFS, "/");
    
    // Route to save all servo load positions  
    server.on("/saveLoadPositions", HTTP_GET, [](AsyncWebServerRequest *request) {
      
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
      SAVE_LOAD_POSITIONS = true;
    });

    // Route to save all servo sleep positions
    server.on("/saveSleepPositions", HTTP_GET, [](AsyncWebServerRequest *request) {
      
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
      SAVE_SLEEP_POSITIONS = true;
    });

    // Route to load all servo load positions
    server.on("/loadLoadPositions", HTTP_GET, [](AsyncWebServerRequest *request) {
      
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
      LOAD_LOAD_POSITIONS = true;
    });

    // Route to load all servo sleep positions
    server.on("/loadSleepPositions", HTTP_GET, [](AsyncWebServerRequest *request) {
      
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
      LOAD_SLEEP_POSITIONS = true;
    });

    server.on("/reload", HTTP_GET, [](AsyncWebServerRequest *request) {
      
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
      RELOAD_CAPS = true;
    });

    // Route to handle a moving servo
    server.on("/move", HTTP_GET, [](AsyncWebServerRequest *request) {
      
      

      if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
        inputValue = request->getParam(PARAM_INPUT_1)->value();
        inputServo = request->getParam(PARAM_INPUT_2)->value();

        MOVE_SERVO = true;
        engageServo(inputServo);
      }
    });
    server.begin();
  }
  else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", SPIFFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_WIFI_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_WIFI_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_WIFI_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(SPIFFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_WIFI_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(SPIFFS, gatewayPath, gateway.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }
}

//Je suppose que ça fonctionne pas car il y a la boucle qui reset à chaque fois --> pas ça



void loop() {
  if (MOVE_SERVO) {
    Serial.println("Moving servo");
    if (SERVO_BASE) {
      Serial.println("Servo base is moving");
      moveServo(servoBase, posServo, inputValue.toInt());
      writeNewPos("servoBase", inputValue.toInt());
      SERVO_BASE = false;
    }
    else if (SERVO_ARM1) {
      Serial.println("Servo arm 1 is moving");
      moveServo(servoArm1, posServo, inputValue.toInt());
      writeNewPos("servoArm1", inputValue.toInt());
      SERVO_ARM1 = false;
    }
    else if (SERVO_ARM2) {
      Serial.println("Servo arm 2 is moving");
      moveServo(servoArm2, posServo, inputValue.toInt());
      writeNewPos("servoArm2", inputValue.toInt());
      SERVO_ARM2 = false;
    }
    else if (SERVO_HEAD) {
      Serial.println("Servo head is moving");
      moveServo(servoHead, posServo, inputValue.toInt());
      writeNewPos("servoHead", inputValue.toInt());
      SERVO_HEAD = false;
    }
    MOVE_SERVO = false;
  }

  if (SAVE_LOAD_POSITIONS) {
    Serial.println("Saving load positions");
    saveServoLoadPositions();
    SAVE_LOAD_POSITIONS = false;
  }
  if (SAVE_SLEEP_POSITIONS) {
    Serial.println("Saving sleep positions");
    saveServoSleepPositions();
    SAVE_SLEEP_POSITIONS = false;
  }
  if (LOAD_LOAD_POSITIONS) {
    Serial.println("Loading load positions");
    loadServoLoadPositions();
    LOAD_LOAD_POSITIONS = false;
  }
  if (LOAD_SLEEP_POSITIONS) {
    Serial.println("Loading sleep positions");
    loadServoSleepPositions();
    LOAD_SLEEP_POSITIONS = false;
  }
  if (RELOAD_CAPS) {
    Serial.println("Reloading Caps");
    reloadCaps();
    RELOAD_CAPS = false;
  }
}
           