#include <WiFi.h>
#include <WebServer.h>
#include <BluetoothSerial.h>
#include <ESPmDNS.h>
#include <Bounce2.h>  // Bounce2 library

// Replace with your network credentials
const char* ssid = "Host";
const char* password = "12345678";

// Create WebServer object on port 80
WebServer server(80);

// Create Bluetooth Serial object
BluetoothSerial SerialBT;

// Relay pin definitions
const int relay1 = 19;
const int relay2 = 21;
const int relay3 = 22;
const int relay4 = 23;

// Manual switch pin definitions
const int switch1 = 13;
const int switch2 = 12;
const int switch3 = 14;
const int switch4 = 27;

// Relay states
bool relay1State = HIGH;
bool relay2State = HIGH;
bool relay3State = HIGH;
bool relay4State = HIGH;

// Create Bounce objects for debouncing
Bounce bounce1 = Bounce();
Bounce bounce2 = Bounce();
Bounce bounce3 = Bounce();
Bounce bounce4 = Bounce();

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize Bluetooth Serial
  SerialBT.begin("ESP32_Smart_Switch");

  // Set relay pins as outputs
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);

  // Set manual switch pins as inputs with pull-up resistors
  pinMode(switch1, INPUT_PULLUP);
  pinMode(switch2, INPUT_PULLUP);
  pinMode(switch3, INPUT_PULLUP);
  pinMode(switch4, INPUT_PULLUP);

  // Attach the switches to the Bounce objects
  bounce1.attach(switch1);
  bounce2.attach(switch2);
  bounce3.attach(switch3);
  bounce4.attach(switch4);

  // Set the debounce interval (in milliseconds)
  bounce1.interval(50); // Adjusted debounce interval
  bounce2.interval(50);
  bounce3.interval(50);
  bounce4.interval(50);

  // Initialize relays to off state (HIGH)
  digitalWrite(relay1, relay1State);
  digitalWrite(relay2, relay2State);
  digitalWrite(relay3, relay3State);
  digitalWrite(relay4, relay4State);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Setup mDNS
  if (!MDNS.begin("smart-m1")) { // Change "smart-m1" to your desired domain name
    Serial.println("Error setting up mDNS responder!");
  } else {
    Serial.println("mDNS responder started");
    Serial.print("Access the device at http://smart-m1.local");
  }

  // Define routes for web server
  server.on("/", handleRoot);
  server.on("/relay_states", handleRelayStates); // Route to get relay states
  server.on("/relay1/on", []() { setRelayState(1, LOW); });
  server.on("/relay1/off", []() { setRelayState(1, HIGH); });
  server.on("/relay2/on", []() { setRelayState(2, LOW); });
  server.on("/relay2/off", []() { setRelayState(2, HIGH); });
  server.on("/relay3/on", []() { setRelayState(3, LOW); });
  server.on("/relay3/off", []() { setRelayState(3, HIGH); });
  server.on("/relay4/on", []() { setRelayState(4, LOW); });
  server.on("/relay4/off", []() { setRelayState(4, HIGH); });

  // Start the server
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
  
  // Update Bounce instances
  bounce1.update();
  bounce2.update();
  bounce3.update();
  bounce4.update();

  // Check manual switches and update relay states
  if (bounce1.fell()) {
    setRelayState(1, LOW);
  }
  if (bounce1.rose()) {
    setRelayState(1, HIGH);
  }
  if (bounce2.fell()) {
    setRelayState(2, LOW);
  }
  if (bounce2.rose()) {
    setRelayState(2, HIGH);
  }
  if (bounce3.fell()) {
    setRelayState(3, LOW);
  }
  if (bounce3.rose()) {
    setRelayState(3, HIGH);
  }
  if (bounce4.fell()) {
    setRelayState(4, LOW);
  }
  if (bounce4.rose()) {
    setRelayState(4, HIGH);
  }
}

// Helper function to set relay state and respond to web requests
void setRelayState(int relayNum, bool state) {
  bool* relayState;
  int relayPin;
  
  switch (relayNum) {
    case 1:
      relayState = &relay1State;
      relayPin = relay1;
      break;
    case 2:
      relayState = &relay2State;
      relayPin = relay2;
      break;
    case 3:
      relayState = &relay3State;
      relayPin = relay3;
      break;
    case 4:
      relayState = &relay4State;
      relayPin = relay4;
      break;
    default:
      return; // Invalid relay number
  }

  *relayState = state;
  digitalWrite(relayPin, *relayState);

  String response = String("Relay ") + relayNum + (state == LOW ? " ON" : " OFF");
  server.send(200, "text/plain", response);
}

void handleRoot() {
  String html = "<html><head>"
                "<style>"
                "body { font-family: Arial, sans-serif; background-color: #f0f0f0; text-align: center; }"
                "h1 { color: #333; }"
                "button { margin: 10px; padding: 15px 30px; font-size: 16px; border: none; border-radius: 5px; cursor: pointer; }"
                ".on { background-color: #4CAF50; color: white; transition: background-color 0.3s; }"
                ".off { background-color: #f44336; color: white; transition: background-color 0.3s; }"
                "button:hover { opacity: 0.8; }"
                "</style>"
                "<script>"
                "function toggleRelay(relay, state) {"
                "  var xhr = new XMLHttpRequest();"
                "  xhr.open('GET', '/' + relay + '/' + state, true);"
                "  xhr.send();"
                "  xhr.onload = function() {"
                "    if (xhr.status == 200) {"
                "      document.getElementById(relay).innerHTML = state == 'on' ? 'ON' : 'OFF';"
                "    }"
                "  }"
                "}"
                "function updateRelayStates() {"
                "  var xhr = new XMLHttpRequest();"
                "  xhr.open('GET', '/relay_states', true);"
                "  xhr.send();"
                "  xhr.onload = function() {"
                "    if (xhr.status == 200) {"
                "      var states = JSON.parse(xhr.responseText);"
                "      document.getElementById('relay1').innerHTML = states.relay1 ? 'ON' : 'OFF';"
                "      document.getElementById('relay2').innerHTML = states.relay2 ? 'ON' : 'OFF';"
                "      document.getElementById('relay3').innerHTML = states.relay3 ? 'ON' : 'OFF';"
                "      document.getElementById('relay4').innerHTML = states.relay4 ? 'ON' : 'OFF';"
                "    }"
                "  }"
                "}"
                "setInterval(updateRelayStates, 1000);" // Update every 1 second
                "</script>"
                "</head>"
                "<body onload='updateRelayStates()'>"
                "<h1>ESP32 Smart Switch</h1>"
                "<p>Relay 1: <span id='relay1'>" + String(relay1State == LOW ? "ON" : "OFF") + "</span></p>"
                "<button class='on' onclick=\"toggleRelay('relay1', 'on')\">Turn Relay 1 ON</button>"
                "<button class='off' onclick=\"toggleRelay('relay1', 'off')\">Turn Relay 1 OFF</button>"
                "<p>Relay 2: <span id='relay2'>" + String(relay2State == LOW ? "ON" : "OFF") + "</span></p>"
                "<button class='on' onclick=\"toggleRelay('relay2', 'on')\">Turn Relay 2 ON</button>"
                "<button class='off' onclick=\"toggleRelay('relay2', 'off')\">Turn Relay 2 OFF</button>"
                "<p>Relay 3: <span id='relay3'>" + String(relay3State == LOW ? "ON" : "OFF") + "</span></p>"
                "<button class='on' onclick=\"toggleRelay('relay3', 'on')\">Turn Relay 3 ON</button>"
                "<button class='off' onclick=\"toggleRelay('relay3', 'off')\">Turn Relay 3 OFF</button>"
                "<p>Relay 4: <span id='relay4'>" + String(relay4State == LOW ? "ON" : "OFF") + "</span></p>"
                "<button class='on' onclick=\"toggleRelay('relay4', 'on')\">Turn Relay 4 ON</button>"
                "<button class='off' onclick=\"toggleRelay('relay4', 'off')\">Turn Relay 4 OFF</button>"
                "</body></html>";
  server.send(200, "text/html", html);
}

void handleRelayStates() {
  String json = "{";
  json += "\"relay1\":" + String(relay1State == LOW ? "true" : "false") + ",";
  json += "\"relay2\":" + String(relay2State == LOW ? "true" : "false") + ",";
  json += "\"relay3\":" + String(relay3State == LOW ? "true" : "false") + ",";
  json += "\"relay4\":" + String(relay4State == LOW ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}