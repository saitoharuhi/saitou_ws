# PyQt5 GUI for displaying /camera/image_raw and sending commands to ESP
# Dependencies: python3-pyqt5, python3-opencv, cv_bridge, rclpy
# Install example (Ubuntu):
# sudo apt update
# sudo apt install python3-pyqt5 python3-opencv
# sudo apt install ros-${ROS_DISTRO}-cv-bridge

import os
# 日本語コメント: Qt プラグインの競合対策。OpenCV の組み込み Qt プラグインではなく
# システムの Qt プラグインを優先するためにパスを設定します。必要に応じて実環境の
# プラグインパスに書き換えてください。
os.environ.setdefault('QT_QPA_PLATFORM', 'xcb')
os.environ.setdefault('QT_QPA_PLATFORM_PLUGIN_PATH', '/usr/lib/qt5/plugins')

import sys
import threading
import rclpy
from rclpy.node import Node
from std_msgs.msg import String
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
import cv2

from PyQt5.QtWidgets import QApplication, QLabel, QPushButton, QVBoxLayout, QHBoxLayout, QWidget, QMainWindow
from PyQt5.QtGui import QImage, QPixmap
from PyQt5.QtCore import pyqtSignal, QObject


class ImageSignal(QObject):
    image_ready = pyqtSignal(QImage)


class RosBridgeNode(Node):
    def __init__(self, image_signal: ImageSignal):
        super().__init__('qt_control_node')
        self._bridge = CvBridge()
        self._image_signal = image_signal
        self._pub = self.create_publisher(String, 'esp/control', 10)
        self.create_subscription(Image, 'camera/image_raw', self._image_cb, 10)

    def _image_cb(self, msg: Image):
        try:
            cv_img = self._bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')
            rgb = cv2.cvtColor(cv_img, cv2.COLOR_BGR2RGB)
            h, w, ch = rgb.shape
            bytes_per_line = ch * w
            qimg = QImage(rgb.data, w, h, bytes_per_line, QImage.Format_RGB888).copy()
            self._image_signal.image_ready.emit(qimg)
        except Exception as e:
            self.get_logger().warn(f'Failed to convert/publish image to GUI: {e}')

    def send_command(self, cmd: str):
        msg = String()
        msg.data = cmd
        self._pub.publish(msg)
        self.get_logger().info(f'Published control: {cmd}')


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('ESP32 Camera Control GUI')
        self._image_label = QLabel('No image')
        self._image_label.setScaledContents(True)

        # Buttons
        btn_forward = QPushButton('Forward (w)')
        btn_back = QPushButton('Back (s)')
        btn_left = QPushButton('Left (a)')
        btn_right = QPushButton('Right (d)')
        btn_stop = QPushButton('Stop (q)')

        btn_forward.clicked.connect(lambda: self._on_command('w'))
        btn_back.clicked.connect(lambda: self._on_command('s'))
        btn_left.clicked.connect(lambda: self._on_command('a'))
        btn_right.clicked.connect(lambda: self._on_command('d'))
        btn_stop.clicked.connect(lambda: self._on_command('q'))

        hbox = QHBoxLayout()
        hbox.addWidget(btn_left)
        hbox.addWidget(btn_forward)
        hbox.addWidget(btn_back)
        hbox.addWidget(btn_right)
        hbox.addWidget(btn_stop)

        vbox = QVBoxLayout()
        vbox.addWidget(self._image_label)
        vbox.addLayout(hbox)

        central = QWidget()
        central.setLayout(vbox)
        self.setCentralWidget(central)

        # ROS related
        self._image_signal = ImageSignal()
        self._image_signal.image_ready.connect(self._update_image)
        self._ros_node = None
        self._ros_thread = None

    def start_ros(self):
        # rclpy の初期化と Node の生成はメインスレッドで行い、spin を別スレッドで回す
        # 日本語コメント: QObject の moveToThread エラー回避のため Node をメインスレッドで作成する
        try:
            rclpy.init()
        except Exception:
            # すでに初期化済みの場合がある
            pass

        # Node をメインスレッドで生成する（Qt のオブジェクト移動問題回避）
        self._ros_node = RosBridgeNode(self._image_signal)

        # spin をバックグラウンドで実行
        def spin_thread():
            try:
                rclpy.spin(self._ros_node)
            except Exception:
                pass

        self._ros_thread = threading.Thread(target=spin_thread, daemon=True)
        self._ros_thread.start()

    def _update_image(self, qimg: QImage):
        pix = QPixmap.fromImage(qimg)
        self._image_label.setPixmap(pix)

    def _on_command(self, cmd: str):
        # Try to publish via ROS node if available
        if self._ros_node is not None:
            try:
                self._ros_node.send_command(cmd)
            except Exception as e:
                print('Failed to send command via ros node:', e)
        else:
            print('ROS node not ready, command not sent')

    def closeEvent(self, event):
        # Shutdown ROS cleanly
        try:
            if self._ros_node is not None:
                self._ros_node.get_logger().info('Shutting down from GUI')
                # trigger shutdown by destroying node and shutting down rclpy in thread
                try:
                    self._ros_node.destroy_node()
                except Exception:
                    pass
        except Exception:
            pass
        event.accept()


def main(argv=sys.argv):
    app = QApplication(argv)
    window = MainWindow()
    window.resize(640, 480)
    window.show()
    window.start_ros()
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()
