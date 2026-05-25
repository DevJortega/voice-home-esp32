import serial
import numpy as np
import wave
import struct
from scipy.signal import butter, sosfilt

PORT = "COM11"
BAUD = 921600
SAMPLE_RATE = 16000
DURACION = 120  # 2 minutos = ~120 muestras
GANANCIA = 8
NOMBRE = "foco"  # cambia por: foco, puerta, clima, ruido

def make_filters(rate):
    hp = butter(4, 80/(rate/2), btype='high', output='sos')
    lp = butter(4, 3800/(rate/2), btype='low', output='sos')
    return hp, lp

hp, lp = make_filters(SAMPLE_RATE)
ser = serial.Serial(PORT, BAUD)
ser.reset_input_buffer()

input(f"\nPresiona ENTER para grabar '{NOMBRE}.wav' ({DURACION} seg)...")
print("Grabando! Di la palabra al ritmo del metronomo...")

total_bytes = SAMPLE_RATE * DURACION * 2
raw = ser.read(total_bytes)

print("Procesando...")

audio = np.frombuffer(raw, dtype=np.int16).astype(np.float32)
audio = sosfilt(hp, audio)
audio = sosfilt(lp, audio)
audio = audio * GANANCIA
audio = np.clip(audio, -32768, 32767).astype(np.int16)

with wave.open(f"{NOMBRE}.wav", 'w') as wf:
    wf.setnchannels(1)
    wf.setsampwidth(2)
    wf.setframerate(SAMPLE_RATE)
    wf.writeframes(struct.pack('<' + 'h' * len(audio), *audio))

print(f"Guardado: {NOMBRE}.wav")
ser.close()