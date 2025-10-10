import sys
if sys.prefix == '/usr':
    sys.real_prefix = sys.prefix
    sys.prefix = sys.exec_prefix = '/home/altair/Documents/saitou_ws/ros2_ws/install/esp_radio_controlled_pkg'
