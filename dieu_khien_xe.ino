#include <WiFi.h>
#include <WebServer.h>

// Cau hinh chan cam
const int in1 = 26;
const int in2 = 25;
const int in3 = 33;
const int in4 = 32;
const int ENA = 27;
const int ENB = 14;
const int sensor =15;
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int LED = 2;
int obstacle;
float distance = 100; // Them bien global de luu khoang cach
int speed = 100;
const float SAFE_DISTANCE_CM = 20.0f;
char currentCommand = 'S';
// Cau hinh Wifi Access point
const char* ssid = "ESP32_CAR";
const char* password = "12345678abc";
// Create web server on port 80
WebServer server(80);

//---------- Setup function------------
void setup() {
  Serial.begin(115200);
  //Cam bien IR
  pinMode(sensor, INPUT);
  //Dong co
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  ledcSetup(0, 1000, 8); 
  ledcSetup(1, 1000, 8);
  ledcAttachPin(ENA, 0);
  ledcAttachPin(ENB, 1);
  //Cam bien sieu am
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED, OUTPUT);
  // Start Wi-Fi access point
  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  // Define HTTP routes
  server.on("/", handleRoot);
  server.on("/F", forward);
  server.on("/B", backward);
  server.on("/L", left);
  server.on("/R", right);
  server.on("/S", stopCar);
  server.begin();
}

//----------MAIN----------------------
void loop() {
  //Cam bien sieu am
  long duration;
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2; // Luu vao bien global
  obstacle = digitalRead(sensor);  // Luu vao bien global
  
  // Luon lang nghe lenh tu web
  server.handleClient();
  
  // Kiem tra vat can va dieu khien LED + chi dung forward
  bool blocked = (obstacle == 0 || distance < SAFE_DISTANCE_CM);
  if(blocked){
    digitalWrite(LED, HIGH);
    if(currentCommand == 'F'){
      digitalWrite(in1, LOW); digitalWrite(in2, LOW);
      digitalWrite(in3, LOW); digitalWrite(in4, LOW);
      ledcWrite(0, 0);
      ledcWrite(1, 0);
      currentCommand = 'S';
    }
  } else {
    digitalWrite(LED, LOW);
  }
  
  delay(10); // Giam delay de doc cam bien nhanh hon
}

//------------ Giao diện HTML---------
void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html><html>
    <head>
      <title>ESP32 Car Control</title>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <style>
        body { text-align: center; font-family: sans-serif; }
        button {
          width: 100px; height: 50px;
          font-size: 16px; margin: 10px;
          user-select: none; /* Tat select text */
          -webkit-user-select: none; /* Safari */
          -moz-user-select: none; /* Firefox */
          -ms-user-select: none; /* IE/Edge */
          touch-action: manipulation; /* Tat zoom khi double-tap */
        }
      </style>
      <script>
        function sendCommand(cmd) {
          fetch("/" + cmd);
        }

        function setupButton(id, command) {
          const btn = document.getElementById(id);
          btn.addEventListener('mousedown', () => sendCommand(command));
          btn.addEventListener('mouseup', () => sendCommand('S'));
          btn.addEventListener('touchstart', () => sendCommand(command));
          btn.addEventListener('touchend', () => sendCommand('S'));
        }

        window.onload = () => {
          setupButton("forward", "F");
setupButton("backward", "B");
          setupButton("left", "L");
          setupButton("right", "R");
        };
      </script>
    </head>
    <body>
      <h2>ESP32 Web Controlled Car</h2>
      <div>
        <button id="forward">Forward</button><br>
        <button id="left">Left</button>
        <button id="right">Right</button><br>
        <button id="backward">Backward</button>
      </div>
    </body>
    </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

//----------- Chuyen dong robot-------------
void backward() {
  // Lui duoc khi co vat can
  digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
  ledcWrite(0, speed); 
  ledcWrite(1, speed);
  currentCommand = 'B';
  server.send(200, "text/plain", "Backward");
}

void forward() {
  // Dung gia tri cam bien da doc tu loop()
  if(obstacle == 1 && distance > SAFE_DISTANCE_CM){
    digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
    digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
    ledcWrite(0, speed); 
    ledcWrite(1, speed);
    currentCommand = 'F';
    server.send(200, "text/plain", "Forward");
  } else {
    // Co vat can -> Khong tien
    digitalWrite(in1, LOW); digitalWrite(in2, LOW);
    digitalWrite(in3, LOW); digitalWrite(in4, LOW);
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    currentCommand = 'S';
    server.send(200, "text/plain", "Blocked");
  }
}

void left() {
  // Re trai duoc khi co vat can
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
  ledcWrite(0, 200); 
  ledcWrite(1, 200);
  currentCommand = 'L';
  server.send(200, "text/plain", "Left");
}

void right() {
  // Re phai duoc khi co vat can
  digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
  ledcWrite(0, 200); 
  ledcWrite(1, 200);
  currentCommand = 'R';
  server.send(200, "text/plain", "Right");
}
void stopCar() {
  digitalWrite(in1, LOW); digitalWrite(in2, LOW);
  digitalWrite(in3, LOW); digitalWrite(in4, LOW);
  ledcWrite(0, 0);
  ledcWrite(1, 0);
  currentCommand = 'S';
  server.send(200, "text/plain", "Stop");
}