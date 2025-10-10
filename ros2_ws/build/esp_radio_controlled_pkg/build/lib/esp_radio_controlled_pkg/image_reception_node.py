import cv2
import socket
import numpy as np

def main():
    # Set up the socket for receiving images
    host = '0.0.0.0'  # Listen on all available interfaces
    port = 5000       # Port to listen on

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind((host, port))
        print(f"Listening for images on {host}:{port}")

        while True:
            # Receive data from ESP32
            data, addr = sock.recvfrom(65535)  # Maximum UDP packet size
            print(f"Received data from {addr}")

            # Convert the received data to a NumPy array
            np_data = np.frombuffer(data, dtype=np.uint8)

            # Decode the image
            img = cv2.imdecode(np_data, cv2.IMREAD_COLOR)

            if img is not None:
                # Display the image
                cv2.imshow('Received Image', img)

                # Exit on pressing 'q'
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
            else:
                print("Failed to decode image")

    cv2.destroyAllWindows()

if __name__ == '__main__':
    main()
