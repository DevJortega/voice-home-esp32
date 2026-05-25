# Voice Home ESP32 🎙️🏠

Sistema domótico basado en reconocimiento de voz utilizando dos placas ESP32 comunicadas mediante HTTP sobre WiFi.

---

# Descripción del sistema

El proyecto está compuesto por dos nodos principales:

## ⌚ Smartwatch — ESP32-S3 Super Mini

Encargado de:

- Capturar audio mediante un micrófono digital INMP441
- Ejecutar inferencia local con un modelo de inteligencia artificial generado en Edge Impulse
- Reconocer comandos de voz en tiempo real
- Enviar comandos HTTP a la casa inteligente

## 🏠 Casa Inteligente — ESP32 WROOM-32

Encargado de:

- Crear una red WiFi en modo Access Point
- Actuar como servidor HTTP
- Controlar actuadores y sensores:
  - LED PWM
  - Servo motor
  - Sensor de temperatura y humedad
  - Sensor de luminosidad

---

# Comandos de voz reconocidos

| Comando | Acción |
|----------|---------|
| `foco` | Encender o apagar iluminación |
| `puerta` | Abrir o cerrar garaje (servo motor) |
| `clima` | Consultar temperatura y humedad |
| `ruido` | Clase negativa (sin acción) |

---

# Hardware utilizado

## ⌚ Smartwatch — ESP32-S3 Super Mini

| Componente | GPIO |
|------------|------|
| INMP441 SCK | GPIO 4 |
| INMP441 WS | GPIO 7 |
| INMP441 SD | GPIO 3 |
| OLED SDA | GPIO 10 |
| OLED SCL | GPIO 11 |
| Botón | GPIO 2 |

## 🏠 Casa Inteligente — ESP32 WROOM-32

| Componente | GPIO |
|------------|------|
| DHT22 | GPIO 33 |
| LDR | GPIO 34 |
| Servo motor | GPIO 18 |
| LED PWM | GPIO 19 |

---

# Arquitectura del sistema

```text
┌──────────────────────┐
│ Smartwatch ESP32-S3 │
│----------------------│
│ Micrófono INMP441    │
│ Modelo IA EdgeImpulse│
│ Clasificación voz    │
└──────────┬───────────┘
           │ HTTP
           ▼
┌──────────────────────┐
│ Casa ESP32 WROOM-32 │
│----------------------│
│ Access Point WiFi    │
│ Servidor HTTP        │
│ Control LED PWM      │
│ Control Servo        │
│ Lectura sensores     │
└──────────────────────┘
```

---

# Estructura del repositorio

```text
voice-home-esp32/
├── smartwatch/
│   └── voz.ino
├── casa/
│   └── casa.ino
├── scripts/
│   └── grabar.py
└── README.md
```

---

# Dependencias

## Arduino IDE

Instalar las siguientes librerías:

- `VoiceHome_inferencing` (Edge Impulse)
- `Adafruit SSD1306`
- `Adafruit GFX Library`
- `ArduinoJson`
- `DHT sensor library`
- `ESP32Servo`

## Python

Instalar dependencias para captura y procesamiento de audio:

```bash
pip install pyserial numpy scipy
```

---

# Configuración WiFi

| Parámetro | Valor |
|-----------|--------|
| SSID | `CasaInteligente` |
| Password | `microc2026` |
| IP Gateway | `192.168.4.1` |
| Dashboard | `http://192.168.4.1` |

---

# Modelo de Inteligencia Artificial

Modelo entrenado en :contentReference[oaicite:0]{index=0}

## Características del dataset

- Frecuencia de muestreo: `16000 Hz`
- 120 segundos de audio por clase
- Extracción de características MFCC
- 13 coeficientes por ventana

## Arquitectura del modelo

- Red neuronal convolucional 1D (CNN)
- Inferencia embebida en ESP32-S3
- Clasificación en tiempo real

## Resultados

- Accuracy obtenida: **100%**

---

# Funcionalidades principales

- Reconocimiento de voz offline
- Comunicación HTTP entre ESP32
- Control domótico en tiempo real
- Inferencia local sin internet
- Dashboard accesible vía navegador
- Sistema portable tipo smartwatch

---

# Autores

- Jorge Ortega
- Darwin Díaz

**Universidad del Norte**  
Microcontroladores — 2026
