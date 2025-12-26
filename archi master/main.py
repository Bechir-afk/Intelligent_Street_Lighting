"""
Smart City - Raspberry Pi Pico Slave
Controls RGB LED lamp and communicates with ESP-01 via UART
WiFi Status LED on GP17: ON=connected, BLINK=connecting, OFF=failed
RGB LED on pins 5,6,7 - Purple when lamp ON, OFF when lamp OFF
Lamp ID: Change LAMP_ID for each slave (1, 2, 3, 4)
"""

from machine import UART, Pin, PWM
import time

# ========== Configuration ==========
LAMP_ID = 1  # *** CHANGE THIS: 1, 2, 3, or 4 ***

# RGB LED Lamp Pins
RGB_R_PIN = 5
RGB_G_PIN = 6
RGB_B_PIN = 7

# WiFi Status LED Pin
WIFI_LED_PIN = 17

# ========== Pin Setup ==========
# WiFi Status LED
wifi_led = Pin(WIFI_LED_PIN, Pin.OUT)
wifi_led.off()

# RGB LED setup with PWM
rgb_r = PWM(Pin(RGB_R_PIN))
rgb_g = PWM(Pin(RGB_G_PIN))
rgb_b = PWM(Pin(RGB_B_PIN))

# Set PWM frequency
rgb_r.freq(1000)
rgb_g.freq(1000)
rgb_b.freq(1000)

# ========== UART to ESP-01 ==========
uart = UART(0, baudrate=9600, tx=Pin(0), rx=Pin(1))

# ========== State Variables ==========
lamp_mode = "AUTO"
lamp_status = "OFF"
last_light = ""
wifi_status = "CONNECTING"  # CONNECTED, CONNECTING, FAILED
last_blink_time = 0
blink_state = False

# ========== RGB Control Functions ==========
def set_rgb(r, g, b):
    """Set RGB LED color (0-255)"""
    rgb_r.duty_u16(int(r * 257))
    rgb_g.duty_u16(int(g * 257))
    rgb_b.duty_u16(int(b * 257))

def lamp_on():
    """Turn lamp ON - Purple"""
    set_rgb(255, 0, 255)

def lamp_off():
    """Turn lamp OFF"""
    set_rgb(0, 0, 0)

# ========== WiFi LED Control ==========
def update_wifi_led():
    """Update WiFi status LED based on current WiFi status"""
    global last_blink_time, blink_state
    
    if wifi_status == "CONNECTED":
        # Solid ON
        wifi_led.on()
    elif wifi_status == "FAILED":
        # OFF
        wifi_led.off()
    elif wifi_status == "CONNECTING":
        # Blink every 250ms
        current_time = time.ticks_ms()
        if time.ticks_diff(current_time, last_blink_time) >= 250:
            last_blink_time = current_time
            blink_state = not blink_state
            if blink_state:
                wifi_led.on()
            else:
                wifi_led.off()

# ========== Helper Functions ==========
def send_status():
    """Send lamp status to ESP-01"""
    msg = f"PUB:{lamp_status}\n"
    uart.write(msg.encode())
    print(f"Sent: {msg.strip()}")

def control_lamp():
    """Control lamp based on mode"""
    global lamp_status
    
    if lamp_mode == "AUTO":
        if last_light == "Dark":
            if lamp_status != "ON":
                lamp_status = "ON"
                lamp_on()
                print("AUTO: Lamp ON (Dark)")
                send_status()
        elif last_light == "Light":
            if lamp_status != "OFF":
                lamp_status = "OFF"
                lamp_off()
                print("AUTO: Lamp OFF (Light)")
                send_status()

def process_command(cmd):
    """Process command from ESP-01"""
    global lamp_mode, lamp_status, last_light, wifi_status
    
    if cmd.startswith("CMD:"):
        command = cmd[4:].strip()
        
        if command == "AUTO":
            lamp_mode = "AUTO"
            print("Mode: AUTO")
            send_status()
            
        elif command == "1":
            lamp_mode = "MANUAL"
            lamp_status = "ON"
            lamp_on()
            print("Mode: MANUAL - ON")
            send_status()
            
        elif command == "0":
            lamp_mode = "MANUAL"
            lamp_status = "OFF"
            lamp_off()
            print("Mode: MANUAL - OFF")
            send_status()
    
    elif cmd.startswith("LIGHT:"):
        last_light = cmd[6:].strip()
        print(f"Light: {last_light}")
    
    elif cmd.startswith("WIFI:"):
        # Update WiFi status from ESP-01
        new_status = cmd[5:].strip()
        if new_status in ["CONNECTED", "CONNECTING", "FAILED", "RETRYING"]:
            if new_status == "RETRYING":
                wifi_status = "CONNECTING"
            else:
                wifi_status = new_status
            print(f"WiFi Status: {wifi_status}")
    
    elif cmd.startswith("MQTT:"):
        print(f"MQTT Status: {cmd[5:]}")

# ========== Main Setup ==========
print(f"\n=== Smart City Pico Slave - Lamp {LAMP_ID} ===")
print("RGB LED Lamp on pins: R=GP5, G=GP6, B=GP7")
print("WiFi Status LED on pin: GP17")
print("Starting...")

lamp_off()
wifi_led.off()

print("Waiting for ESP-01...")

buffer = ""

# ========== Main Loop ==========
while True:
    # Update WiFi status LED
    update_wifi_led()
    
    # Read from ESP-01
    if uart.any():
        data = uart.read()
        if data:
            buffer += data.decode('utf-8', 'ignore')
            
            # Process complete lines
            while '\n' in buffer:
                line, buffer = buffer.split('\n', 1)
                line = line.strip()
                if line:
                    print(f"Received: {line}")
                    process_command(line)
    
    # Control lamp based on current state
    control_lamp()
    
    time.sleep(0.01)  # 10ms loop
