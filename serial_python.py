import serial
import time

# シリアルポートを正しく設定
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
time.sleep(2)  # 接続安定待ち

for angle in [0, 45, 90, 135, 180]:
    cmd = f"angle:{angle}\n"
    ser.write(cmd.encode())
    resp = ser.readline().decode().strip()
    print(f"Sent {angle}, ESP32→ {resp}")
    time.sleep(2)

ser.close()

