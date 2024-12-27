#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WLAN credentials
const char* ssid = "extender";
const char* password = "xxxxxxxxxxx";

// Pin definitions for Molex Fan
const int MOLEX_FAN_PWM = 1;     // GPIO1 for PWM
const int MOLEX_FAN_RPM = 2;     // GPIO2 for RPM signal
const int GPIO_SWITCH = 38;       // GPIO38 for switch
const int molexFanEnablePin = 8;

// Pin definitions for 0-10V Fan
const int ANALOG_FAN_PWM = 9;     // GPIO9 for PWM
const int ANALOG_FAN_ENABLE = 11; // GPIO11 for enable

// PWM configuration for Molex Fan
const int MOLEX_PWM_FREQ = 25000;  // 25 kHz
const int MOLEX_PWM_CHANNEL = 0;
const int MOLEX_PWM_RESOLUTION = 8;

// PWM configuration for 0-10V Fan
const int ANALOG_PWM_FREQ = 5000;   // 5 kHz
const int ANALOG_PWM_CHANNEL = 1;
const int ANALOG_PWM_RESOLUTION = 8;

WebServer server(80);

// Global variables
volatile unsigned long lastPulseTime = 0;
volatile unsigned int pulseCount = 0;
unsigned int rpm = 0;
int molexFanSpeed = 0;           // Current speed for Molex fan (0-100%)
float analogFanVoltage = 6.5;    // Current voltage for 0-10V fan

// Interrupt Service Routine for RPM measurement
void IRAM_ATTR rpmISR() {
  pulseCount++;
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // IP address
  display.setCursor(0,0);
  display.print("IP: ");
  display.println(WiFi.localIP());
  
  // Molex fan speed
  display.setCursor(0,16);
  display.print("Molex: ");
  display.print(molexFanSpeed);
  display.println("%");
  
  // Analog fan voltage
  display.setCursor(0,32);
  display.print("0-10V: ");
  display.print(analogFanVoltage, 1);
  display.println("V");
  
  // RPM
  display.setCursor(0,48);
  display.print(rpm);
  display.println(" RPM");
  
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize OLED
  Wire.begin(6, 5);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Starting...");
  display.display();
  
  // Initialize EEPROM
  EEPROM.begin(512);
  
  // Load saved values
  molexFanSpeed = EEPROM.read(0);
  if(molexFanSpeed > 100) molexFanSpeed = 50;
  
  analogFanVoltage = EEPROM.readFloat(4);
  if(analogFanVoltage < 0 || analogFanVoltage > 10) analogFanVoltage = 6.5;
  
  // Configure Molex fan pins
  pinMode(MOLEX_FAN_RPM, INPUT_PULLUP);
  pinMode(GPIO_SWITCH, OUTPUT);
  digitalWrite(GPIO_SWITCH, LOW);
  pinMode(molexFanEnablePin, OUTPUT);
  digitalWrite(molexFanEnablePin, LOW);
  
  // Configure 0-10V fan pins
  pinMode(ANALOG_FAN_ENABLE, OUTPUT);
  
  // Configure PWM channels
  ledcSetup(MOLEX_PWM_CHANNEL, MOLEX_PWM_FREQ, MOLEX_PWM_RESOLUTION);
  ledcAttachPin(MOLEX_FAN_PWM, MOLEX_PWM_CHANNEL);
  
  ledcSetup(ANALOG_PWM_CHANNEL, ANALOG_PWM_FREQ, ANALOG_PWM_RESOLUTION);
  ledcAttachPin(ANALOG_FAN_PWM, ANALOG_PWM_CHANNEL);
  
  // Set initial values
  ledcWrite(MOLEX_PWM_CHANNEL, map(molexFanSpeed, 0, 100, 255, 0));
  setAnalogFanVoltage(analogFanVoltage);
  
  // Setup RPM interrupt
  attachInterrupt(digitalPinToInterrupt(MOLEX_FAN_RPM), rpmISR, FALLING);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Connecting WiFi...");
  display.display();
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // Define web server routes
  server.on("/", handleRoot);
  server.on("/setMolexSpeed", handleSetMolexSpeed);
  server.on("/setAnalogVoltage", handleSetAnalogVoltage);
  server.on("/getData", handleGetData);
  server.on("/switch", handleSwitch);
  server.begin();
  
  updateDisplay();
}

void loop() {
  server.handleClient();
  
  static unsigned long lastDisplayUpdate = 0;
  static unsigned long lastRpmCalc = 0;
  
  // Calculate RPM every 500ms
  if(millis() - lastRpmCalc >= 500) {
    rpm = (pulseCount * 120);
    pulseCount = 0;
    lastRpmCalc = millis();
  }
  
  // Update display every 1000ms
  if(millis() - lastDisplayUpdate >= 1000) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
}

void setAnalogFanVoltage(float voltage) {
  if (voltage <= 0.0) {
    digitalWrite(ANALOG_FAN_ENABLE, LOW);
    ledcWrite(ANALOG_PWM_CHANNEL, 0);
  } else {
    digitalWrite(ANALOG_FAN_ENABLE, HIGH);
    int dutyCycle = (voltage * 255) / 10.0;
    ledcWrite(ANALOG_PWM_CHANNEL, dutyCycle);
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html>\n"
                "<html>\n"
                "<head>\n"
                    "<title>Fan Control System</title>\n"
                    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
                    "<style>\n"
                        "body { font-family: Arial; text-align: center; margin: 20px; }\n"
                        ".slider { width: 80%; max-width: 400px; }\n"
                        ".control-group { margin: 30px 0; }\n"
                        "button { padding: 10px 20px; margin: 5px; }\n"
                    "</style>\n"
                "</head>\n"
                "<body>\n"
                    "<h1>Fan Control System</h1>\n"
                    "<div class=\"control-group\">\n"
                        "<h2>Molex Fan Control</h2>\n"
                        "<input type=\"range\" min=\"0\" max=\"100\" value=\"" + String(molexFanSpeed) + "\" class=\"slider\" id=\"molexSlider\">\n"
                        "<p>Speed: <span id=\"molexValue\">" + String(molexFanSpeed) + "</span>%</p>\n"
                        "<p>RPM: <span id=\"rpmValue\">" + String(rpm) + "</span></p>\n"
                        "<p>\n"
                            "<button onclick=\"switchGPIO(1)\">GPIO 38 ON</button>\n"
                            "<button onclick=\"switchGPIO(0)\">GPIO 38 OFF</button>\n"
                        "</p>\n"
                    "</div>\n"
                    "<div class=\"control-group\">\n"
                        "<h2>0-10V Fan Control</h2>\n"
                        "<input type=\"range\" min=\"0\" max=\"100\" value=\"" + String(analogFanVoltage * 10) + "\" class=\"slider\" id=\"analogSlider\">\n"
                        "<p>Voltage: <span id=\"analogValue\">" + String(analogFanVoltage) + "</span>V</p>\n"
                    "</div>\n"
                    "<script>\n"
                        "var molexSlider = document.getElementById('molexSlider');\n"
                        "var analogSlider = document.getElementById('analogSlider');\n"
                        "var molexValue = document.getElementById('molexValue');\n"
                        "var analogValue = document.getElementById('analogValue');\n"
                        "var rpmValue = document.getElementById('rpmValue');\n"
                        
                        "molexSlider.oninput = function() {\n"
                            "molexValue.innerHTML = this.value;\n"
                            "fetch('/setMolexSpeed?speed=' + this.value);\n"
                        "};\n"
                        
                        "analogSlider.oninput = function() {\n"
                            "let voltage = (this.value / 10).toFixed(1);\n"
                            "analogValue.innerHTML = voltage;\n"
                            "fetch('/setAnalogVoltage?voltage=' + voltage);\n"
                        "};\n"
                        
                        "function switchGPIO(state) {\n"
                            "fetch('/switch?state=' + state);\n"
                        "}\n"
                        
                        "setInterval(function() {\n"
                            "fetch('/getData')\n"
                            ".then(response => response.json())\n"
                            ".then(data => {\n"
                                "rpmValue.innerHTML = data.rpm;\n"
                                "molexValue.innerHTML = data.molexSpeed;\n"
                                "analogValue.innerHTML = data.analogVoltage;\n"
                                "molexSlider.value = data.molexSpeed;\n"
                                "analogSlider.value = data.analogVoltage * 10;\n"
                            "});\n"
                        "}, 500);\n"
                    "</script>\n"
                "</body>\n"
                "</html>\n";
  server.send(200, "text/html", html);
}

void handleSetMolexSpeed() {
  if(server.hasArg("speed")) {
    molexFanSpeed = server.arg("speed").toInt();
    molexFanSpeed = constrain(molexFanSpeed, 0, 100);
    
    if(molexFanSpeed == 0) {
      ledcWrite(MOLEX_PWM_CHANNEL, 0);
      digitalWrite(molexFanEnablePin, LOW);
    } else {
      digitalWrite(molexFanEnablePin, HIGH);
      ledcWrite(MOLEX_PWM_CHANNEL, map(molexFanSpeed, 0, 100, 255, 0));
    }
    
    EEPROM.write(0, molexFanSpeed);
    EEPROM.commit();
    
    server.send(200, "text/plain", "OK");
  }
}

void handleSetAnalogVoltage() {
  if(server.hasArg("voltage")) {
    analogFanVoltage = server.arg("voltage").toFloat();
    analogFanVoltage = constrain(analogFanVoltage, 0, 10);
    
    setAnalogFanVoltage(analogFanVoltage);
    
    EEPROM.writeFloat(4, analogFanVoltage);
    EEPROM.commit();
    
    server.send(200, "text/plain", "OK");
  }
}

void handleSwitch() {
  if(server.hasArg("state")) {
    if(server.arg("state") == "1") {
      digitalWrite(GPIO_SWITCH, HIGH);
    } else {
      digitalWrite(GPIO_SWITCH, LOW);
    }
    server.send(200, "text/plain", "OK");
  }
}

void handleGetData() {
  String json = "{\"molexSpeed\":" + String(molexFanSpeed) + 
                ",\"analogVoltage\":" + String(analogFanVoltage) + 
                ",\"rpm\":" + String(rpm) + "}";
  server.send(200, "application/json", json);
}
