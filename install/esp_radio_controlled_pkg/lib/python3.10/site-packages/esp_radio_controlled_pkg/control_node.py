import socket
import struct

def main():
    # ESP32のIPアドレスとポート番号
    esp32_host = '192.168.0.102'  # ESP32のIPアドレスに置き換えてください
    esp32_port = 5001             # ESP32でリッスンするポート番号

    # UDPソケットの作成
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        print("Control Node: Use 'w', 's', 'a', 'd' to control, 'q' to quit.")

        while True:
            command = input("Enter command: ")

            if command == 'q':
                print("Exiting control node.")
                break

            if command in ['w', 's', 'a', 'd']:
                # モータとサーボの値を設定
                if command == 'w':
                    motor_value = 100  # 前進
                    servo_angle = 90   # サーボはそのまま
                elif command == 's':
                    motor_value = -100 # 後退
                    servo_angle = 90   # サーボはそのまま
                elif command == 'a':
                    motor_value = 0    # モータ停止
                    servo_angle = 70   # サーボを左に
                elif command == 'd':
                    motor_value = 0    # モータ停止
                    servo_angle = 110  # サーボを右に

                # データをパックして送信
                data = struct.pack('ii', motor_value, servo_angle)
                sock.sendto(data, (esp32_host, esp32_port))
                print(f"Sent command '{command}' with motor={motor_value}, servo={servo_angle}")
            else:
                print("Invalid command. Use 'w', 's', 'a', 'd' or 'q'.")

if __name__ == '__main__':
    main()