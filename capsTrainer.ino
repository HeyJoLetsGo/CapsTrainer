/*********
  Jonathan Rivault
*********/

#include <WiFi.h>
#include <Servo.h>
#include <Preferences.h>
#include <WiFiManager.h>

Servo servoBase;  // create servo object to control a servo
Servo servoArm1;
Servo servoArm2;
Servo servoEnd;
// twelve servo objects can be created on most boards

Preferences preferences;

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

int posServoBase = 0;
int posServoArm1 = 0;
int posServoArm2 = 0;
int posServoEnd = 0;
String posBaseServo = String(0);
String posArm1Servo = String(0);
String posArm2Servo = String(0);
String posEndServo = String(0);

int elecAimDev = 25;
int elecAimAD = 26;
int elecAimAG = 27;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);

  wifiConnection();
  initServo();
  initAimant();

  preferences.begin("positions", false);

  //start server  
  server.begin();
}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients
  
  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");     // print a message out in the serial port
    String currentLine = "";    
                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();
      if (client.available()) {            // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          
          if (currentLine.length() == 0) {
            manageServerResponse(header);

            loadHtmlPage(client);

            client.println();
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }      
      
    }
    //Serial.println(preferences.getUInt("servoBase", posServoBase));
    //Serial.println(preferences.getUInt("servoArm1", posServoArm1));
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void wifiConnection() {
  //Local initialization of WiFiManager
  WiFiManager wm;

  bool res;
  res = wm.autoConnect("AutoConnectAP","CapsTrainer");

  if (!res) {
    Serial.println("Failed to connect to WiFi");
  }
  else {
    Serial.println("Connected to WiFi !");
  }
}

void initServo() {
  servoBase.attach(18);  // attaches the servo on the servoPin to the servo object
  servoArm1.attach(19);
  servoArm2.attach(21);
  servoEnd.attach(5);
}

void initAimant() {
  pinMode(elecAimDev, OUTPUT);
  digitalWrite(elecAimDev, HIGH);
  pinMode(elecAimAD, OUTPUT);
  digitalWrite(elecAimAD, HIGH);
  pinMode(elecAimAG, OUTPUT);
  digitalWrite(elecAimAG, HIGH);
}

void manageServerResponse(String header) {

  if(header.indexOf("GET /?value=")>=0 && header.indexOf("&servo=")>=0) {
    int posEqualURL = header.indexOf('=');
    int posAndURL = header.indexOf('&');
    String posServo = header.substring(posEqualURL+1, posAndURL);
    int posServoURL = header.indexOf("servo=");
    posEqualURL = header.indexOf('=', posServoURL);
    posAndURL = header.indexOf('&', posServoURL);
    String whichServo = header.substring(posEqualURL+1, posAndURL);
    //Rotate the servo
    Serial.println("Servo : " + whichServo);

    moveServo(whichServo, posServo);
 
    //Serial.println("posServoBase : " + posServoBase); 
    
  }
  if(header.indexOf("GET /save/load") >= 0) {
    saveServoLoadPositions();
  }  
  if(header.indexOf("GET /load/load") >= 0) {
    loadServoLoadPositions();
  }
  if(header.indexOf("GET /save/sleep") >= 0) {
    saveServoSleepPositions();
  }  
  if(header.indexOf("GET /load/sleep") >= 0) {
    loadServoSleepPositions();
  }
  if(header.indexOf("Get /reload") >= 0) {
    reloadCaps();
  }      
  // The HTTP response ends with another blank line
  
  // Break out of the while loop
  
}

void moveServo(String servo, String positionServo) {

  if (servo == "servoBase") {
      posBaseServo = positionServo;
      posServoBase = positionServo.toInt();
      servoBase.write(posServoBase);
  }
  else if (servo == "servoArm1") { 
      posArm1Servo = positionServo;
      posServoArm1 = positionServo.toInt();
      servoArm1.write(posServoArm1);
  }
  else if (servo == "servoArm2") {
      posArm2Servo = positionServo;
      posServoArm2 = positionServo.toInt();
      servoArm2.write(posServoArm2);      
  }
  else if (servo == "servoEnd") {
      posEndServo = positionServo;
      posServoEnd = positionServo.toInt();
      servoEnd.write(posServoEnd); 
  }
}

void saveServoLoadPositions() {   
  preferences.putUInt("servoBaseLoad", posServoBase);
  preferences.putUInt("servoArm1Load", posServoArm1);
  preferences.putUInt("servoArm2Load", posServoArm2);
  preferences.putUInt("servoEndLoad", posServoEnd);
}

void loadServoLoadPositions() {
  posServoBase = preferences.getUInt("servoBaseLoad", 0);
  posBaseServo = String(posServoBase);
  servoBase.write(posServoBase); 
  posServoArm1 = preferences.getUInt("servoArm1Load", 0);   
  posArm1Servo = String(posServoArm1);     
  servoArm1.write(posServoArm1);
  posServoArm2 = preferences.getUInt("servoArm2Load", 0);   
  posArm2Servo = String(posServoArm2);     
  servoArm2.write(posServoArm2);
  posServoEnd = preferences.getUInt("servoEndLoad", 0);   
  posEndServo = String(posServoEnd);     
  servoEnd.write(posServoEnd);
}

void saveServoSleepPositions() {   
  preferences.putUInt("servoBaseSleep", posServoBase);
  preferences.putUInt("servoArm1Sleep", posServoArm1);
  preferences.putUInt("servoArm2Sleep", posServoArm2);
  preferences.putUInt("servoEndSleep", posServoEnd);
}

void loadServoSleepPositions() {
  posServoBase = preferences.getUInt("servoBaseSleep", 0);
  posBaseServo = String(posServoBase);
  servoBase.write(posServoBase); 
  posServoArm1 = preferences.getUInt("servoArm1Sleep", 0);   
  posArm1Servo = String(posServoArm1);     
  servoArm1.write(posServoArm1);
  posServoArm2 = preferences.getUInt("servoArm2Sleep", 0);   
  posArm2Servo = String(posServoArm2);     
  servoArm2.write(posServoArm2);
  posServoEnd = preferences.getUInt("servoEndSleep", 0);   
  posEndServo = String(posServoEnd);     
  servoEnd.write(posServoEnd);
}

void reloadCaps() {
  goToBottlePosition();
  delay(2000);
  elecAimOff();
  delay(1000);
  goToSleepPosition();
  delay(500);
  elecAimOn();  
}

void goToBottlePosition() {
  servoBase.write(preferences.getUInt("servoBaseLoad", 0));
  servoArm1.write(preferences.getUInt("servoArm1Load", 0));  
  servoArm2.write(preferences.getUInt("servoArm2Load", 0));
  servoEnd.write(preferences.getUInt("servoEndLoad", 0));
}

void elecAimOff() {
  digitalWrite(elecAimDev, LOW);
  digitalWrite(elecAimDev, LOW);
  digitalWrite(elecAimDev, LOW);
}

void goToSleepPosition() {
  servoEnd.write(preferences.getUInt("servoEndSleep", 0));
  servoArm2.write(preferences.getUInt("servoArm2Sleep", 0));
  servoArm1.write(preferences.getUInt("servoArm1Sleep", 0));
  servoBase.write(preferences.getUInt("servoBaseSleep", 0));
}

void elecAimOn() {
  digitalWrite(elecAimDev, HIGH);
  digitalWrite(elecAimDev, HIGH);
  digitalWrite(elecAimDev, HIGH);
}

void loadHtmlPage(WiFiClient client) {
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();

  // Display the HTML web page
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" charset=\"UTF-8\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");
  // CSS to style the on/off buttons 
  // Feel free to change the background-color and font-size attributes to fit your preferences
  client.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial; margin-left:auto; margin-right:auto;}");
  client.println(".tab {overflow: hidden;border: 1px solid #ccc;background-color: #f1f1f1;}");
  client.println(".tab button {background-color: inherit;float: left;width: 50%;border: none;outline: none;cursor: pointer;padding: 14px 16px;transition: 0.3s;}");
  client.println(".tab button:hover {background-color: #ddd;}");
  client.println(".tab button.active {background-color: #ccc;}");
  client.println(".tabcontent {display: none; padding: 6px 12px;}");
  client.println(".slider { width: 300px; }</style>");
  client.println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>");
            
  // Web Page
  client.println("</head><body><h1>CAPS TRAINER</h1>");
  client.println("<div class=\"tab\"><button class=\"tablinks\" onclick=\"openTab(event, 'Setup')\" id=\"defaultOpen\">Setup</button><button class=\"tablinks\" onclick=\"openTab(event, 'Use')\">Use</button></div>");
  client.println("<div id=\"Setup\" class=\"tabcontent\">");
  client.println("<p>Position base : <span id=\"basePos\"></span></p>");          
  client.println("<input type=\"range\" min=\"0\" max=\"180\" class=\"slider\" id=\"baseSlider\" onchange=\"servo(this.value,'servoBase')\" value=\""+posBaseServo+"\"/>");
  client.println("<p>Position bras 1 : <span id=\"arm1Pos\"></span></p>");          
  client.println("<input type=\"range\" min=\"0\" max=\"180\" class=\"slider\" id=\"arm1Slider\" onchange=\"servo(this.value,'servoArm1')\" value=\""+posArm1Servo+"\"/>");
  client.println("<p>Position bras 2 : <span id=\"arm2Pos\"></span></p>");          
  client.println("<input type=\"range\" min=\"0\" max=\"180\" class=\"slider\" id=\"arm2Slider\" onchange=\"servo(this.value,'servoArm2')\" value=\""+posArm2Servo+"\"/>");
  client.println("<p>Position tête : <span id=\"endPos\"></span></p>");          
  client.println("<input type=\"range\" min=\"0\" max=\"180\" class=\"slider\" id=\"endSlider\" onchange=\"servo(this.value,'servoEnd')\" value=\""+posEndServo+"\"/>");
  client.println("<p>Enregistrer la position de recharge: <span id=\"save\"></span></p><a href=\"/save/load\"><button>Save</button></a>"); 
  client.println("<p>Charger la position de recharge: <span id=\"load\"></span></p><a href=\"/load/load\"><button>Load</button></a>"); 
  client.println("<p>Enregistrer la position de repos: <span id=\"save\"></span></p><a href=\"/save/sleep\"><button>Save</button></a>"); 
  client.println("<p>Charger la position de repos: <span id=\"load\"></span></p><a href=\"/load/sleep\"><button>Load</button></a>"); 
  client.println("</div>");
  client.println("<div id=\"Use\" class=\"tabcontent\">");
  client.println("<p>Recharger les CAPS : <span id=\"recharge\"></span></p><a href=\"/reload\"><button>Recharger</button></a>");
  client.println("</div>");
  client.println("<script>");
  client.println("var sliderBase = document.getElementById(\"baseSlider\"); var servoBasePos = document.getElementById(\"basePos\"); servoBasePos.innerHTML = sliderBase.value;" );
  client.println("sliderBase.oninput = function() { sliderBase.value = this.value; servoBasePos.innerHTML = this.value; }");
  client.println("var sliderArm1 = document.getElementById(\"arm1Slider\"); var servoArm1Pos = document.getElementById(\"arm1Pos\"); servoArm1Pos.innerHTML = sliderArm1.value;");
  client.println("sliderArm1.oninput = function() { sliderArm1.value = this.value; servoArm1Pos.innerHTML = this.value; }");
  client.println("var sliderArm2 = document.getElementById(\"arm2Slider\"); var servoArm2Pos = document.getElementById(\"arm2Pos\"); servoArm2Pos.innerHTML = sliderArm2.value;");
  client.println("sliderArm2.oninput = function() { sliderArm2.value = this.value; servoArm2Pos.innerHTML = this.value; }");
  client.println("var sliderEnd = document.getElementById(\"endSlider\"); var servoEndPos = document.getElementById(\"endPos\"); servoEndPos.innerHTML = sliderEnd.value;");
  client.println("sliderEnd.oninput = function() { sliderEnd.value = this.value; servoEndPos.innerHTML = this.value; }");
  client.println("function openTab(evt, cityName) {");
  client.println("var i, tabcontent, tablinks;tabcontent = document.getElementsByClassName(\"tabcontent\");");
  client.println("for (i = 0; i < tabcontent.length; i++) {tabcontent[i].style.display = \"none\";}");
  client.println("tablinks = document.getElementsByClassName(\"tablinks\");");
  client.println("for (i = 0; i < tablinks.length; i++) {tablinks[i].className = tablinks[i].className.replace(\" active\", \"\");}");
  client.println("document.getElementById(cityName).style.display = \"block\";evt.currentTarget.className += \" active\";}");
  client.println("document.getElementById(\"defaultOpen\").click();");
  client.println("$.ajaxSetup({timeout:1000}); function servo(pos, servo) { ");
  client.println("$.get(\"/?value=\" + pos + \"&\" + \"servo=\" + servo + \"&\"); {Connection: close};}</script>");
  
  client.println("</body></html>"); 
}
