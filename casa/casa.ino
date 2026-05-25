#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <ESP32Servo.h>

const char* AP_SSID     = "CasaInteligente";
const char* AP_PASSWORD = "microc2026";
const int   AP_CANAL    = 6;

#define DHT_PIN     33
#define DHT_TYPE    DHT22
#define LDR_PIN     34
#define SERVO_PIN   18
#define LED_PIN     19
#define LED_FREQ    5000
#define LED_RES     19
#define VENTANA_CERRADA  0
#define VENTANA_ABIERTA  90

DHT       dht(DHT_PIN, DHT_TYPE);
Servo     ventana;
WebServer server(80);

float temperatura    = 0.0;
float humedad        = 0.0;
int   luzAmbiente    = 0;
bool  ventanaAbierta = false;
bool  luzEncendida   = false;
int   brilloLED      = 200;

unsigned long ultimaLectura = 0;
const unsigned long INTERVALO = 2000;

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Casa Inteligente</title>
<style>
  :root{--bg:#0a0e14;--card:#131820;--acc:#00d9a3;--text:#e8eaed;--muted:#6b7280;--border:#1f2937;}
  *{box-sizing:border-box;margin:0;padding:0;}
  body{background:var(--bg);color:var(--text);font-family:'Segoe UI',system-ui,sans-serif;min-height:100vh;padding:20px;max-width:480px;margin:0 auto;}
  .header{display:flex;justify-content:space-between;align-items:center;margin-bottom:24px;}
  .title{font-size:1.4rem;font-weight:600;}
  .subtitle{font-size:.7rem;color:var(--muted);text-transform:uppercase;margin-top:2px;}
  .status{display:flex;align-items:center;gap:6px;font-size:.7rem;color:var(--muted);}
  .dot{width:6px;height:6px;border-radius:50%;background:var(--acc);box-shadow:0 0 6px var(--acc);}
  .section{margin-bottom:24px;}
  .section-title{font-size:.7rem;letter-spacing:3px;color:var(--muted);text-transform:uppercase;margin-bottom:12px;}
  .sensors{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px;}
  .sensor{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:14px 10px;text-align:center;}
  .sensor-icon{font-size:1.4rem;margin-bottom:6px;}
  .sensor-val{font-size:1.6rem;font-weight:600;}
  .sensor-unit{font-size:.7rem;color:var(--muted);margin-top:3px;}
  .sensor-label{font-size:.65rem;color:var(--muted);margin-top:8px;text-transform:uppercase;}
  .actuator{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:16px;margin-bottom:10px;}
  .act-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;}
  .act-name{font-size:.95rem;font-weight:500;}
  .act-status{font-size:.7rem;padding:3px 10px;border-radius:4px;text-transform:uppercase;}
  .act-status.on{background:rgba(0,217,163,.15);color:var(--acc);}
  .act-status.off{background:rgba(107,114,128,.2);color:var(--muted);}
  .act-controls{display:flex;gap:8px;}
  .act-btn{flex:1;padding:10px;border:1px solid var(--border);border-radius:8px;background:transparent;color:var(--text);font-family:inherit;font-size:.8rem;cursor:pointer;transition:all .2s;}
  .act-btn:hover{border-color:var(--acc);color:var(--acc);}
  .brillo-row{margin-top:10px;display:flex;align-items:center;gap:10px;}
  .brillo-lbl{font-size:.7rem;color:var(--muted);}
  input[type=range]{flex:1;accent-color:var(--acc);}
</style>
</head>
<body>
<div class="header">
  <div><div class="title">Casa Inteligente</div><div class="subtitle">Unidad Central</div></div>
  <div class="status"><span class="dot"></span><span id="st">Conectado</span></div>
</div>
<div class="section">
  <div class="section-title">Sensores</div>
  <div class="sensors">
    <div class="sensor"><div class="sensor-icon">🌡️</div><div class="sensor-val" id="temp">--</div><div class="sensor-unit">°C</div><div class="sensor-label">Temperatura</div></div>
    <div class="sensor"><div class="sensor-icon">💧</div><div class="sensor-val" id="hum">--</div><div class="sensor-unit">%</div><div class="sensor-label">Humedad</div></div>
    <div class="sensor"><div class="sensor-icon">☀️</div><div class="sensor-val" id="luz">--</div><div class="sensor-unit">%</div><div class="sensor-label">Luz amb.</div></div>
  </div>
</div>
<div class="section">
  <div class="section-title">Actuadores</div>
  <div class="actuator">
    <div class="act-header"><div class="act-name">🪟 Ventana</div><div class="act-status off" id="vs">Cerrada</div></div>
    <div class="act-controls"><button class="act-btn" onclick="fetch('/ventana?estado=cerrar').then(actualizar)">Cerrar</button><button class="act-btn" onclick="fetch('/ventana?estado=abrir').then(actualizar)">Abrir</button></div>
  </div>
  <div class="actuator">
    <div class="act-header"><div class="act-name">💡 Luz</div><div class="act-status off" id="ls">Apagada</div></div>
    <div class="act-controls"><button class="act-btn" onclick="fetch('/led?estado=off').then(actualizar)">Apagar</button><button class="act-btn" onclick="fetch('/led?estado=on').then(actualizar)">Encender</button></div>
    <div class="brillo-row"><span class="brillo-lbl">BRILLO</span><input type="range" min="0" max="255" value="200" oninput="fetch('/brillo?v='+this.value)"><span class="brillo-lbl" id="bv">200</span></div>
  </div>
</div>
<script>
async function actualizar(){
  try{
    const d=await(await fetch('/datos')).json();
    document.getElementById('temp').textContent=d.temperatura.toFixed(1);
    document.getElementById('hum').textContent=d.humedad.toFixed(0);
    document.getElementById('luz').textContent=d.luz;
    document.getElementById('vs').textContent=d.ventana?'Abierta':'Cerrada';
    document.getElementById('vs').className='act-status '+(d.ventana?'on':'off');
    document.getElementById('ls').textContent=d.led?'Encendida':'Apagada';
    document.getElementById('ls').className='act-status '+(d.led?'on':'off');
    document.getElementById('st').textContent='Conectado';
  }catch(e){document.getElementById('st').textContent='Desconectado';}
}
setInterval(actualizar,2000);actualizar();
</script>
</body>
</html>
)rawliteral";

void leerSensores() {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) temperatura = t;
    if (!isnan(h)) humedad = h;
    luzAmbiente = map(analogRead(LDR_PIN), 0, 4095, 0, 100);
}

void abrirVentana()  { ventana.write(VENTANA_ABIERTA); ventanaAbierta = true;  }
void cerrarVentana() { ventana.write(VENTANA_CERRADA); ventanaAbierta = false; }
void toggleVentana() { ventanaAbierta ? cerrarVentana() : abrirVentana(); }
void encenderLED()   { ledcWrite(LED_PIN, brilloLED); luzEncendida = true;  }
void apagarLED()     { ledcWrite(LED_PIN, 0);         luzEncendida = false; }
void toggleLED()     { luzEncendida ? apagarLED() : encenderLED(); }

void handleRoot()  { server.send_P(200, "text/html", INDEX_HTML); }

void handleDatos() {
    String json = "{";
    json += "\"temperatura\":" + String(temperatura, 1) + ",";
    json += "\"humedad\":"     + String(humedad, 1)     + ",";
    json += "\"luz\":"         + String(luzAmbiente)    + ",";
    json += "\"ventana\":"     + String(ventanaAbierta ? "true" : "false") + ",";
    json += "\"led\":"         + String(luzEncendida   ? "true" : "false") + ",";
    json += "\"brillo\":"      + String(brilloLED);
    json += "}";
    server.send(200, "application/json", json);
}

void handleFoco() {
    toggleLED();
    String json = "{\"led\":" + String(luzEncendida ? "true" : "false") + "}";
    server.send(200, "application/json", json);
    Serial.printf("[HTTP] FOCO → %s\n", luzEncendida ? "ENCENDIDO" : "APAGADO");
}

void handlePuerta() {
    toggleVentana();
    String json = "{\"ventana\":" + String(ventanaAbierta ? "true" : "false") + "}";
    server.send(200, "application/json", json);
    Serial.printf("[HTTP] PUERTA → %s\n", ventanaAbierta ? "ABIERTA" : "CERRADA");
}

void handleClima() {
    String json = "{";
    json += "\"temperatura\":" + String(temperatura, 1) + ",";
    json += "\"humedad\":"     + String(humedad, 1);
    json += "}";
    server.send(200, "application/json", json);
    Serial.printf("[HTTP] CLIMA → %.1f C, %.0f %%\n", temperatura, humedad);
}

void handleVentanaSet() {
    if (server.hasArg("estado")) {
        if (server.arg("estado") == "abrir")  abrirVentana();
        if (server.arg("estado") == "cerrar") cerrarVentana();
    }
    server.send(200, "text/plain", "OK");
}

void handleLedSet() {
    if (server.hasArg("estado")) {
        if (server.arg("estado") == "on")  encenderLED();
        if (server.arg("estado") == "off") apagarLED();
    }
    server.send(200, "text/plain", "OK");
}

void handleBrillo() {
    if (server.hasArg("v")) {
        brilloLED = constrain(server.arg("v").toInt(), 0, 255);
        if (luzEncendida) ledcWrite(LED_PIN, brilloLED);
    }
    server.send(200, "text/plain", "OK");
}

void setup() {
    Serial.begin(115200);
    dht.begin();
    ledcAttach(LED_PIN, LED_FREQ, LED_RES);
    ledcWrite(LED_PIN, 0);
    ESP32PWM::allocateTimer(2);
    ventana.setPeriodHertz(50);
    ventana.attach(SERVO_PIN, 500, 2400);
    ventana.write(VENTANA_CERRADA);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CANAL);
    Serial.printf("AP: %s\n", AP_SSID);
    Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("MAC: %s\n", WiFi.macAddress().c_str());

    server.on("/",        handleRoot);
    server.on("/datos",   handleDatos);
    server.on("/foco",    handleFoco);
    server.on("/puerta",  handlePuerta);
    server.on("/clima",   handleClima);
    server.on("/ventana", handleVentanaSet);
    server.on("/led",     handleLedSet);
    server.on("/brillo",  handleBrillo);
    server.begin();
    Serial.println("Todo listo");
}

void loop() {
    server.handleClient();
    if (millis() - ultimaLectura > INTERVALO) {
        ultimaLectura = millis();
        leerSensores();
    }
}