/*********
  Jonathan Rivault
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <WiFi.h>
#include <Servo.h>
#include <Preferences.h>
#include <WiFiManager.h>

Servo servoBase;  // create servo object to control a servo
Servo servoArm1;
// twelve servo objects can be created on most boards

Preferences preferences;

// Replace with your network credentials
const char* ssid     = "Livebox-B450_Etage";
const char* password = "92ColonaVirus_";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

int posServoBase = 0;
int posServoArm1 = 0;
String posBaseServo = String(0);
String posArm1Servo = String(0);

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);

  wifiConnection();

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
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial; margin-left:auto; margin-right:auto;}");
            client.println(".slider { width: 300px; }</style>");
            client.println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>");
                     
            // Web Page
            client.println("</head><body><h1>ESP32 with Servo</h1>");
            client.println("<p>Position base : <span id=\"basePos\"></span></p>");          
            client.println("<input type=\"range\" min=\"0\" max=\"180\" class=\"slider\" id=\"baseSlider\" onchange=\"servo(this.value,'servoBase')\" value=\""+posBaseServo+"\"/>");
            client.println("<p>Position bras 1 : <span id=\"arm1Pos\"></span></p>");          
            client.println("<input type=\"range\" min=\"0\" max=\"180\" class=\"slider\" id=\"arm1Slider\" onchange=\"servo(this.value,'servoArm1')\" value=\""+posArm1Servo+"\"/>");
            client.println("<p>Enregistrer la position : <span id=\"save\"></span></p><a href=\"/save\"><button>Save</button></a>"); 
            client.println("<p>Charger la position : <span id=\"load\"></span></p><a href=\"/load\"><button>LOAD</button></a>"); 

            client.println("<script>var sliderBase = document.getElementById(\"baseSlider\");");
            client.println("var servoBasePos = document.getElementById(\"basePos\"); servoBasePos.innerHTML = sliderBase.value;" );
            client.println("var sliderArm1 = document.getElementById(\"arm1Slider\"); var servoArm1Pos = document.getElementById(\"arm1Pos\"); servoArm1Pos.innerHTML = sliderArm1.value;");
            client.println("sliderBase.oninput = function() { sliderBase.value = this.value; servoBasePos.innerHTML = this.value; }");
            client.println("sliderArm1.oninput = function() { sliderArm1.value = this.value; servoArm1Pos.innerHTML = this.value; }");
            client.println("$.ajaxSetup({timeout:1000}); function servo(pos, servo) { ");
            client.println("$.get(\"/?value=\" + pos + \"&\" + \"servo=\" + servo + \"&\"); {Connection: close};}</script>");
           
            client.println("</body></html>"); 
            
            //GET /?value=180& HTTP/1.1
            Serial.println(header);
            
            manageServerResponse(header);

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
}

void manageServerResponse(String header) {
  String posServo = String(0);
  String whichServo = String(0);
  int posServoURL = 0;
  int posEqualURL = 0;
  int posAndURL = 0;

  if(header.indexOf("GET /?value=")>=0 && header.indexOf("&servo=")>=0) {
    posEqualURL = header.indexOf('=');
    posAndURL = header.indexOf('&');
    posServo = header.substring(posEqualURL+1, posAndURL);
    posServoURL = header.indexOf("servo=");
    posEqualURL = header.indexOf('=', posServoURL);
    posAndURL = header.indexOf('&', posServoURL);
    whichServo = header.substring(posEqualURL+1, posAndURL);
    //Rotate the servo
    Serial.println("Servo : " + whichServo);

    moveServo(whichServo, posServo);

    Serial.println(posServo); 
  }
  if(header.indexOf("GET /save") >= 0) {
    saveServoPositions;
  }  
  if(header.indexOf("GET /load") >= 0) {
    loadServoPositions;
  }        
  // The HTTP response ends with another blank line
  
  // Break out of the while loop
  
}

void moveServo(String servo, String positionServo) {
  if (servo == "servoBase") {
      posServoBase = positionServo.toInt();
      servoBase.write(posServoBase);
  }
  else if (servo == "servoArm1") { 
      posServoArm1 = positionServo.toInt();
      servoArm1.write(posServoArm1);
  }
}

void saveServoPositions() {
    preferences.putUInt("servoBase", posServoBase);
    preferences.putUInt("servoArm1", posServoArm1);
}

void loadServoPositions() {
    servoBase.write(preferences.getUInt("servoBase", 0));              
    servoArm1.write(preferences.getUInt("servoArm1", 0));
}