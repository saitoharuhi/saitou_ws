#include <ESP32Servo.h> // 修正: ESP32用のライブラリを使用
#include <WiFi.h>
#include <esp_camera.h>

#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM


#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  40
#define SIOC_GPIO_NUM  39

#define Y9_GPIO_NUM    48
#define Y8_GPIO_NUM    11
#define Y7_GPIO_NUM    12
#define Y6_GPIO_NUM    14
#define Y5_GPIO_NUM    16
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    17
#define Y2_GPIO_NUM    15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  47
#define PCLK_GPIO_NUM  13

Servo servo;
const int motorPin1 = 2;
const int motorPin2 = 3;

// WiFi credentials
const char *ssid = "SSID-9188DF";
const char *password = "3d67747b";

extern void startCameraServer(void);


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
    config.jpeg_quality = 10;           // JPEG品質をさらに低く設定
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
    startCameraServer();  
}

void loop()
{
    if (Serial.available() > 0)
    {
        char command = Serial.read();

        switch (command)
        {
        case 'w':
            // Move motor forward
            digitalWrite(motorPin1, HIGH);
            digitalWrite(motorPin2, LOW);
            break;
        case 's':
            // Move motor backward
            digitalWrite(motorPin1, LOW);
            digitalWrite(motorPin2, HIGH);
            break;
        case 'a':
            // Turn servo to the left
            servo.write(45);
            break;
        case 'd':
            // Turn servo to the right
            servo.write(135);
            break;
        case 'q':
            // Stop motor
            digitalWrite(motorPin1, LOW);
            digitalWrite(motorPin2, LOW);
            break;
        default:
            Serial.println("Unknown command");
            break;
        }
    }
    // Add any additional control logic here
}

// void startCameraServer() 関数の定義を削除
// 関数呼び出しはそのまま残します
