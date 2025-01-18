//Esp32 or nodemcu with 3 DOF claw hand 
#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include <iostream>
#include <sstream>
#include <vector>

// WiFi Configuration
const char* ssid = "MPB_RoboClawCar";
const char* password = "12345678";

// Motor Driver Configuration
struct MOTOR_PINS {
  int pinEn;
  int pinIN1;
  int pinIN2;
};

std::vector<MOTOR_PINS> motorPins = {
  {32, 16, 17},  // RIGHT_MOTOR Pins (EnA, IN1, IN2)
  {23, 18, 19},  // LEFT_MOTOR  Pins (EnB, IN3, IN4)
};

#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define STOP 0

#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1

#define FORWARD 1
#define BACKWARD -1

const int PWMFreq = 1000; /* 1 KHz */
const int PWMResolution = 8;
const int PWMSpeedChannel = 4;

// Claw Configuration
struct ServoPins {
  Servo servo;
  int servoPin;
  String servoName;
  int initialPosition;
};

std::vector<ServoPins> servoPins = {
  { Servo(), 27 , "Base", 90},
  { Servo(), 26 , "Shoulder", 90},
  { Servo(), 25 , "Elbow", 90},
  { Servo(), 33 , "Gripper", 90},
};

struct RecordedStep {
  int servoIndex;
  int value;
  int delayInStep;
};

std::vector<RecordedStep> recordedSteps;
bool recordSteps = false;
bool playRecordedSteps = false;
unsigned long previousTimeInMilli = millis();

// Web Server and WebSocket
AsyncWebServer server(80);
AsyncWebSocket wsCarInput("/CarInput");
AsyncWebSocket wsRobotArmInput("/RobotArmInput");

// HTML Interface
const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=2, user-scalable=yes">
    <style>
      /* General Styles */
      body {
        background-color: #f5f5f5; /* Soft light gray background */
        color: #000; /* Black text for readability */
        font-family: Arial, sans-serif;
      }

      h1, h2 {
        color: #007BFF; /* Bright blue for headings */
      }

      /* Claw Controls */
      input[type=button] {
        background-color: #007BFF; /* Bright blue buttons */
        color: white;
        border-radius: 30px;
        width: 100%;
        height: 40px;
        font-size: 20px;
        text-align: center;
        border: none;
        cursor: pointer;
      }

      .slidecontainer {
        width: 100%;
      }

      .slider {
        -webkit-appearance: none;
        width: 100%;
        height: 20px;
        border-radius: 5px;
        background: #d3d3d3;
        outline: none;
        opacity: 0.7;
        -webkit-transition: .2s;
        transition: opacity .2s;
      }

      .slider:hover {
        opacity: 1;
      }

      .slider::-webkit-slider-thumb {
        -webkit-appearance: none;
        appearance: none;
        width: 40px;
        height: 40px;
        border-radius: 50%;
        background: #007BFF; /* Bright blue slider thumb */
        cursor: pointer;
      }

      .slider::-moz-range-thumb {
        width: 40px;
        height: 40px;
        border-radius: 50%;
        background: #007BFF; /* Bright blue slider thumb */
        cursor: pointer;
      }

      /* Car Controls */
      .arrows {
        font-size: 40px;
        color: #007BFF; /* Bright blue arrows */
      }

      td.button {
        background-color: #fff; /* White background for buttons */
        border-radius: 25%;
        box-shadow: 5px 5px #888888;
        cursor: pointer;
      }

      td.button:active {
        transform: translate(5px, 5px);
        box-shadow: none;
      }

      .noselect {
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
      }

      /* Dark Mode Toggle */
      .dark-mode-toggle {
        position: fixed;
        top: 20px;
        right: 20px;
        background-color: #007BFF;
        color: white;
        border: none;
        border-radius: 50%;
        width: 40px;
        height: 40px;
        font-size: 20px;
        cursor: pointer;
        display: flex;
        align-items: center;
        justify-content: center;
      }
    </style>
  </head>
  <body class="noselect" align="center">
    <button class="dark-mode-toggle" onclick="toggleDarkMode()">🌙</button>
    <h1>MPB_RoboClawCAR</h1>
    <h2>Claw Control</h2>

    <!-- Claw Controls -->
    <table id="clawTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Gripper:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="15" max="170" value="90" class="slider" id="Gripper" oninput='sendButtonInput("Gripper",value)'>
          </div>
        </td>
      </tr>
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Elbow:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="50" max="180" value="90" class="slider" id="Elbow" oninput='sendButtonInput("Elbow",value)'>
          </div>
        </td>
      </tr>
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Shoulder:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Shoulder" oninput='sendButtonInput("Shoulder",value)'>
          </div>
        </td>
      </tr>
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Base:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" style="transform: scaleX(-1);" id="Base" oninput='sendButtonInput("Base",value)'>
          </div>
        </td>
      </tr>
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Record:</b></td>
        <td><input type="button" id="Record" value="OFF" ontouchend='onclickButton(this)'></td>
        <td></td>
      </tr>
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Play:</b></td>
        <td><input type="button" id="Play" value="OFF" ontouchend='onclickButton(this)'></td>
        <td></td>
      </tr>
    </table>

    <!-- Car Controls -->
    <h2>Car Control</h2>
    <table id="carTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr>
        <td></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","1")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">⇧</span></td>
        <td></td>
      </tr>
      <tr>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","3")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">⇦</span></td>
        <td class="button"></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","4")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">⇨</span></td>
      </tr>
      <tr>
        <td></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","2")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">⇩</span></td>
        <td></td>
      </tr>
      <tr/><tr/>
      <tr/><tr/>
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Speed:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="255" value="150" class="slider" id="Speed" oninput='sendButtonInput("Speed",value)'>
          </div>
        </td>
      </tr>
    </table>

    <script>
      // WebSocket for Claw Controls
      var webSocketRobotArmInputUrl = "ws:\/\/" + window.location.hostname + "/RobotArmInput";
      var websocketRobotArmInput;

      function initRobotArmInputWebSocket() {
        websocketRobotArmInput = new WebSocket(webSocketRobotArmInputUrl);
        websocketRobotArmInput.onopen = function(event) {};
        websocketRobotArmInput.onclose = function(event) {
          setTimeout(initRobotArmInputWebSocket, 2000);
        };
        websocketRobotArmInput.onmessage = function(event) {
          var keyValue = event.data.split(",");
          var button = document.getElementById(keyValue[0]);
          button.value = keyValue[1];
          if (button.id == "Record" || button.id == "Play") {
            button.style.backgroundColor = (button.value == "ON" ? "#007BFF" : "#007BFF"); /* Bright blue for ON/OFF */
            enableDisableButtonsSliders(button);
          }
        };
      }

      function sendButtonInput(key, value) {
        var data = key + "," + value;
        if (key == "Gripper" || key == "Elbow" || key == "Shoulder" || key == "Base" || key == "Record" || key == "Play") {
          websocketRobotArmInput.send(data);
        } else if (key == "MoveCar" || key == "Speed") {
          websocketCarInput.send(data);
        }
      }

      function onclickButton(button) {
        button.value = (button.value == "ON") ? "OFF" : "ON";
        button.style.backgroundColor = (button.value == "ON" ? "green" : "#007BFF"); /* Bright blue for ON/OFF */
        var value = (button.value == "ON") ? 1 : 0;
        sendButtonInput(button.id, value);
        enableDisableButtonsSliders(button);
      }

      function enableDisableButtonsSliders(button) {
        if (button.id == "Play") {
          var disabled = "auto";
          if (button.value == "ON") {
            disabled = "none";
          }
          document.getElementById("Gripper").style.pointerEvents = disabled;
          document.getElementById("Elbow").style.pointerEvents = disabled;
          document.getElementById("Shoulder").style.pointerEvents = disabled;
          document.getElementById("Base").style.pointerEvents = disabled;
          document.getElementById("Record").style.pointerEvents = disabled;
        }
        if (button.id == "Record") {
          var disabled = "auto";
          if (button.value == "ON") {
            disabled = "none";
          }
          document.getElementById("Play").style.pointerEvents = disabled;
        }
      }

      // WebSocket for Car Controls
      var webSocketCarInputUrl = "ws:\/\/" + window.location.hostname + "/CarInput";
      var websocketCarInput;

      function initCarInputWebSocket() {
        websocketCarInput = new WebSocket(webSocketCarInputUrl);
        websocketCarInput.onopen = function(event) {
          var speedButton = document.getElementById("Speed");
          sendButtonInput("Speed", speedButton.value);
        };
        websocketCarInput.onclose = function(event) {
          setTimeout(initCarInputWebSocket, 2000);
        };
        websocketCarInput.onmessage = function(event) {};
      }

      // Dark Mode Toggle
      function toggleDarkMode() {
        const body = document.body;
        body.classList.toggle("dark-mode");
        const isDarkMode = body.classList.contains("dark-mode");
        body.style.backgroundColor = isDarkMode ? "#333" : "#f5f5f5";
        body.style.color = isDarkMode ? "#fff" : "#000";
        const buttons = document.querySelectorAll("input[type=button]");
        buttons.forEach(button => {
          button.style.backgroundColor = isDarkMode ? "#555" : "#007BFF";
        });
        const sliders = document.querySelectorAll(".slider");
        sliders.forEach(slider => {
          slider.style.backgroundColor = isDarkMode ? "#555" : "#d3d3d3";
        });
      }

      window.onload = function() {
        initRobotArmInputWebSocket();
        initCarInputWebSocket();
      };
      document.getElementById("mainTable").addEventListener("touchend", function(event) {
        event.preventDefault();
      });
    </script>
  </body>
</html>
)HTMLHOMEPAGE";

// Motor Driver Functions
void rotateMotor(int motorNumber, int motorDirection) {
  if (motorDirection == FORWARD) {
    digitalWrite(motorPins[motorNumber].pinIN1, HIGH);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);    
  } else if (motorDirection == BACKWARD) {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, HIGH);     
  } else {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);       
  }
}

void moveCar(int inputValue) {
  Serial.printf("Got value as %d\n", inputValue);  
  switch(inputValue) {
    case UP:
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, FORWARD);                  
      break;
    case DOWN:
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD);  
      break;
    case LEFT:
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, FORWARD);  
      break;
    case RIGHT:
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD); 
      break;
    case STOP:
      rotateMotor(RIGHT_MOTOR, STOP);
      rotateMotor(LEFT_MOTOR, STOP);    
      break;
    default:
      rotateMotor(RIGHT_MOTOR, STOP);
      rotateMotor(LEFT_MOTOR, STOP);    
      break;
  }
}

// Claw Functions
void writeServoValues(int servoIndex, int value) {
  if (recordSteps) {
    RecordedStep recordedStep;       
    if (recordedSteps.size() == 0) {
      for (int i = 0; i < servoPins.size(); i++) {
        recordedStep.servoIndex = i; 
        recordedStep.value = servoPins[i].servo.read(); 
        recordedStep.delayInStep = 0;
        recordedSteps.push_back(recordedStep);         
      }      
    }
    unsigned long currentTime = millis();
    recordedStep.servoIndex = servoIndex; 
    recordedStep.value = value; 
    recordedStep.delayInStep = currentTime - previousTimeInMilli;
    recordedSteps.push_back(recordedStep);  
    previousTimeInMilli = currentTime;         
  }
  servoPins[servoIndex].servo.write(value);   
}

void playRecordedRobotArmSteps() {
  if (recordedSteps.size() == 0) {
    return;
  }
  for (int i = 0; i < 4 && playRecordedSteps; i++) {
    RecordedStep &recordedStep = recordedSteps[i];
    int currentServoPosition = servoPins[recordedStep.servoIndex].servo.read();
    while (currentServoPosition != recordedStep.value && playRecordedSteps) {
      currentServoPosition = (currentServoPosition > recordedStep.value ? currentServoPosition - 1 : currentServoPosition + 1); 
      servoPins[recordedStep.servoIndex].servo.write(currentServoPosition);
      wsRobotArmInput.textAll(servoPins[recordedStep.servoIndex].servoName + "," + currentServoPosition);
      delay(50);
    }
  }
  delay(2000);
  for (int i = 4; i < recordedSteps.size() && playRecordedSteps ; i++) {
    RecordedStep &recordedStep = recordedSteps[i];
    delay(recordedStep.delayInStep);
    servoPins[recordedStep.servoIndex].servo.write(recordedStep.value);
    wsRobotArmInput.textAll(servoPins[recordedStep.servoIndex].servoName + "," + recordedStep.value);
  }
}
void handleRoot(AsyncWebServerRequest *request) 
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) 
{
    request->send(404, "text/plain", "File Not Found");
}

// WebSocket Event Handlers
void onCarInputWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      moveCar(STOP);
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        std::string myData = "";
        myData.assign((char *)data, len);
        std::istringstream ss(myData);
        std::string key, value;
        std::getline(ss, key, ',');
        std::getline(ss, value, ',');
        Serial.printf("Key [%s] Value[%s]\n", key.c_str(), value.c_str()); 
        int valueInt = atoi(value.c_str());     
        if (key == "MoveCar") {
          moveCar(valueInt);        
        } else if (key == "Speed") {
          ledcWrite(PWMSpeedChannel, valueInt);
        }
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}

void onRobotArmInputWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      sendCurrentRobotArmState();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        std::string myData = "";
        myData.assign((char *)data, len);
        std::istringstream ss(myData);
        std::string key, value;
        std::getline(ss, key, ',');
        std::getline(ss, value, ',');
        Serial.printf("Key [%s] Value[%s]\n", key.c_str(), value.c_str()); 
        int valueInt = atoi(value.c_str()); 
        if (key == "Record") {
          recordSteps = valueInt;
          if (recordSteps) {
            recordedSteps.clear();
            previousTimeInMilli = millis();
          }
        } else if (key == "Play") {
          playRecordedSteps = valueInt;
        } else if (key == "Base") {
          writeServoValues(0, valueInt);            
        } else if (key == "Shoulder") {
          writeServoValues(1, valueInt);           
        } else if (key == "Elbow") {
          writeServoValues(2, valueInt);           
        } else if (key == "Gripper") {
          writeServoValues(3, valueInt);     
        }   
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}

void sendCurrentRobotArmState() {
  for (int i = 0; i < servoPins.size(); i++) {
    wsRobotArmInput.textAll(servoPins[i].servoName + "," + servoPins[i].servo.read());
  }
  wsRobotArmInput.textAll(String("Record,") + (recordSteps ? "ON" : "OFF"));
  wsRobotArmInput.textAll(String("Play,") + (playRecordedSteps ? "ON" : "OFF"));  
}

// Setup and Loop
void setUpPinModes() {
  // Set up Motor Pins
  ledcSetup(PWMSpeedChannel, PWMFreq, PWMResolution);
  for (int i = 0; i < motorPins.size(); i++) {
    pinMode(motorPins[i].pinEn, OUTPUT);    
    pinMode(motorPins[i].pinIN1, OUTPUT);
    pinMode(motorPins[i].pinIN2, OUTPUT);  
    ledcAttachPin(motorPins[i].pinEn, PWMSpeedChannel);
  }
  moveCar(STOP);

  // Set up Servo Pins
  for (int i = 0; i < servoPins.size(); i++) {
    servoPins[i].servo.attach(servoPins[i].servoPin);
    servoPins[i].servo.write(servoPins[i].initialPosition);    
  }
}

void setup(void) {
  setUpPinModes();
  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
      
  wsCarInput.onEvent(onCarInputWebSocketEvent);
  wsRobotArmInput.onEvent(onRobotArmInputWebSocketEvent);
  server.addHandler(&wsCarInput);
  server.addHandler(&wsRobotArmInput);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  wsCarInput.cleanupClients();
  wsRobotArmInput.cleanupClients();
  if (playRecordedSteps) { 
    playRecordedRobotArmSteps();
  }
}