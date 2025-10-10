#include <Servo.h>
#include <WiFi.h>
#include <esp_camera.h>

Servo servo;
const int motorPin1 = 2;
const int motorPin2 = 3;

// WiFi credentials
const char *ssid = "your_SSID";
const char *password = "your_PASSWORD";

// Camera configuration
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

void startCameraServer();

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

    // Initialize the camera
    if (esp_camera_init(&config) != ESP_OK)
    {
        Serial.println("Camera init failed");
        return;
    }

    // Start the camera server
    startCameraServer();

    Serial.println("Camera ready! Use 'http://" + WiFi.localIP().toString() + "' to connect");
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

void startCameraServer()
{
    // Placeholder for camera server initialization
    // Typically, you would use an HTTP server to stream images
}