import threading
import socket
import struct
import cv2
import numpy as np
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
from std_msgs.msg import String

# Parameters
TCP_PORT = 5000
TCP_HOST = '0.0.0.0'  # bind on all interfaces

class TcpImageReceiver(Node):
    def __init__(self):
        super().__init__('image_reception_node')
        self.pub = self.create_publisher(Image, 'camera/image_raw', 10)
        self.bridge = CvBridge()
        self._server_sock = None
        self._thread = threading.Thread(target=self._server_thread, daemon=True)
        self._stop_event = threading.Event()
        # connection management for sending control commands back to ESP
        self._conn_lock = threading.Lock()
        self._current_conn = None
        # subscriber to receive control commands from GUI/control node
        self._control_sub = self.create_subscription(String, 'esp/control', self._on_control_cmd, 10)

        self.get_logger().info(f'Starting TCP image server on {TCP_HOST}:{TCP_PORT}')
        self._thread.start()

    def _recv_all(self, conn, length):
        data = bytearray()
        while len(data) < length:
            packet = conn.recv(length - len(data))
            if not packet:
                return None
            data.extend(packet)
        return bytes(data)

    def _handle_client(self, conn, addr):
        self.get_logger().info(f'Client connected: {addr}')
        # store current connection so other callbacks can send commands
        with self._conn_lock:
            self._current_conn = conn
        try:
            while not self._stop_event.is_set():
                # Read 4-byte big-endian length
                header = self._recv_all(conn, 4)
                if header is None:
                    self.get_logger().info('Client disconnected')
                    break
                frame_len = struct.unpack('>I', header)[0]
                if frame_len == 0 or frame_len > 10 * 1024 * 1024:
                    self.get_logger().warn(f'Invalid frame length: {frame_len}, closing connection')
                    break

                jpg = self._recv_all(conn, frame_len)
                if jpg is None:
                    self.get_logger().info('Client disconnected during frame receive')
                    break

                # Decode JPEG
                np_arr = np.frombuffer(jpg, np.uint8)
                img = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
                if img is None:
                    self.get_logger().warn('Failed to decode JPEG frame')
                    continue

                # Publish ROS Image
                try:
                    ros_img = self.bridge.cv2_to_imgmsg(img, encoding='bgr8')
                    ros_img.header.stamp = self.get_clock().now().to_msg()
                    self.pub.publish(ros_img)
                except Exception as e:
                    self.get_logger().error(f'Failed to publish image: {e}')

        finally:
            # clear stored connection on exit
            with self._conn_lock:
                try:
                    if self._current_conn is conn:
                        self._current_conn = None
                except Exception:
                    pass
            try:
                conn.close()
            except Exception:
                pass

    def _server_thread(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            s.bind((TCP_HOST, TCP_PORT))
            s.listen(1)
            self._server_sock = s
            while not self._stop_event.is_set():
                try:
                    s.settimeout(1.0)
                    conn, addr = s.accept()
                    # Handle client in same thread to simplify ordering; could spawn per-client thread
                    self._handle_client(conn, addr)
                except socket.timeout:
                    continue
                except Exception as e:
                    self.get_logger().error(f'Server error: {e}')
                    break

    def _on_control_cmd(self, msg: String):
        """ROS subscription callback: forward incoming control string to ESP over TCP."""
        data = msg.data.encode('utf-8')
        with self._conn_lock:
            conn = self._current_conn
        if conn is None:
            self.get_logger().warn('No ESP client connected; cannot forward control command')
            return
        try:
            # send as raw bytes; ESP expects single-byte commands like 'a','d','w','s'
            conn.sendall(data)
            self.get_logger().info(f'Forwarded control to ESP: {msg.data}')
        except Exception as e:
            self.get_logger().error(f'Failed to send control to ESP: {e}')

    def destroy_node(self):
        self.get_logger().info('Shutting down TCP image receiver')
        self._stop_event.set()
        # Close server socket to unblock accept
        try:
            if self._server_sock:
                self._server_sock.close()
        except Exception:
            pass
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = TcpImageReceiver()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
