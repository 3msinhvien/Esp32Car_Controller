#include <WiFi.h>
#include <WebServer.h>

// Cau hinh chan cam
const int in1 = 26;
const int in2 = 25;
const int in3 = 33;
const int in4 = 32;
const int ENA = 27;
const int ENB = 14;
const int sensor = 15;
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int LED = 2;
const int TURN_SPEED = 200;
const int DRIVE_SPEED_MIN = 85;
const int DRIVE_SPEED_MAX = 120;
int obstacle;
float distance = 100; // Them bien global de luu khoang cach
int driveSpeed = 100;
int driveSpeedPercent = 100;
const float SAFE_DISTANCE_CM = 20.0f;
char currentCommand = 'S';
// Cau hinh Wifi Access point
const char* ssid = "ESP32_CAR";
const char* password = "12345678abc";
// Create web server on port 80
WebServer server(80);

int convertPercentToSpeed(int percent) {
  percent = constrain(percent, 1, 100);
  float ratio = float(percent - 1) / 99.0f;
  return int(DRIVE_SPEED_MIN + ratio * (DRIVE_SPEED_MAX - DRIVE_SPEED_MIN) + 0.5f);
}

int convertSpeedToPercent(int speed) {
  speed = constrain(speed, DRIVE_SPEED_MIN, DRIVE_SPEED_MAX);
  float ratio = float(speed - DRIVE_SPEED_MIN) / float(DRIVE_SPEED_MAX - DRIVE_SPEED_MIN);
  return int(1.0f + ratio * 99.0f + 0.5f);
}

//---------- Setup function------------
void setup() {
  Serial.begin(115200);
  driveSpeed = constrain(driveSpeed, DRIVE_SPEED_MIN, DRIVE_SPEED_MAX);
  driveSpeedPercent = convertSpeedToPercent(driveSpeed);
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
  server.on("/status", handleStatus);
  server.on("/speed", handleSpeed);
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
  distance = duration * 0.034f / 2.0f; // Luu vao bien global
  obstacle = digitalRead(sensor);      // Luu vao bien global

  // Luon lang nghe lenh tu web
  server.handleClient();

  // Kiem tra vat can va dieu khien LED + chi dung forward
  bool blocked = (obstacle == 0 || distance < SAFE_DISTANCE_CM);
  if (blocked) {
    digitalWrite(LED, HIGH);
    if (currentCommand == 'F') {
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

//------------ Giao dien HTML---------
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html><html lang="en">
<head>
  <meta charset="utf-8">
  <title>ESP32 Car Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    :root { color-scheme: light dark; }
    body {
      margin: 0;
      font-family: "Segoe UI", Tahoma, sans-serif;
      background: linear-gradient(160deg, #eceff1, #cfd8dc);
      color: #263238;
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
    }
    main {
      width: min(480px, 90vw);
      background: rgba(255,255,255,0.92);
      border-radius: 18px;
      box-shadow: 0 18px 40px rgba(38,50,56,0.18);
      padding: 24px;
      backdrop-filter: blur(8px);
    }
    h1 {
      margin: 0 0 6px;
      font-size: 1.8rem;
      letter-spacing: 0.5px;
    }
    p.subtitle {
      margin: 0 0 20px;
      color: #546e7a;
    }
    section {
      margin-bottom: 22px;
      border-radius: 14px;
      background: rgba(236, 239, 241, 0.7);
      padding: 16px;
    }
    section:last-of-type { margin-bottom: 0; }
    .status-row {
      display: flex;
      justify-content: space-between;
      margin: 6px 0;
      font-size: 0.95rem;
    }
    .status-label { color: #607d8b; }
    .status-value { font-weight: 600; }
    .button-grid {
      display: grid;
      grid-template-columns: repeat(3, minmax(90px, 1fr));
      grid-auto-rows: 72px;
      gap: 12px;
      justify-items: stretch;
      align-items: stretch;
    }
    .grid-spacer {
      pointer-events: none;
      visibility: hidden;
    }
    button.control {
      width: 100%;
      height: 100%;
      min-height: 72px;
      font-size: 1rem;
      font-weight: 600;
      background: linear-gradient(135deg, #29b6f6, #0288d1);
      color: #fff;
      border: none;
      border-radius: 12px;
      box-shadow: 0 6px 14px rgba(2,136,209,0.28);
      transition: transform 0.12s ease, box-shadow 0.12s ease;
      user-select: none;
      touch-action: manipulation;
    }
    button.control:active {
      transform: scale(0.97);
      box-shadow: 0 2px 6px rgba(2,136,209,0.4);
    }
    button.secondary {
      background: linear-gradient(135deg, #ff8a65, #f4511e);
      box-shadow: 0 6px 14px rgba(244,81,30,0.28);
    }
    button.stop {
      background: linear-gradient(135deg, #ef5350, #c62828);
      box-shadow: 0 6px 16px rgba(198,40,40,0.32);
    }
    .speed-row {
      display: flex;
      align-items: center;
      gap: 12px;
    }
    input[type="range"] {
      flex: 1;
      accent-color: #0288d1;
    }
    .speed-value {
      min-width: 48px;
      text-align: right;
      font-weight: 600;
    }
    @media (prefers-color-scheme: dark) {
      body { background: linear-gradient(160deg, #263238, #37474f); color: #eceff1; }
      main { background: rgba(38,50,56,0.85); }
      section { background: rgba(69,90,100,0.45); }
      .status-label { color: #b0bec5; }
    }
  </style>
  <script>
    function sendCommand(cmd) {
      fetch("/" + cmd).catch(() => {});
    }

    function setupButton(id, command) {
      const btn = document.getElementById(id);
      if (!btn) return;
      const pressStart = (event) => {
        event.preventDefault();
        sendCommand(command);
      };
      const pressStop = (event) => {
        event.preventDefault();
        sendCommand('S');
      };
      btn.addEventListener('mousedown', pressStart);
      btn.addEventListener('mouseup', pressStop);
      btn.addEventListener('mouseleave', pressStop);
      btn.addEventListener('touchstart', pressStart, { passive: false });
      btn.addEventListener('touchend', pressStop, { passive: false });
      btn.addEventListener('touchcancel', pressStop, { passive: false });
      btn.addEventListener('contextmenu', (event) => event.preventDefault());
    }

    let adjustingSpeed = false;

    function initControls() {
      setupButton("forward", "F");
      setupButton("backward", "B");
      setupButton("left", "L");
      setupButton("right", "R");
      const stopBtn = document.getElementById('stop');
      if (stopBtn) {
        stopBtn.addEventListener('click', () => sendCommand('S'));
      }

      const slider = document.getElementById('speed');
      const speedValue = document.getElementById('speedValue');
      if (slider && speedValue) {
        speedValue.textContent = Number(slider.value);
        slider.addEventListener('input', (event) => {
          adjustingSpeed = true;
          const value = Number(event.target.value);
          speedValue.textContent = value;
        });
        slider.addEventListener('change', (event) => {
          adjustingSpeed = false;
          const value = Number(event.target.value);
          speedValue.textContent = value;
          fetch(`/speed?value=${value}`).catch(() => {});
        });
        document.addEventListener('mouseup', () => { adjustingSpeed = false; });
        document.addEventListener('touchend', () => { adjustingSpeed = false; });
      }
    }

    function updateStatusCard(data) {
      const distanceField = document.getElementById('distance');
      const obstacleField = document.getElementById('obstacle');
      const commandField = document.getElementById('command');
      const ledField = document.getElementById('led');
      const slider = document.getElementById('speed');
      const speedValue = document.getElementById('speedValue');
      const driveSpeedField = document.getElementById('driveSpeed');
      if (distanceField && typeof data.distance === 'number') {
        distanceField.textContent = `${data.distance.toFixed(1)} cm`;
      }
      if (obstacleField) {
        obstacleField.textContent = data.obstacle ? 'Clear' : 'Blocked';
        obstacleField.style.color = data.obstacle ? '#388e3c' : '#c62828';
      }
      if (commandField) {
        const map = { F: 'Forward', B: 'Backward', L: 'Left', R: 'Right', S: 'Stopped' };
        commandField.textContent = map[data.command] || 'Unknown';
      }
      if (ledField) {
        ledField.textContent = data.blocked ? 'ON' : 'OFF';
        ledField.style.color = data.blocked ? '#ef5350' : '#546e7a';
      }
      if (driveSpeedField && typeof data.driveSpeed === 'number') {
        driveSpeedField.textContent = `${data.driveSpeed}`;
      }
      if (!adjustingSpeed && slider && speedValue && typeof data.speed === 'number') {
        slider.value = data.speed;
        speedValue.textContent = data.speed;
      }
    }

    async function pollStatus() {
      try {
        const response = await fetch('/status', { cache: 'no-store' });
        if (!response.ok) return;
        const data = await response.json();
        updateStatusCard(data);
      } catch (error) {
        // Silent retry on network hiccups
      }
    }

    window.onload = () => {
      initControls();
      pollStatus();
      setInterval(pollStatus, 600);
    };
  </script>
</head>
<body>
  <main>
    <header>
      <h1>ESP32 Car Controller</h1>
      <p class="subtitle">Nhan va giu nut de dieu khien. Kiem tra trang thai xe ben duoi.</p>
    </header>
    <section>
      <h2>Trang thai</h2>
      <div class="status-row"><span class="status-label">Khoang cach</span><span id="distance" class="status-value">-- cm</span></div>
      <div class="status-row"><span class="status-label">Cam bien</span><span id="obstacle" class="status-value">--</span></div>
      <div class="status-row"><span class="status-label">Lenh hien tai</span><span id="command" class="status-value">--</span></div>
      <div class="status-row"><span class="status-label">Den canh bao</span><span id="led" class="status-value">--</span></div>
      <div class="status-row"><span class="status-label">PWM tien/lui</span><span id="driveSpeed" class="status-value">--</span></div>
    </section>
    <section>
      <h2>Toc do (1 - 100)</h2>
      <div class="speed-row">
  <input id="speed" type="range" min="1" max="100" value="43">
  <span class="speed-value"><span id="speedValue">43</span> / 100</span>
      </div>
    </section>
    <section>
      <h2>Dieu khien</h2>
      <div class="button-grid">
        <div class="grid-spacer"></div>
        <button id="forward" class="control">Tien</button>
        <div class="grid-spacer"></div>
        <button id="left" class="control secondary">Trai</button>
        <button id="stop" class="control stop">Dung</button>
        <button id="right" class="control secondary">Phai</button>
        <div class="grid-spacer"></div>
        <button id="backward" class="control">Lui</button>
        <div class="grid-spacer"></div>
      </div>
    </section>
  </main>
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
  ledcWrite(0, driveSpeed);
  ledcWrite(1, driveSpeed);
  currentCommand = 'B';
  server.send(200, "text/plain", "Backward");
}

void forward() {
  // Dung gia tri cam bien da doc tu loop()
  if(obstacle == 1 && distance > SAFE_DISTANCE_CM){
    digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
    digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
    ledcWrite(0, driveSpeed);
    ledcWrite(1, driveSpeed);
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
  ledcWrite(0, TURN_SPEED);
  ledcWrite(1, TURN_SPEED);
  currentCommand = 'L';
  server.send(200, "text/plain", "Left");
}

void right() {
  // Re phai duoc khi co vat can
  digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
  ledcWrite(0, TURN_SPEED);
  ledcWrite(1, TURN_SPEED);
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

void handleStatus() {
  bool blocked = (obstacle == 0 || distance < SAFE_DISTANCE_CM);
  String response = "{";
  response += "\"distance\":" + String(distance, 1) + ",";
  response += "\"obstacle\":" + String(obstacle) + ",";
  response += "\"blocked\":";
  response += blocked ? "true," : "false,";
  response += "\"speed\":" + String(driveSpeedPercent) + ",";
  response += "\"driveSpeed\":" + String(driveSpeed) + ",";
  response += "\"command\":\"";
  response += currentCommand;
  response += "\"}";
  server.send(200, "application/json", response);
}

void handleSpeed() {
  if (!server.hasArg("value")) {
    server.send(400, "text/plain", "Missing value");
    return;
  }
  int requestedPercent = server.arg("value").toInt();
  requestedPercent = constrain(requestedPercent, 1, 100);
  driveSpeedPercent = requestedPercent;
  driveSpeed = convertPercentToSpeed(driveSpeedPercent);

  if (currentCommand == 'F' || currentCommand == 'B') {
    ledcWrite(0, driveSpeed);
    ledcWrite(1, driveSpeed);
  }

  String response = "{\"speed\":" + String(driveSpeedPercent) + ",";
  response += "\"driveSpeed\":" + String(driveSpeed) + "}";
  server.send(200, "application/json", response);
}
