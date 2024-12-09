import paho.mqtt.client as mqtt
import numpy as np
import sounddevice as sd
import soundfile as sf
import threading
import time

# Configuratie
MQTT_BROKER = "10.3.141.1"  # Pas dit aan naar jouw MQTT-broker IP
TOPIC = "audio/noise_levels"

AUDIO_FILE = 'audio/bingbingbong.wav'

# Doelwaarden voor de potentiometers
TARGET_VALUES = [500, 500]  # Pas deze aan naar de correcte waarden
TOLERANCE = 10  # Toegestane afwijking
CORRECT_POSITION_DURATION = 3  # Tijd in seconden dat de potentiometers correct moeten staan

# Globale variabelen
current_noise_levels = [1.0, 1.0]  # Initiële ruisniveaus
pot_values = [0, 0]  # Huidige potentiometerwaarden
correct_position_start_time = None

# Audiobestand laden
data, fs = sf.read(AUDIO_FILE, dtype='float32')
audio_len = len(data)

# Ruis genereren
def generate_noise(length):
    return np.random.normal(0, 0.1, size=(length, data.shape[1]))

noise1 = generate_noise(audio_len)
noise2 = generate_noise(audio_len)

# Callback functie voor MQTT-berichten
def on_message(client, userdata, msg):
    global current_noise_levels, pot_values, correct_position_start_time
    try:
        message = msg.payload.decode('utf-8')
        values = message.split(',')
        if len(values) == 2:
            pot1, pot2 = int(values[0]), int(values[1])
            pot_values = [pot1, pot2]

            # Ruisniveaus bijwerken op basis van potentiometerwaarden
            current_noise_levels = [1 - (pot1 / 1023), 1 - (pot2 / 1023)]

            # Controleren of potentiometers in de juiste positie staan
            if (abs(pot1 - TARGET_VALUES[0]) <= TOLERANCE and
                abs(pot2 - TARGET_VALUES[1]) <= TOLERANCE):
                if correct_position_start_time is None:
                    correct_position_start_time = time.time()
                elif time.time() - correct_position_start_time >= CORRECT_POSITION_DURATION:
                    # Ruis verwijderen
                    current_noise_levels = [0.0, 0.0]
            else:
                correct_position_start_time = None
        else:
            print("Ongeldig berichtformaat ontvangen")
    except Exception as e:
        print(f"Fout bij het verwerken van het bericht: {e}")

# Audio callback functie
def audio_callback(outdata, frames, time_info, status):
    global current_noise_levels
    start_idx = audio_callback.current_pos
    end_idx = start_idx + frames

    if end_idx <= audio_len:
        audio_chunk = data[start_idx:end_idx]
        noise_chunk1 = noise1[start_idx:end_idx] * current_noise_levels[0]
        noise_chunk2 = noise2[start_idx:end_idx] * current_noise_levels[1]
        noisy_audio = audio_chunk + noise_chunk1 + noise_chunk2
        outdata[:] = noisy_audio
        audio_callback.current_pos += frames
    else:
        # Audio laten loopen
        frames_remaining = audio_len - start_idx
        audio_chunk1 = data[start_idx:audio_len]
        audio_chunk2 = data[0:end_idx - audio_len]
        audio_chunk = np.concatenate((audio_chunk1, audio_chunk2), axis=0)

        noise_chunk1a = noise1[start_idx:audio_len] * current_noise_levels[0]
        noise_chunk1b = noise1[0:end_idx - audio_len] * current_noise_levels[0]
        noise_chunk1 = np.concatenate((noise_chunk1a, noise_chunk1b), axis=0)

        noise_chunk2a = noise2[start_idx:audio_len] * current_noise_levels[1]
        noise_chunk2b = noise2[0:end_idx - audio_len] * current_noise_levels[1]
        noise_chunk2 = np.concatenate((noise_chunk2a, noise_chunk2b), axis=0)

        noisy_audio = audio_chunk + noise_chunk1 + noise_chunk2
        outdata[:] = noisy_audio
        audio_callback.current_pos = end_idx - audio_len

audio_callback.current_pos = 0

# MQTT-client instellen
client = mqtt.Client()
client.on_message = on_message

def mqtt_thread():
    try:
        client.connect(MQTT_BROKER, 1883, 60)
    except Exception as e:
        print(f"Kan niet verbinden met broker: {e}")
        exit(1)
    client.subscribe(TOPIC)
    client.loop_forever()

# Start MQTT in een aparte thread
threading.Thread(target=mqtt_thread).start()

# Start audio stream
try:
    with sd.OutputStream(channels=data.shape[1], samplerate=fs, callback=audio_callback):
        print("Audio afspelen gestart. Druk op Ctrl+C om te stoppen.")
        while True:
            time.sleep(1)
except KeyboardInterrupt:
    print("Programma gestopt door gebruiker")
