#include <ESP32Servo.h> // 修正: ESP32用のライブラリを使用
#include <WiFi.h>
#include <esp_camera.h>

#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM

#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 10
#define SIOD_GPIO_NUM 40
#define SIOC_GPIO_NUM 39

#define Y9_GPIO_NUM 48
#define Y8_GPIO_NUM 11
#define Y7_GPIO_NUM 12
#define Y6_GPIO_NUM 14
#define Y5_GPIO_NUM 16
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 17
#define Y2_GPIO_NUM 15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM 47
#define PCLK_GPIO_NUM 13

Servo servo;
const int motorPin1 = 2;
const int motorPin2 = 3;

// WiFi credentials
const char *ssid = "SkenWi-Fi";
const char *password = "SkenWi-Fi2023";

// PC (ROS) server to send frames to
const char *pc_ip = "192.168.0.101"; // Set to ROS PC IP
const uint16_t pc_port = 5000;

extern void startCameraServer(void);

WiFiClient client;

void setup()
{
    // Initialize servo and motor pins
    servo.attach(1); // Servo connected to pin 1
    pinMode(motorPin1, OUTPUT);
    pinMode(motorPin2, OUTPUT);

    // Start serial communication
    Serial.begin(115200);
    Serial.println("ESP32 Control Ready");

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Debug: Check if PSRAM is enabled
    if (psramFound())
    {
        Serial.println("PSRAM is enabled");
    }
    else
    {
        Serial.println("PSRAM is not enabled. Please enable it in the board settings.");
        Serial.println("Skipping camera initialization due to insufficient memory.");
        return; // PSRAMが有効でない場合、カメラ初期化をスキップ
    }

    // Initialize the camera
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_QVGA; // 解像度をさらに低く設定 (例: QQVGA)
    config.jpeg_quality = 20;           // JPEG品質をさらに低く設定
    config.fb_count = 1;                // フレームバッファ数を1に設定

    // Initialize the camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.println("Camera init failed");
        Serial.printf("Camera init error: %s\n", esp_err_to_name(err));
        return;
    }

    Serial.println("Camera ready! Use 'http://" + WiFi.localIP().toString() + "' to connect");

    // Attempt initial TCP connect to PC (ROS) server
    Serial.printf("Connecting to PC %s:%d...\n", pc_ip, pc_port);
    if (!client.connect(pc_ip, pc_port))
    {
        Serial.println("Initial connection failed, will retry in loop");
    }
    else
    {
        Serial.println("Connected to PC server");
    }
}

void loop()
{
    static unsigned long lastLogTime = 0;
    unsigned long currentTime = millis();

    // Non-blocking serial command handling
    if (Serial.available() > 0)
    {
        char command = Serial.read();

        switch (command)
        {
        case 'w':
            // Move motor forward
            digitalWrite(motorPin1, HIGH);
            digitalWrite(motorPin2, LOW);
            Serial.println("[LOG] Command 'w' received: Moving motor forward");
            break;
        case 's':
            // Move motor backward
            digitalWrite(motorPin1, LOW);
            digitalWrite(motorPin2, HIGH);
            Serial.println("[LOG] Command 's' received: Moving motor backward");
            break;
        case 'a':
            // Turn servo to the left
            servo.write(60);
            Serial.println("[LOG] Command 'a' received: Turning servo left");
            break;
        case 'd':
            // Turn servo to the right
            servo.write(70);
            Serial.println("[LOG] Command 'd' received: Turning servo right");
            break;
        case 'q':
            // Stop motor
            digitalWrite(motorPin1, LOW);
            digitalWrite(motorPin2, LOW);
            Serial.println("[LOG] Command 'q' received: Stopping motor");
            break;
        default:
            Serial.println("[LOG] Unknown command received");
            break;
        }
    }

    // Log communication status every 200ms (5 times per second)
    if (currentTime - lastLogTime >= 200)
    {
        Serial.println("[LOG] Communication active");
        lastLogTime = currentTime;
    }

    // Send camera frames to PC if connected
    if (client.connected())
    {
        // Capture frame
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            Serial.println("Camera capture failed");
            delay(100);
            return;
        }

        // Ensure JPEG format
        if (fb->format != PIXFORMAT_JPEG)
        {
            Serial.println("Frame is not JPEG, skipping");
            esp_camera_fb_return(fb);
            delay(100);
            return;
        }

        // Send 4-byte big-endian length followed by JPEG data
        uint32_t len = fb->len;
        uint8_t header[4];
        header[0] = (len >> 24) & 0xFF;
        header[1] = (len >> 16) & 0xFF;
        header[2] = (len >> 8) & 0xFF;
        header[3] = len & 0xFF;

        bool ok = true;
        if (client.write(header, 4) != 4)
            ok = false;
        if (ok)
        {
            size_t written = 0;
            const uint8_t *ptr = fb->buf;
            while (written < len)
            {
                int chunk = client.write(ptr + written, len - written);
                if (chunk <= 0)
                {
                    ok = false;
                    break;
                }
                written += chunk;
            }
        }

        esp_camera_fb_return(fb);

        if (!ok)
        {
            Serial.println("Send failed, closing client");
            client.stop();
        }
        else
        {
            //Serial.printf("Sent frame %u bytes\n", len);
        }

        // Throttle frame rate
        delay(100); // 10 fps-ish
    }
    else
    {
        // Try to reconnect if not connected
        if (!client.connect(pc_ip, pc_port))
        {
            // Do not spam connection attempts
            delay(500);
        }
        else
        {
            Serial.println("Reconnected to PC server");
        }
    }

    // Small yield to watchdog
    delay(1);
}

// Note: startCameraServer() not used when sending frames via TCP