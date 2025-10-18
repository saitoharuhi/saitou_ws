import socket

def main():
    # Set up the socket for sending commands
    esp32_host = '10.50.100.217'  # Replace with the ESP32's IP address
    esp32_port = 5001           # Port to send commands to

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        print("Control Node: Use 'w', 's', 'a', 'd' to control, 'q' to quit.")

        while True:
            command = input("Enter command: ")

            if command == 'q':
                print("Exiting control node.")
                break

            if command in ['w', 's', 'a', 'd']:
                # Send the command to ESP32
                sock.sendto(command.encode(), (esp32_host, esp32_port))
                print(f"Sent command '{command}' to ESP32")
            else:
                print("Invalid command. Use 'w', 's', 'a', 'd' or 'q'.")

if __name__ == '__main__':
    main()