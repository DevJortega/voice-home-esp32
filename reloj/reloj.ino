#define EIDSP_QUANTIZE_FILTERBANK   0

#include <VoiceHome_inferencing_inferencing.h>//libreria de edge 
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* SSID     = "CasaInteligente";
const char* PASSWORD = "microc2026";
const char* CASA_IP  = "http://192.168.4.1";

#define BOOT_BTN 2
#define I2S_SCK  4
#define I2S_WS   7
#define I2S_SD   3
#define OLED_SDA 10
#define OLED_SCL 11
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

typedef struct {
    int16_t *buffer;
    uint8_t buf_ready;
    uint32_t buf_count;
    uint32_t n_samples;
} inference_t;

static inference_t inference;
static const uint32_t sample_buffer_size = 2048;
static signed short sampleBuffer[sample_buffer_size];
static bool debug_nn = false;
static bool record_status = true;

void mostrarTexto(const char* linea1, const char* linea2 = "") {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.println(linea1);
    if (strlen(linea2) > 0) {
        display.setTextSize(1);
        display.setCursor(0, 50);
        display.println(linea2);
    }
    display.display();
}

void enviarComando(const char* comando) {
    if (WiFi.status() != WL_CONNECTED) {
        mostrarTexto("Sin WiFi", "");
        return;
    }

    HTTPClient http;
    String url = String(CASA_IP) + "/" + String(comando);
    http.begin(url);
    int code = http.GET();

    if (code == 200) {
        String payload = http.getString();
        StaticJsonDocument<200> doc;
        deserializeJson(doc, payload);

        if (strcmp(comando, "clima") == 0) {
            float temp = doc["temperatura"];
            float hum  = doc["humedad"];
            char l1[20], l2[20];
            sprintf(l1, "%.1f C", temp);
            sprintf(l2, "Hum: %.0f%%", hum);
            mostrarTexto(l1, l2);
            delay(3000);

        } else if (strcmp(comando, "foco") == 0) {
            bool estado = doc["led"];
            mostrarTexto("FOCO", estado ? "Encendido" : "Apagado");
            delay(2000);

        } else if (strcmp(comando, "puerta") == 0) {
            bool estado = doc["ventana"];
            mostrarTexto("PUERTA", estado ? "Abierta" : "Cerrada");
            delay(2000);
        }
    } else {
        mostrarTexto("Error", "Sin respuesta");
        delay(1500);
    }

    http.end();
    mostrarTexto("Listo", "Presiona boton");
}

void setup() {
    delay(500);
    Serial.begin(115200);
    while (!Serial && millis() < 3000) delay(10);
    delay(500);
    pinMode(BOOT_BTN, INPUT_PULLUP);

    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("Error OLED");
    }
    display.clearDisplay();
    display.display();
    mostrarTexto("Iniciando", "...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);
    Serial.print("Conectando WiFi");
    mostrarTexto("WiFi", "Conectando...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(300);
        Serial.print(".");
    }
    Serial.printf("\nConectado canal %d\n", WiFi.channel());
    mostrarTexto("WiFi OK", "");
    delay(500);

    ei_printf("Frecuencia: %d Hz\n", EI_CLASSIFIER_FREQUENCY);
    ei_sleep(2000);

    if (microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT) == false) {
        ei_printf("ERR: No se pudo allocar buffer\r\n");
        mostrarTexto("ERROR", "Buffer audio");
        return;
    }

    mostrarTexto("Listo", "Presiona boton");
    Serial.println("Listo.");
}

void loop() {
    if (digitalRead(BOOT_BTN) == HIGH) {
        delay(10);
        return;
    }

    mostrarTexto("Escuchando", "Habla ahora...");
    Serial.println("Grabando...");
    ei_sleep(50);

    bool m = microphone_inference_record();
    if (!m) {
        mostrarTexto("ERROR", "Fallo audio");
        return;
    }

    mostrarTexto("Procesando", "...");

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK) {
        mostrarTexto("ERROR", "Clasificador");
        return;
    }

    float max_val = 0;
    const char* max_label = "";
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        if (result.classification[ix].value > max_val) {
            max_val = result.classification[ix].value;
            max_label = result.classification[ix].label;
        }
    }

    ei_printf("---\n");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("  %s: %.2f\n", result.classification[ix].label, result.classification[ix].value);
    }

    if (max_val > 0.7 && strcmp(max_label, "ruido") != 0) {
        ei_printf(">>> DETECTADO: %s (%.0f%%)\n", max_label, max_val * 100);
        enviarComando(max_label);
    } else {
        mostrarTexto("?", "No detectado");
        delay(1500);
        mostrarTexto("Listo", "Presiona boton");
    }

    while (digitalRead(BOOT_BTN) == LOW) delay(10);
}

static void audio_inference_callback(uint32_t n_bytes) {
    for(int i = 0; i < n_bytes>>1; i++) {
        inference.buffer[inference.buf_count++] = sampleBuffer[i];
        if(inference.buf_count >= inference.n_samples) {
            inference.buf_count = 0;
            inference.buf_ready = 1;
        }
    }
}

static void capture_samples(void* arg) {
    const int32_t i2s_bytes_to_read = (uint32_t)arg;
    size_t bytes_read = i2s_bytes_to_read;
    while (record_status) {
        i2s_read((i2s_port_t)1, (void*)sampleBuffer, i2s_bytes_to_read, &bytes_read, 100);
        if (bytes_read <= 0) {
            ei_printf("Error I2S read: %d", bytes_read);
        } else {
            for (int x = 0; x < i2s_bytes_to_read/2; x++) {
                sampleBuffer[x] = (int16_t)(sampleBuffer[x]) * 8;
            }
            if (record_status) audio_inference_callback(i2s_bytes_to_read);
            else break;
        }
    }
    vTaskDelete(NULL);
}

static bool microphone_inference_start(uint32_t n_samples) {
    inference.buffer = (int16_t *)malloc(n_samples * sizeof(int16_t));
    if(inference.buffer == NULL) return false;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;
    if (i2s_init(EI_CLASSIFIER_FREQUENCY)) ei_printf("Failed to start I2S!");
    ei_sleep(100);
    record_status = true;
    xTaskCreate(capture_samples, "CaptureSamples", 1024 * 32, (void*)sample_buffer_size, 10, NULL);
    return true;
}

static bool microphone_inference_record(void) {
    inference.buf_count = 0;
    inference.buf_ready = 0;
    while (inference.buf_ready == 0) delay(10);
    inference.buf_ready = 0;
    return true;
}

static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);
    return 0;
}

static void microphone_inference_end(void) {
    i2s_deinit();
    ei_free(inference.buffer);
}

static int i2s_init(uint32_t sampling_rate) {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = sampling_rate,
        .bits_per_sample = (i2s_bits_per_sample_t)16,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0,
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num   = I2S_SCK,
        .ws_io_num    = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = I2S_SD,
    };
    esp_err_t ret = 0;
    ret = i2s_driver_install((i2s_port_t)1, &i2s_config, 0, NULL);
    if (ret != ESP_OK) ei_printf("Error i2s_driver_install");
    ret = i2s_set_pin((i2s_port_t)1, &pin_config);
    if (ret != ESP_OK) ei_printf("Error i2s_set_pin");
    ret = i2s_zero_dma_buffer((i2s_port_t)1);
    if (ret != ESP_OK) ei_printf("Error i2s_zero_dma_buffer");
    return int(ret);
}

static int i2s_deinit(void) {
    i2s_driver_uninstall((i2s_port_t)1);
    return 0;
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif