import paho.mqtt.client as mqtt
import numpy as np
import sounddevice as sd
import soundfile as sf
import threading
import time
import json

# Configuratie
MQTT_BROKER = "192.168.1.100"
MQTT_PORT = 1883
TOPIC_VALUES = "potentiometer/values"
TOPIC_CONTROL = "mqtt/defcon/control"
TOPIC_CH1_CONTROL = "mqtt/defcon/ch1/control"
TOPIC_CH1_STATUS = "mqtt/defcon/ch1/status"
AUDIO_FILE = "audio/bingbingbong.wav"

# Parameters
TARGET_POT1 = 0  # Pas aan naar de gewenste doelwaarde
TARGET_POT2 = 0  # Pas aan naar de gewenste doelwaarde
TOLERANCE_RANGE = 200
REQUIRED_TIME = 3  # Aantal seconden binnen tolerantie voor ruisverdwijning
VOLUME_SCALE = 1.0  # Vaste luidheid voor audio

# Globale variabelen
pot1_value = pot2_value = 0
stop_event = threading.Event()
active_event = threading.Event()
correct_position_start = None  # Tijdstip waarop potentiometers binnen tolerantie kwamen

def mqtt_on_message(client, userdata, message):
    """
    Callback voor MQTT-berichten.
    Verwerkt potentiometerwaarden en controlecommands.
    """
    global pot1_value, pot2_value, stop_event, active_event, correct_position_start
    topic = message.topic
    payload = message.payload.decode('utf-8').strip()

    try:
        if topic == TOPIC_VALUES:
            pot1_value, pot2_value = map(int, payload.split(','))
        elif topic == TOPIC_CONTROL:
            command = json.loads(payload).get("command", "").lower()
            if command == "start":
                stop_event.clear()
                active_event.set()
                correct_position_start = None
                print("[MQTT] Start command ontvangen")
            elif command == "reset":
                stop_event.set()
                active_event.clear()
                correct_position_start = None
                print("[MQTT] Reset command ontvangen - Wacht op start")
        elif topic == TOPIC_CH1_CONTROL:
            command = json.loads(payload).get("control", "").lower()
            print(f"[MQTT] Ontvangen control commando op {TOPIC_CH1_CONTROL}: {command}")
        else:
            print(f"[MQTT] Onbekend topic of command: {topic}")
    except (json.JSONDecodeError, ValueError) as e:
        print(f"[Error] Fout bij het verwerken van MQTT payload: {e}")

def is_within_tolerance():
    """
    Controleer of beide potentiometerwaarden binnen de tolerantie zijn.
    """
    within_pot1 = abs(pot1_value - TARGET_POT1) <= TOLERANCE_RANGE
    within_pot2 = abs(pot2_value - TARGET_POT2) <= TOLERANCE_RANGE
    return within_pot1 and within_pot2

def calculate_noise_level():
    """
    Bereken het ruisniveau op basis van de afstand van de potentiometerwaarden tot hun doelwaarden.
    """
    max_distance = 4095
    distance_pot1 = abs(pot1_value - TARGET_POT1)
    distance_pot2 = abs(pot2_value - TARGET_POT2)

    normalized_distance_pot1 = distance_pot1 / max_distance
    normalized_distance_pot2 = distance_pot2 / max_distance

    average_distance = (normalized_distance_pot1 + normalized_distance_pot2) / 2

    signal_strength = 1 - average_distance
    signal_strength = max(0, min(signal_strength, 1))

    return signal_strength

def publish_status(client, status):
    """
    Publiceer een statusbericht naar `mqtt/defcon/ch1/status`.
    """
    try:
        message = json.dumps({"status": status})
        client.publish(TOPIC_CH1_STATUS, message)
        print(f"[MQTT] Status gepubliceerd: {message}")
    except Exception as e:
        print(f"[Error] Kan status niet publiceren: {e}")

def play_audio(client):
    """
    Speelt audio af en combineert deze met ruis.
    Ruisniveau wordt dynamisch aangepast op basis van potentiometerwaarden.
    """
    global correct_position_start
    try:
        data, fs = sf.read(AUDIO_FILE, dtype='float32')
        if len(data.shape) == 1:
            data = np.column_stack((data, data))

        data *= 2.0  # Verhoog volume met factor 2
        noise = np.random.uniform(-1, 1, data.shape)

        total_frames = len(data)

        def audio_callback(outdata, frames, time_info, status):
            global correct_position_start
            if stop_event.is_set():
                raise sd.CallbackAbort

            frame_start = audio_callback.frame_idx % total_frames
            frame_end = frame_start + frames

            if frame_end <= total_frames:
                audio_chunk = data[frame_start:frame_end]
                noise_chunk = noise[frame_start:frame_end]
            else:
                audio_chunk = np.concatenate((data[frame_start:], data[:frame_end - total_frames]))
                noise_chunk = np.concatenate((noise[frame_start:], noise[:frame_end - total_frames]))

            signal_strength = calculate_noise_level()  # Initialiseer met standaard ruisniveau

            if is_within_tolerance():
                if correct_position_start is None:
                    correct_position_start = time.time()
                elif time.time() - correct_position_start >= REQUIRED_TIME:
                    signal_strength = 1.0
                    # Publiceer slechts één keer de "completed" status
                    if not getattr(audio_callback, 'status_published', False):
                        publish_status(client, "completed")
                        audio_callback.status_published = True
                else:
                    signal_strength = calculate_noise_level()
            else:
                correct_position_start = None
                audio_callback.status_published = False  # Reset publicatie wanneer tolerantie wordt verlaten
                signal_strength = calculate_noise_level()

            combined = VOLUME_SCALE * (signal_strength * audio_chunk + (1 - signal_strength) * noise_chunk)
            outdata[:len(combined)] = np.clip(combined, -1.0, 1.0)

            if len(combined) < frames:
                outdata[len(combined):] = 0

            audio_callback.frame_idx += frames

        audio_callback.frame_idx = 0
        audio_callback.status_published = False

        with sd.OutputStream(callback=audio_callback, samplerate=fs, channels=data.shape[1], blocksize=1024):
            while active_event.is_set() and not stop_event.is_set():
                time.sleep(0.1)
    except FileNotFoundError:
        print(f"[Error] Audiobestand niet gevonden: {AUDIO_FILE}")
    except sd.PortAudioError as e:
        print(f"[Error] Geluidsstream fout: {e}")

def start_mqtt(client):
    """
    Start de MQTT-client en luistert naar berichten.
    """
    try:
        client.on_message = mqtt_on_message
        client.connect(MQTT_BROKER, MQTT_PORT)
        client.subscribe([(TOPIC_VALUES, 0), (TOPIC_CONTROL, 0), (TOPIC_CH1_CONTROL, 0)])
        print("[MQTT] Verbonden met broker")
        client.loop_forever()
    except Exception as e:
        print(f"[Error] Fout bij MQTT verbinding of abonnement: {e}")

def start():
    """
    Start de hoofdthread en initialiseert audio en MQTT.
    Wacht op een startcommand om audio te starten.
    """
    client = mqtt.Client()
    threading.Thread(target=start_mqtt, args=(client,), daemon=True).start()
    while True:
        if active_event.is_set():
            play_audio(client)
        else:
            time.sleep(0.1)

if __name__ == "__main__":
    try:
        start()
    except KeyboardInterrupt:
        print("[Programma] Afsluiten op verzoek")
    except Exception as e:
        print(f"[Error] Onverwachte fout: {e}")
