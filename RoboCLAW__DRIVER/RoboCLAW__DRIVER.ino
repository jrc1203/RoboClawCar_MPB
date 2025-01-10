#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// PCA9685 Configuration
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40); // Default I2C address

// WiFi Configuration
const char* ssid = "Robo_MPB";
const char* password = "12345678";

// Claw Configuration
struct ServoPins {
  int servoPin;
  String servoName;
  int minAngle;
  int maxAngle;
  int initialPosition;
};

std::vector<ServoPins> servoPins = {
  { 0, "Base", 0, 180, 90 },      // Channel 0: Base (0°–180°)
  { 1, "Shoulder", 0, 120, 45 },  // Channel 1: Shoulder (0°–120°)
  { 2, "Elbow", 0, 120, 45 },     // Channel 2: Elbow (0°–120°)
  { 3, "Gripper", 0, 90, 0 }      // Channel 3: Gripper (0°–90°)
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
AsyncWebSocket wsRobotArmInput("/RobotArmInput");

// HTML Interface for Claw
const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>
    /* Claw Controls */
    input[type=button] {
      background-color:red;color:white;border-radius:30px;width:100%;height:40px;font-size:20px;text-align:center;
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
      background: red;
      cursor: pointer;
    }
    .slider::-moz-range-thumb {
      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }
    .noselect {
      -webkit-touch-callout: none; /* iOS Safari */
        -webkit-user-select: none; /* Safari */
         -khtml-user-select: none; /* Konqueror HTML */
           -moz-user-select: none; /* Firefox */
            -ms-user-select: none; /* Internet Explorer/Edge */
                user-select: none; /* Non-prefixed version, currently
                                      supported by Chrome and Opera */
    }
    </style>
  </head>
  <body class="noselect" align="center" style="background-color:white">
    <h1 style="color: teal;text-align:center;"> MPB _RoboClawCAR_ </h1>
    <h2 style="color: teal;text-align:center;">3-DOF Robotic Claw</h2>

    <!-- Claw Controls -->
    <table id="clawTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Base:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Base" oninput='sendButtonInput("Base",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Shoulder:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="120" value="45" class="slider" id="Shoulder" oninput='sendButtonInput("Shoulder",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/>      
      <tr>
        <td style="text-align:left;font-size:25px"><b>Elbow:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="120" value="45" class="slider" id="Elbow" oninput='sendButtonInput("Elbow",value)'>
          </div>
        </td>
      </tr>  
      <tr/><tr/>      
      <tr>
        <td style="text-align:left;font-size:25px"><b>Gripper:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="90" value="0" class="slider" id="Gripper" oninput='sendButtonInput("Gripper",value)'>
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

    <script>
      // WebSocket for Claw Controls
      var webSocketRobotArmInputUrl = "ws:\/\/" + window.location.hostname + "/RobotArmInput";      
      var websocketRobotArmInput;
      
      function initRobotArmInputWebSocket() {
        websocketRobotArmInput = new WebSocket(webSocketRobotArmInputUrl);
        websocketRobotArmInput.onopen = function(event){};
        websocketRobotArmInput.onclose = function(event){setTimeout(initRobotArmInputWebSocket, 2000);};
        websocketRobotArmInput.onmessage = function(event){
          var keyValue = event.data.split(",");
          var button = document.getElementById(keyValue[0]);
          button.value = keyValue[1];
          if (button.id == "Record" || button.id == "Play") {
            button.style.backgroundColor = (button.value == "ON" ? "green" : "red");  
            enableDisableButtonsSliders(button);
          }
        };
      }
      
      function sendButtonInput(key, value) {
        var data = key + "," + value;
        websocketRobotArmInput.send(data);
      }
      
      function onclickButton(button) {
        button.value = (button.value == "ON") ? "OFF" : "ON" ;        
        button.style.backgroundColor = (button.value == "ON" ? "green" : "red");          
        var value = (button.value == "ON") ? 1 : 0 ;
        sendButtonInput(button.id, value);
        enableDisableButtonsSliders(button);
      }
      
      function enableDisableButtonsSliders(button) {
        if(button.id == "Play") {
          var disabled = "auto";
          if (button.value == "ON") {
            disabled = "none";            
          }
          document.getElementById("Base").style.pointerEvents = disabled;
          document.getElementById("Shoulder").style.pointerEvents = disabled;          
          document.getElementById("Elbow").style.pointerEvents = disabled;          
          document.getElementById("Gripper").style.pointerEvents = disabled; 
          document.getElementById("Record").style.pointerEvents = disabled;
        }
        if(button.id == "Record") {
          var disabled = "auto";
          if (button.value == "ON") {
            disabled = "none";            
          }
          document.getElementById("Play").style.pointerEvents = disabled;
        }        
      }

      window.onload = function() {
        initRobotArmInputWebSocket();
      };
      document.getElementById("mainTable").addEventListener("touchend", function(event){
        event.preventDefault()
      });      
    </script>
  </body>    
</html>
)HTMLHOMEPAGE";

// Claw Functions
void writeServoValues(int servoIndex, int value) {
  // Map the angle to the PWM range (150–375)
  int pwmValue = map(value, 0, 180, 150, 375);

  // Set the PWM value for the corresponding channel
  pwm.setPWM(servoPins[servoIndex].servoPin, 0, pwmValue);

  // Recording functionality
  if (recordSteps) {
    RecordedStep recordedStep;       
    if (recordedSteps.size() == 0) {
      for (int i = 0; i < servoPins.size(); i++) {
        recordedStep.servoIndex = i; 
        recordedStep.value = value;
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
}

void playRecordedRobotArmSteps() {
  if (recordedSteps.size() == 0) {
    return;
  }
  for (int i = 0; i < 4 && playRecordedSteps; i++) {
    RecordedStep &recordedStep = recordedSteps[i];
    int currentServoPosition = map(pwm.getPWM(servoPins[recordedStep.servoIndex].servoPin), 150, 375, 0, 180);
    while (currentServoPosition != recordedStep.value && playRecordedSteps) {
      currentServoPosition = (currentServoPosition > recordedStep.value ? currentServoPosition - 1 : currentServoPosition + 1); 
      writeServoValues(recordedStep.servoIndex, currentServoPosition);
      wsRobotArmInput.textAll(servoPins[recordedStep.servoIndex].servoName + "," + currentServoPosition);
      delay(50);
    }
  }
  delay(2000);
  for (int i = 4; i < recordedSteps.size() && playRecordedSteps ; i++) {
    RecordedStep &recordedStep = recordedSteps[i];
    delay(recordedStep.delayInStep);
    writeServoValues(recordedStep.servoIndex, recordedStep.value);
    wsRobotArmInput.textAll(servoPins[recordedStep.servoIndex].servoName + "," + recordedStep.value);
  }
}

// Web Server and WebSocket Handlers
void handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "File Not Found");
}

void onRobotArmInputWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        String myData = String((char *)data);
        int commaIndex = myData.indexOf(',');
        if (commaIndex > 0) {
          String key = myData.substring(0, commaIndex);
          String value = myData.substring(commaIndex + 1);
          Serial.printf("Key [%s] Value[%s]\n", key.c_str(), value.c_str());
          int valueInt = value.toInt();
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
    int currentAngle = map(pwm.getPWM(servoPins[i].servoPin), 150, 375, 0, 180);
    wsRobotArmInput.textAll(servoPins[i].servoName + "," + currentAngle);
  }
  wsRobotArmInput.textAll(String("Record,") + (recordSteps ? "ON" : "OFF"));
  wsRobotArmInput.textAll(String("Play,") + (playRecordedSteps ? "ON" : "OFF"));  
}

// Setup and Loop
void setUpPinModes() {
  // Initialize PCA9685
  Wire.begin();  // Initialize default I2C bus for PCA9685
  pwm.begin();
  pwm.setPWMFreq(50); // Set PWM frequency to 50Hz (standard for servos)

  // Set initial servo positions
  for (int i = 0; i < servoPins.size(); i++) {
    writeServoValues(i, servoPins[i].initialPosition);
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
      
  wsRobotArmInput.onEvent(onRobotArmInputWebSocketEvent);
  server.addHandler(&wsRobotArmInput);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  wsRobotArmInput.cleanupClients();
  if (playRecordedSteps) { 
    playRecordedRobotArmSteps();
  }
}