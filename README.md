```markdown
# Voice Home ESP32 🎙️🏠

Sistema de control domótico activado por comandos de voz, usando dos ESP32 que se comunican mediante HTTP sobre WiFi.

## Descripción

El sistema consta de dos nodos:

- **Smartwatch (ESP32-S3 Super Mini):** Captura audio con micrófono INMP441, clasifica la palabra con un modelo de IA embebido (Edge Impulse) y envía el comando a la casa via HTTP.
- **Casa Inteligente (ESP32 WROOM-32):** Actúa como Access Point WiFi y servidor HTTP. Controla un LED PWM y un servo motor, y lee sensores de temperatura, humedad y luminosidad.

## Palabras clave reconocidas

| Palabra | Acción |
|---------|--------|
| `foco` | Toggle LED iluminación |
| `puerta` | Toggle servo motor (garaje) |
| `clima` | Consultar temperatura y humedad |
| `ruido` | Clase negativa (sin acción) |

## Hardware

### Smartwatch — ESP32-S3 Super Mini
| Componente | GPIO |
|------------|------|
| INMP441 SCK | GPIO 4 |
| INMP441 WS | GPIO 7 |
| INMP441 SD | GPIO 3 |
| OLED SDA | GPIO 10 |
| OLED SCL | GPIO 11 |
| Botón | GPIO 2 |

### Casa — ESP32 WROOM-32
| Componente | GPIO |
|------------|------|
| DHT22 | GPIO 33 |
| LDR | GPIO 34 |
| Servo | GPIO 18 |
| LED PWM | GPIO 19 |

## Estructura del repositorio

```
voice-home-esp32/
├── smartwatch/
│   └── voz.ino
├── casa/
│   └── casa.ino
├── scripts/
│   └── grabar.py
└── README.md
```

## Dependencias

### Arduino
- `VoiceHome_inferencing` (Edge Impulse)
- `Adafruit SSD1306`
- `Adafruit GFX Library`
- `ArduinoJson`
- `DHT sensor library`
- `ESP32Servo`

### Python
```bash
pip install pyserial numpy scipy
```

## Configuración WiFi

- **SSID:** `CasaInteligente`
- **Password:** `microc2026`
- **IP:** `192.168.4.1`
- **Dashboard:** `http://192.168.4.1`

## Modelo de IA

Entrenado en [Edge Impulse](https://edgeimpulse.com):
- 120 segundos por clase a 16000 Hz
- Características MFCC (13 coeficientes)
- Red neuronal convolucional 1D
- **Accuracy: 100%**

## Autores

- Jorge Ortega
- Darwin Díaz

Universidad del Norte — Microcontroladores 2026
```
