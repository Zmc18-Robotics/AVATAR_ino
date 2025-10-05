#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// Konfigurasi WiFi
const char* ssid = "PUT YOUR WIFI NAME HERE / TARUH NAMA WIFI ANDA DISINI";
const char* password = "PUT YOUR WIFI PASSWORD HERE / TARUH SANDI WIFI ANDA DISINI";

// Pin Definitions
#define TRIG_PIN 19
#define ECHO_PIN 21
#define BUZZER_PIN 18
#define SERVO_PIN 2

// MAX7219 Pin Definitions - SISI KIRI ESP32
#define MAX_CLK_PIN   25  // CLK
#define MAX_DATA_PIN  26  // DIN
#define MAX_CS_PIN    27  // CS

// MAX7219 Configuration - 4 Modules, Rotasi 90 derajat
#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW
#define MAX_DEVICES 4

// Pin RGB LED
#define LED_R 13
#define LED_G 12
#define LED_B 14

// Inisialisasi dengan Software SPI
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, MAX_DATA_PIN, MAX_CLK_PIN, MAX_CS_PIN, MAX_DEVICES);

// Servo
Servo myServo;

// Web Server
WebServer server(80);

// Global Variables
bool swingMode = false;
bool sensorTriggered = false;
bool displayActive = false;
bool customTextActive = false;
unsigned long lastSwingTime = 0;
unsigned long sensorTriggerTime = 0;
unsigned long lastSensorCheck = 0;
unsigned long displayStartTime = 0;
int servoAngle = 0;
bool servoDirection = true;
long lastDistance = -1;
int swingDelay = 20;
bool pixelEditorActive = false;
uint8_t pixelMatrix[32][8]; // 32 columns x 8 rows (4 modules x 8 cols each)

// RGB Control Variables
bool rgbActive = false;
int rgbMode = 0; // 0=off, 1=static, 2=blink, 3=random, 4=fade, 5=police
unsigned long lastRgbUpdate = 0;
int rgbBlinkState = 0;
int rgbFadeValue = 0;
int rgbFadeDirection = 1;
int rgbPoliceCount = 0;
int currentR = 0, currentG = 0, currentB = 0;

// Character arrays for cycling
const char CHAR_SET[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const int CHAR_SET_SIZE = 37;

// Current character indices for each column (0 = space)
int charIndices[4] = {0, 0, 0, 0}; // Starting with spaces

const int SENSOR_MIN_DISTANCE_CM = 60;
const int SENSOR_MAX_DISTANCE_CM = 90;
const int SENSOR_DISPLAY_DURATION = 3000;
const int SENSOR_CHECK_INTERVAL = 200;
const int SERVO_STEP = 3;
const int DISPLAY_BRIGHTNESS = 7;

// Font 5x7 untuk rotasi 90 derajat ke KANAN (clockwise)
const uint8_t H[8] = {0x00, 0x7F, 0x08, 0x08, 0x08, 0x08, 0x7F, 0x00};
const uint8_t A[8] = {0x00, 0x7E, 0x09, 0x09, 0x09, 0x09, 0x7E, 0x00};
const uint8_t L[8] = {0x00, 0x7F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00};
const uint8_t O[8] = {0x00, 0x3E, 0x41, 0x41, 0x41, 0x41, 0x3E, 0x00};
const uint8_t T[8] = {0x00, 0x01, 0x01, 0x01, 0x7F, 0x01, 0x01, 0x01};
const uint8_t E[8] = {0x00, 0x7F, 0x49, 0x49, 0x49, 0x49, 0x41, 0x00};
const uint8_t S[8] = {0x00, 0x26, 0x49, 0x49, 0x49, 0x49, 0x32, 0x00};
const uint8_t I[8] = {0x00, 0x00, 0x41, 0x7F, 0x41, 0x00, 0x00, 0x00};
const uint8_t P[8] = {0x00, 0x7F, 0x09, 0x09, 0x09, 0x09, 0x06, 0x00};
const uint8_t DIGIT_8[8] = {0x00, 0x36, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00};
const uint8_t F[8] = {0x00, 0x7F, 0x09, 0x09, 0x09, 0x09, 0x01, 0x00};
const uint8_t B[8] = {0x00, 0x7F, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00};
const uint8_t C[8] = {0x00, 0x3E, 0x41, 0x41, 0x41, 0x41, 0x22, 0x00};
const uint8_t D[8] = {0x00, 0x7F, 0x41, 0x41, 0x41, 0x22, 0x1C, 0x00};
const uint8_t G[8] = {0x00, 0x3E, 0x41, 0x41, 0x49, 0x49, 0x7A, 0x00};
const uint8_t J[8] = {0x00, 0x20, 0x40, 0x40, 0x40, 0x3F, 0x00, 0x00};
const uint8_t K[8] = {0x00, 0x7F, 0x08, 0x08, 0x14, 0x22, 0x41, 0x00};
const uint8_t M[8] = {0x00, 0x7F, 0x02, 0x04, 0x02, 0x7F, 0x00, 0x00};
const uint8_t N[8] = {0x00, 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00, 0x00};
const uint8_t Q[8] = {0x00, 0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00, 0x00};
const uint8_t R[8] = {0x00, 0x7F, 0x09, 0x19, 0x29, 0x46, 0x00, 0x00};
const uint8_t U[8] = {0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00, 0x00};
const uint8_t V[8] = {0x00, 0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00, 0x00};
const uint8_t W[8] = {0x00, 0x3F, 0x40, 0x38, 0x40, 0x3F, 0x00, 0x00};
const uint8_t X[8] = {0x00, 0x63, 0x14, 0x08, 0x14, 0x63, 0x00, 0x00};
const uint8_t Y[8] = {0x00, 0x07, 0x08, 0x70, 0x08, 0x07, 0x00, 0x00};
const uint8_t Z[8] = {0x00, 0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x00};
const uint8_t DIGIT_0[8] = {0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x00};
const uint8_t DIGIT_1[8] = {0x00, 0x00, 0x42, 0x7F, 0x40, 0x00, 0x00, 0x00};
const uint8_t DIGIT_2[8] = {0x00, 0x62, 0x51, 0x49, 0x49, 0x46, 0x00, 0x00};
const uint8_t DIGIT_3[8] = {0x00, 0x22, 0x41, 0x49, 0x49, 0x36, 0x00, 0x00};
const uint8_t DIGIT_4[8] = {0x00, 0x18, 0x14, 0x12, 0x7F, 0x10, 0x00, 0x00};
const uint8_t DIGIT_5[8] = {0x00, 0x27, 0x45, 0x45, 0x45, 0x39, 0x00, 0x00};
const uint8_t DIGIT_6[8] = {0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00, 0x00};
const uint8_t DIGIT_7[8] = {0x00, 0x01, 0x71, 0x09, 0x05, 0x03, 0x00, 0x00};
const uint8_t DIGIT_9[8] = {0x00, 0x06, 0x49, 0x49, 0x29, 0x1E, 0x00, 0x00};
const uint8_t BLANK[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// HTML Web Page
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 IoT Control</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 1200px; /* Diperlebar dari 800px */
            margin: 50px auto;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
        }
        .container {
            background: white;
            padding: 30px;
            border-radius: 15px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 30px;
        }
        .control-group {
            margin: 20px 0;
            padding: 20px;
            background: #f5f5f5;
            border-radius: 10px;
        }
        .control-group h2 {
            font-size: 18px;
            color: #555;
            margin-bottom: 15px;
        }
        button {
            width: 100%;
            padding: 15px;
            margin: 10px 0;
            font-size: 16px;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            transition: all 0.3s;
            font-weight: bold;
        }
        .btn-primary {
            background: #667eea;
            color: white;
        }
        .btn-primary:hover {
            background: #5568d3;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102,126,234,0.4);
        }
        .btn-success {
            background: #48bb78;
            color: white;
        }
        .btn-success:hover {
            background: #38a169;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(72,187,120,0.4);
        }
        .btn-danger {
            background: #f56565;
            color: white;
        }
        .btn-danger:hover {
            background: #e53e3e;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(245,101,101,0.4);
        }
        .btn-info {
            background: #4299e1;
            color: white;
        }
        .btn-info:hover {
            background: #3182ce;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(66,153,225,0.4);
        }
        .btn-warning {
            background: #ed8936;
            color: white;
        }
        .btn-warning:hover {
            background: #dd6b20;
            transform: translateY(-2px);
        }
        .btn-purple {
            background: #9f7aea;
            color: white;
        }
        .btn-purple:hover {
            background: #805ad5;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(159,122,234,0.4);
        }
        .btn-cyan {
            background: #0891b2;
            color: white;
        }
        .btn-cyan:hover {
            background: #0e7490;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(8,145,178,0.4);
        }
        .status {
            padding: 10px;
            margin: 10px 0;
            border-radius: 5px;
            text-align: center;
            font-weight: bold;
        }
        .status-on {
            background: #c6f6d5;
            color: #22543d;
        }
        .status-off {
            background: #fed7d7;
            color: #742a2a;
        }
        .status-detected {
            background: #fef08a;
            color: #713f12;
            animation: pulse 1s infinite;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.7; }
        }
        .info {
            background: #bee3f8;
            padding: 15px;
            border-radius: 8px;
            margin: 20px 0;
            color: #2c5282;
        }
        .angle-display {
            background: #e6fffa;
            padding: 10px;
            border-radius: 5px;
            text-align: center;
            margin: 10px 0;
            font-size: 18px;
            color: #234e52;
        }
        .speed-control {
            margin: 15px 0;
            padding: 15px;
            background: #fff3cd;
            border-radius: 8px;
        }
        .speed-control label {
            display: block;
            margin-bottom: 10px;
            color: #333;
            font-weight: bold;
            font-size: 16px;
        }
        .speed-buttons {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 10px;
        }
        .btn-speed {
            padding: 12px;
            font-size: 14px;
            background: #ffc107;
            color: #000;
            font-weight: bold;
        }
        .btn-speed:hover {
            background: #ffb300;
            transform: translateY(-2px);
        }
        .speed-value {
            text-align: center;
            margin: 10px 0;
            font-size: 16px;
            color: #333;
            font-weight: bold;
        }
        .display-info {
            background: #d1fae5;
            padding: 15px;
            border-radius: 8px;
            margin: 10px 0;
            color: #065f46;
        }
        .test-buttons {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
        }
        .custom-text-container {
            background: #fef3c7;
            padding: 20px;
            border-radius: 10px;
            margin: 15px 0;
        }
        .custom-text-preview {
            background: #fbbf24;
            padding: 15px;
            border-radius: 8px;
            text-align: center;
            font-size: 32px;
            font-weight: bold;
            color: #78350f;
            margin: 10px 0 20px 0;
            letter-spacing: 12px;
            font-family: monospace;
        }
        .char-controls {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 15px;
            margin: 20px 0;
        }
        .char-control {
            background: white;
            border-radius: 8px;
            padding: 15px;
            text-align: center;
            border: 2px solid #d97706;
        }
        .char-control h3 {
            margin: 0 0 10px 0;
            color: #92400e;
            font-size: 14px;
        }
        .char-display {
            font-size: 36px;
            font-weight: bold;
            color: #78350f;
            margin: 10px 0;
            height: 50px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-family: monospace;
        }
        .arrow-buttons {
            display: flex;
            flex-direction: column;
            gap: 8px;
        }
        .btn-arrow {
            padding: 10px;
            font-size: 18px;
            background: #d97706;
            color: white;
            border: none;
            border-radius: 6px;
            cursor: pointer;
            font-weight: bold;
            transition: all 0.2s;
        }
        .btn-arrow:hover {
            background: #b45309;
            transform: scale(1.05);
        }
        .btn-arrow:active {
            transform: scale(0.95);
        }
        .custom-text-buttons {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            margin-top: 15px;
        }
        
        /* Pixel Editor Styles */
        .pixel-editor-container {
            background: #f0f9ff;
            padding: 20px;
            border-radius: 10px;
            margin: 15px 0;
        }
        .pixel-canvas-wrapper {
            background: #1e293b;
            padding: 20px;
            border-radius: 10px;
            margin: 15px 0;
            display: flex;
            justify-content: center;
            align-items: center;
            overflow-x: auto; /* Allow horizontal scroll jika perlu */
        }
        .pixel-canvas {
            display: inline-grid;
            grid-template-columns: repeat(32, 20px);
            grid-template-rows: repeat(8, 20px);
            gap: 2px;
            background: #0f172a;
            padding: 10px;
            border-radius: 8px;
            border: 3px solid #334155;
        }
        .pixel {
            width: 20px;
            height: 20px;
            background: #1e293b;
            border: 1px solid #334155;
            cursor: pointer;
            transition: all 0.1s;
            border-radius: 2px;
        }
        .pixel:hover {
            border-color: #06b6d4;
            transform: scale(1.1);
        }
        .pixel.active {
            background: #22d3ee;
            box-shadow: 0 0 10px #22d3ee;
            border-color: #06b6d4;
        }
        .pixel-info {
            background: #dbeafe;
            padding: 15px;
            border-radius: 8px;
            margin: 10px 0;
            text-align: center;
            color: #1e40af;
            font-size: 14px;
        }
        .pixel-buttons {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 10px;
            margin-top: 15px;
        }
        .btn-pixel {
            padding: 12px;
            font-size: 14px;
        }
        .pixel-status {
            background: #e0f2fe;
            padding: 12px;
            border-radius: 8px;
            text-align: center;
            margin: 10px 0;
            color: #075985;
            font-weight: bold;
        }
        .preset-buttons {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 8px;
            margin: 15px 0;
        }
        .btn-preset {
            padding: 10px;
            font-size: 12px;
            background: #0891b2;
            color: white;
            border: none;
            border-radius: 6px;
            cursor: pointer;
            font-weight: bold;
            transition: all 0.2s;
        }
        .btn-preset:hover {
            background: #0e7490;
            transform: translateY(-2px);
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎛️ ESP32 IoT Control</h1>
        
        <div class="info">
            <strong>Status Sistem:</strong> Online ✅<br>
            <small>MAX7219: 4 Modules (32x8 pixels) | PIN: CLK=25, DIN=26, CS=27</small>
        </div>

        <!-- PIXEL EDITOR SECTION -->
        <div class="control-group">
            <h2>🎨 Pixel Art Editor</h2>
            <div class="pixel-editor-container">
                <div class="pixel-status" id="pixelStatus">Editor: Ready</div>
                
                <div class="pixel-info">
                    <strong>32 x 8 Pixel Canvas (Rotated 90°)</strong><br>
                    Klik pixel untuk menggambar | Canvas lebih lebar dari display fisik
                </div>
                
                <div class="pixel-canvas-wrapper">
                    <div class="pixel-canvas" id="pixelCanvas">
                        <!-- Pixels will be generated by JavaScript -->
                    </div>
                </div>
                
                <div class="preset-buttons">
                    <button class="btn-preset" onclick="loadPreset('smile')">😊 Smile</button>
                    <button class="btn-preset" onclick="loadPreset('heart')">❤️ Heart</button>
                    <button class="btn-preset" onclick="loadPreset('arrow')">➡️ Arrow</button>
                    <button class="btn-preset" onclick="loadPreset('wave')">🌊 Wave</button>
                </div>
                
                <div class="pixel-buttons">
                    <button class="btn-success btn-pixel" onclick="showPixelArt()">📺 Tampilkan</button>
                    <button class="btn-warning btn-pixel" onclick="clearPixelArt()">⏹️ Stop</button>
                    <button class="btn-danger btn-pixel" onclick="resetPixelEditor()">🔄 Reset</button>
                </div>
                
                <div style="margin-top: 10px; font-size: 12px; color: #075985; text-align: center;">
                    Klik pixel untuk menggambar | Preset patterns tersedia di atas
                </div>
            </div>
        </div>

        <!-- RGB LED CONTROL SECTION -->
        <div class="control-group">
            <h2>💡 RGB LED Control</h2>
            
            <div id="rgbStatus" class="status status-off">RGB: OFF</div>
            
            <div style="background: #1e293b; padding: 20px; border-radius: 10px; margin: 15px 0;">
                <div id="rgbPreview" style="background: rgb(0,0,0); width: 100%; height: 80px; border-radius: 8px; border: 3px solid #334155; box-shadow: inset 0 0 30px rgba(255,255,255,0.1); transition: all 0.3s ease;"></div>
                <div style="text-align: center; color: #cbd5e1; margin-top: 10px; font-size: 14px;">
                    R: <span id="rgbRValue">0</span> | G: <span id="rgbGValue">0</span> | B: <span id="rgbBValue">0</span>
                </div>
            </div>
            
            <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin: 15px 0;">
                <button class="btn-success" onclick="rgbOn()">💡 ON (White)</button>
                <button class="btn-danger" onclick="rgbOff()">⚫ OFF</button>
            </div>
            
            <div style="margin: 15px 0; padding: 15px; background: #f0f9ff; border-radius: 8px;">
                <h3 style="margin: 0 0 10px 0; color: #0c4a6e; font-size: 16px;">🎨 Pilih Warna Preset</h3>
                <div style="display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px;">
                    <button class="btn-danger" style="padding: 12px; font-size: 13px;" onclick="setColor(255,0,0)">🔴 Merah</button>
                    <button class="btn-success" style="padding: 12px; font-size: 13px;" onclick="setColor(0,255,0)">🟢 Hijau</button>
                    <button class="btn-info" style="padding: 12px; font-size: 13px;" onclick="setColor(0,0,255)">🔵 Biru</button>
                    <button class="btn-warning" style="padding: 12px; font-size: 13px;" onclick="setColor(255,255,0)">🟡 Kuning</button>
                    <button class="btn-cyan" style="padding: 12px; font-size: 13px;" onclick="setColor(0,255,255)">💠 Cyan</button>
                    <button class="btn-purple" style="padding: 12px; font-size: 13px;" onclick="setColor(255,0,255)">🟣 Magenta</button>
                    <button class="btn-primary" style="padding: 12px; font-size: 13px;" onclick="setColor(255,255,255)">⚪ Putih</button>
                    <button class="btn-warning" style="padding: 12px; font-size: 13px; background: #f97316;" onclick="setColor(255,128,0)">🟠 Orange</button>
                    <button class="btn-purple" style="padding: 12px; font-size: 13px; background: #7c3aed;" onclick="setColor(128,0,128)">💜 Ungu</button>
                </div>
            </div>
            
            <div style="margin: 15px 0; padding: 15px; background: #fef3c7; border-radius: 8px;">
                <h3 style="margin: 0 0 10px 0; color: #78350f; font-size: 16px;">✨ Mode Efek Animasi</h3>
                <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 10px;">
                    <button class="btn-warning" onclick="rgbBlink()">⚡ Blink</button>
                    <button class="btn-purple" onclick="rgbRandom()">🎲 Random</button>
                    <button class="btn-info" onclick="rgbFade()">🌊 Fade</button>
                    <button class="btn-danger" onclick="rgbPolice()">🚨 Polisi</button>
                </div>
                <div style="margin-top: 10px; font-size: 12px; color: #78350f; text-align: center;">
                    Blink: Kedip normal | Random: Warna acak | Fade: Redup-terang | Polisi: Merah-biru
                </div>
            </div>
            
            <div id="rgbModeInfo" style="background: #dbeafe; padding: 12px; border-radius: 8px; text-align: center; color: #1e40af; font-weight: bold; margin-top: 10px;">
                Mode: OFF
            </div>
        </div>

        <!-- CUSTOM TEXT SECTION -->
        <div class="control-group">
            <h2>✏️ Custom Text Display</h2>
            <div class="custom-text-container">
                <div class="custom-text-preview" id="textPreview">- - - -</div>
                
                <div class="char-controls">
                    <div class="char-control">
                        <h3>Channel 1</h3>
                        <div class="char-display" id="char1">-</div>
                        <div class="arrow-buttons">
                            <button class="btn-arrow" onclick="changeChar(0, 1)">▲</button>
                            <button class="btn-arrow" onclick="changeChar(0, -1)">▼</button>
                        </div>
                    </div>
                    
                    <div class="char-control">
                        <h3>Channel 2</h3>
                        <div class="char-display" id="char2">-</div>
                        <div class="arrow-buttons">
                            <button class="btn-arrow" onclick="changeChar(1, 1)">▲</button>
                            <button class="btn-arrow" onclick="changeChar(1, -1)">▼</button>
                        </div>
                    </div>
                    
                    <div class="char-control">
                        <h3>Channel 3</h3>
                        <div class="char-display" id="char3">-</div>
                        <div class="arrow-buttons">
                            <button class="btn-arrow" onclick="changeChar(2, 1)">▲</button>
                            <button class="btn-arrow" onclick="changeChar(2, -1)">▼</button>
                        </div>
                    </div>
                    
                    <div class="char-control">
                        <h3>Channel 4</h3>
                        <div class="char-display" id="char4">-</div>
                        <div class="arrow-buttons">
                            <button class="btn-arrow" onclick="changeChar(3, 1)">▲</button>
                            <button class="btn-arrow" onclick="changeChar(3, -1)">▼</button>
                        </div>
                    </div>
                </div>
                
                <div class="custom-text-buttons">
                    <button class="btn-purple" onclick="showCustomText()">📺 Tampilkan</button>
                    <button class="btn-danger" onclick="stopCustomText()">⏹️ Stop</button>
                </div>
                <div style="margin-top: 10px; font-size: 12px; color: #92400e; text-align: center;">
                    Gunakan tombol ▲▼ untuk mengubah karakter
                </div>
            </div>
        </div>

        <!-- SERVO CONTROL SECTION -->
        <div class="control-group">
            <h2>🔄 Servo Control</h2>
            <div id="swingStatus" class="status status-off">Swing Mode: OFF</div>
            <div id="angleDisplay" class="angle-display">Angle: 0°</div>
            <button class="btn-success" onclick="toggleSwing()">Toggle Swing Mode</button>
            <button class="btn-primary" onclick="testServo()">Test Servo (90°)</button>
            
            <div class="speed-control">
                <label>⚡ Kecepatan Rotasi:</label>
                <div class="speed-value">Delay: <span id="currentSpeed">20</span> ms</div>
                <div class="speed-buttons">
                    <button class="btn-speed" onclick="setSpeed(5)">Sangat Cepat<br>5 ms</button>
                    <button class="btn-speed" onclick="setSpeed(15)">Cepat<br>15 ms</button>
                    <button class="btn-speed" onclick="setSpeed(20)">Normal<br>20 ms</button>
                    <button class="btn-speed" onclick="setSpeed(30)">Sedang<br>30 ms</button>
                    <button class="btn-speed" onclick="setSpeed(50)">Lambat<br>50 ms</button>
                    <button class="btn-speed" onclick="setSpeed(80)">Sangat Lambat<br>80 ms</button>
                </div>
            </div>
        </div>

        <!-- SENSOR & DISPLAY SECTION -->
        <div class="control-group">
            <h2>📊 Sensor & Display</h2>
            <div id="sensorInfo" class="info">Distance: -- cm</div>
            <div id="displayStatus" class="display-info">Display: Standby</div>
            <div class="test-buttons">
                <button class="btn-info" onclick="testDisplay()">Test "HALO"</button>
                <button class="btn-warning" onclick="testDisplay2()">Test "TEST"</button>
            </div>
            <button class="btn-info" onclick="clearDisplay()">Clear Display</button>
        </div>

        <!-- SYSTEM CONTROL SECTION -->
        <div class="control-group">
            <h2>🔧 System Control</h2>
            <button class="btn-danger" onclick="stopAll()">Stop All</button>
        </div>
    </div>

    <script>
        let currentChars = [' ', ' ', ' ', ' '];
        const charset = ' ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
        let pixelData = [];
        let currentRgbR = 0, currentRgbG = 0, currentRgbB = 0;

        function rgbOn() {
            fetch('/rgb-on')
                .then(response => response.text())
                .then(data => {
                    updateStatus();
                })
                .catch(err => console.log('Error:', err));
        }

        function rgbOff() {
            fetch('/rgb-off')
                .then(response => response.text())
                .then(data => {
                    updateStatus();
                })
                .catch(err => console.log('Error:', err));
        }

        function setColor(r, g, b) {
            fetch('/rgb-color?r=' + r + '&g=' + g + '&b=' + b)
                .then(response => response.text())
                .then(data => {
                    updateStatus();
                })
                .catch(err => console.log('Error:', err));
        }

        function rgbBlink() {
            fetch('/rgb-blink')
                .then(response => response.text())
                .then(data => {
                    updateStatus();
                })
                .catch(err => console.log('Error:', err));
        }

        function rgbRandom() {
            fetch('/rgb-random')
                .then(response => response.text())
                .then(data => {
                    updateStatus();
                })
                .catch(err => console.log('Error:', err));
        }

        function rgbFade() {
            // Use current color or default to white
            let r = currentRgbR || 255;
            let g = currentRgbG || 255;
            let b = currentRgbB || 255;
            //By Zmc18_Robotics Zminecrafter @mc.zminecrafter_18 @Zmc18_Roboticz
            fetch('/rgb-fade?r=' + r + '&g=' + g + '&b=' + b)
                .then(response => response.text())
                .then(data => {
                    updateStatus();
                })
                .catch(err => console.log('Error:', err));
        }

        function rgbPolice() {
            fetch('/rgb-police')
                .then(response => response.text())
                .then(data => {
                    updateStatus();
                })
                .catch(err => console.log('Error:', err));
        }

        function updateRgbDisplay(data) {
            // Update RGB status
            document.getElementById('rgbStatus').className = 
                'status ' + (data.rgbActive ? 'status-on' : 'status-off');
            document.getElementById('rgbStatus').textContent = 
                'RGB: ' + (data.rgbActive ? 'ON' : 'OFF');
            
            // Update RGB values
            currentRgbR = data.rgbR || 0;
            currentRgbG = data.rgbG || 0;
            currentRgbB = data.rgbB || 0;
            
            // Update preview box
            document.getElementById('rgbPreview').style.background = 
                'rgb(' + currentRgbR + ',' + currentRgbG + ',' + currentRgbB + ')';
            
            // Add glow effect if active
            if(data.rgbActive && (currentRgbR > 0 || currentRgbG > 0 || currentRgbB > 0)) {
                document.getElementById('rgbPreview').style.boxShadow = 
                    'inset 0 0 30px rgba(255,255,255,0.3), 0 0 20px rgba(' + 
                    currentRgbR + ',' + currentRgbG + ',' + currentRgbB + ',0.6)';
            } else {
                document.getElementById('rgbPreview').style.boxShadow = 
                    'inset 0 0 30px rgba(255,255,255,0.1)';
            }
            
            // Update RGB value display
            document.getElementById('rgbRValue').textContent = currentRgbR;
            document.getElementById('rgbGValue').textContent = currentRgbG;
            document.getElementById('rgbBValue').textContent = currentRgbB;
            
            // Update mode info
            let modeText = 'Mode: ';
            let modeNames = ['OFF', 'Static', 'Blink', 'Random', 'Fade', 'Police'];
            modeText += modeNames[data.rgbMode] || 'OFF';
            document.getElementById('rgbModeInfo').textContent = modeText;
        }

        // Initialize pixel canvas
        function initPixelCanvas() {
            const canvas = document.getElementById('pixelCanvas');
            canvas.innerHTML = '';
            
            // Create 8 rows x 32 columns grid (8 rows, 32 columns)
            for(let row = 0; row < 8; row++) {
                for(let col = 0; col < 32; col++) {
                    const pixel = document.createElement('div');
                    pixel.className = 'pixel';
                    pixel.dataset.col = col;
                    pixel.dataset.row = row;
                    pixel.onclick = function() {
                        togglePixel(col, row, this);
                    };
                    canvas.appendChild(pixel);
                }
            }
            
            // Initialize pixel data array
            pixelData = Array(32).fill(0).map(() => Array(8).fill(0));
        }

        function togglePixel(col, row, element) {
            fetch('/toggle-pixel?col=' + col + '&row=' + row)
                .then(response => response.text())
                .then(data => {
                    if(data === '1') {
                        element.classList.add('active');
                        pixelData[col][row] = 1;
                    } else {
                        element.classList.remove('active');
                        pixelData[col][row] = 0;
                    }
                })
                .catch(err => console.log('Error:', err));
        }

        function showPixelArt() {
            fetch('/show-pixel-art')
                .then(response => response.text())
                .then(data => {
                    updateStatus();
                })
                .catch(err => console.log('Error:', err));
        }

        function clearPixelArt() {
            fetch('/clear-pixel-art')
                .then(response => response.text())
                .then(data => {
                    updateStatus();
                })
                .catch(err => console.log('Error:', err));
        }

        function resetPixelEditor() {
            if(confirm('Reset semua pixel? Gambar akan terhapus.')) {
                fetch('/reset-pixel-editor')
                    .then(response => response.text())
                    .then(data => {
                        // Clear visual canvas
                        const pixels = document.querySelectorAll('.pixel');
                        pixels.forEach(p => p.classList.remove('active'));
                        
                        // Clear data array
                        pixelData = Array(32).fill(0).map(() => Array(8).fill(0));
                        
                        updateStatus();
                    })
                    .catch(err => console.log('Error:', err));
            }
        }

        function loadPreset(type) {
            // Clear first
            const pixels = document.querySelectorAll('.pixel');
            pixels.forEach(p => p.classList.remove('active'));
            
            let pattern = [];
            
            if(type === 'smile') {
                // Smiley face disesuaikan dengan rotasi
                pattern = [
                    [10,2], [10,5], [21,2], [21,5], // eyes
                    [8,1], [8,6], [9,0], [9,7], [22,0], [22,7], [23,1], [23,6], // outline
                    [12,0], [13,0], [14,0], [15,0], [16,0], [17,0], [18,0], [19,0] // smile
                ]; //By Zmc18_Robotics Zminecrafter @mc.zminecrafter_18 @Zmc18_Roboticz
            } else if(type === 'heart') {
                // Heart pattern
                pattern = [
                    [10,2], [10,3], [11,1], [11,2], [11,3], [11,4], [12,2], [12,3],
                    [20,2], [20,3], [21,1], [21,2], [21,3], [21,4], [22,2], [22,3],
                    [13,4], [14,5], [15,6], [16,7], [17,6], [18,5], [19,4]
                ];
            } else if(type === 'arrow') {
                // Arrow pointing right
                pattern = [
                    [12,3], [12,4], [13,3], [13,4], [14,3], [14,4], [15,3], [15,4],
                    [16,2], [16,3], [16,4], [16,5], [17,1], [17,2], [17,5], [17,6],
                    [18,0], [18,1], [18,6], [18,7]
                ];
            } else if(type === 'wave') {
                // Wave across all columns
                for(let col = 0; col < 32; col++) {
                    let row = Math.round(4 + 2 * Math.sin(col * Math.PI / 8));
                    pattern.push([col, row]);
                }
            }
            
            // Apply pattern
            pattern.forEach(([col, row]) => {
                if(col < 32 && row < 8) {
                    const pixelIndex = row * 32 + col;
                    togglePixel(col, row, pixels[pixelIndex]);
                }
            });
        }

        function changeChar(channel, direction) {
            fetch('/change-char?ch=' + channel + '&dir=' + direction)
                .then(response => response.text())
                .then(data => {
                    updateStatus();
                })
                .catch(err => console.log('Error:', err));
        }

        function updateCharDisplay(chars) {
            document.getElementById('char1').textContent = chars[0] || '-';
            document.getElementById('char2').textContent = chars[1] || '-';
            document.getElementById('char3').textContent = chars[2] || '-';
            document.getElementById('char4').textContent = chars[3] || '-';
            
            let preview = '';
            for(let i = 0; i < 4; i++) {
                preview += (chars[i] === ' ' ? '-' : chars[i]) + ' ';
            }
            document.getElementById('textPreview').textContent = preview.trim();
        }

        function showCustomText() {
            fetch('/show-custom-text')
                .then(response => response.text())
                .then(data => {
                    updateStatus();
                })
                .catch(err => console.log('Error:', err));
        }

        function stopCustomText() {
            fetch('/stop-custom-text')
                .then(response => response.text())
                .then(data => {
                    updateStatus();
                })
                .catch(err => console.log('Error:', err));
        }

        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    // Servo status
                    document.getElementById('swingStatus').className = 
                        'status ' + (data.swing ? 'status-on' : 'status-off');
                    document.getElementById('swingStatus').textContent = 
                        'Swing Mode: ' + (data.swing ? 'ON' : 'OFF');
                    
                    document.getElementById('angleDisplay').textContent = 
                        'Angle: ' + data.angle + '°';
                    
                    // Sensor status
                    let distanceText = 'Distance: ';
                    if(data.distance > 0) {
                        distanceText += data.distance + ' cm';
                        if(data.distance >= 60 && data.distance <= 90) {
                            distanceText += ' 🎯 (Detection Zone)';
                        }
                    } else {
                        distanceText += 'No object';
                    }
                    document.getElementById('sensorInfo').textContent = distanceText;
                    
                    if(data.distance >= 60 && data.distance <= 90) {
                        document.getElementById('sensorInfo').className = 'status-detected';
                    } else {
                        document.getElementById('sensorInfo').className = 'info';
                    }
                    
                    // Display status
                    let displayText = 'Display: ';
                    if(data.pixelEditorActive) {
                        displayText += '🎨 Pixel Art';
                    } else if(data.customTextActive) {
                        displayText += '✏️ Custom Text';
                    } else if(data.displayActive) {
                        displayText += '🔴 Active';
                    } else {
                        displayText += '⚪ Standby';
                    }
                    document.getElementById('displayStatus').textContent = displayText;
                    
                    // Pixel status
                    let pixelStatusText = 'Editor: ';
                    if(data.pixelEditorActive) {
                        pixelStatusText += '🟢 Displaying on Device';
                    } else {
                        pixelStatusText += 'Ready';
                    }
                    document.getElementById('pixelStatus').textContent = pixelStatusText;
                    
                    // Speed
                    document.getElementById('currentSpeed').textContent = data.speed;
                    
                    // Custom text characters
                    if(data.chars) {
                        updateCharDisplay(data.chars);
                    }
                    
                    // Update pixel canvas from server data
                    if(data.pixels) {
                        const pixels = document.querySelectorAll('.pixel');
                        for(let col = 0; col < 32; col++) {
                            for(let row = 0; row < 8; row++) {
                                const pixelElement = pixels[row * 32 + col];
                                if(data.pixels[col][row] === 1) {
                                    pixelElement.classList.add('active');
                                } else {
                                    pixelElement.classList.remove('active');
                                }
                            }
                        }
                    }
                    
                    // RGB LED status update
                    updateRgbDisplay(data);
                })
                .catch(err => console.log('Error:', err));
        }

        function setSpeed(value) {
            fetch('/speed?val=' + value)
                .then(response => response.text())
                .then(data => {
                    updateStatus();
                })
                .catch(err => console.log('Error:', err));
        }

        function toggleSwing() {
            fetch('/toggle-swing').then(() => updateStatus());
        }

        function testServo() {
            fetch('/test-servo').then(() => updateStatus());
        }

        function testDisplay() {
            fetch('/test-display').then(() => updateStatus());
        }

        function testDisplay2() {
            fetch('/test-display2').then(() => updateStatus());
        }

        function clearDisplay() {
            fetch('/clear-display').then(() => updateStatus());
        }

        function stopAll() {
            fetch('/stop-all').then(() => updateStatus());
        }

        // Initialize on load
        window.onload = function() {
            initPixelCanvas();
            updateStatus();
            setInterval(updateStatus, 1000);
        };
    </script>
</body>
</html>
)rawliteral";

// Function Prototypes
long getDistance();
void handleRoot();
void handleToggleSwing();
void handleTestServo();
void handleTestDisplay();
void handleTestDisplay2();
void handleClearDisplay();
void handleStopAll();
void handleStatus();
void handleSpeed();
void handleChangeChar();
void handleShowCustomText();
void handleStopCustomText();
void updateServo();
void playStartupBuzzer();
void playDetectionTone();
void playCustomTextTone();
void displayText(const char* text);
void displayChar(int module, char ch);
void handleTogglePixel();
void handleShowPixelArt();
void handleClearPixelArt();
void handleResetPixelEditor();
void displayPixelArt();
void initPixelMatrix();
String getCurrentText();

void initPixelMatrix() {
    for(int col = 0; col < 32; col++) {
        for(int row = 0; row < 8; row++) {
            pixelMatrix[col][row] = 0;
        }
    }
    Serial.println("[PIXEL] Matrix initialized (32x8)");
}
//By Zmc18_Robotics Zminecrafter @mc.zminecrafter_18 @Zmc18_Roboticz
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=== ESP32 IoT Control System ===");
    Serial.println("MAX7219 4-Module Rotated 90° Version");
    Serial.println("Button Control Edition with Pixel Editor & RGB LED");
    
    // Initialize pixel matrix
    initPixelMatrix();
    
    // Setup Pins
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    // Setup RGB LED pins
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    setRGB(0, 0, 0);
    Serial.println("[OK] RGB LED initialized (R=13, G=12, B=14)");
    
    // Initialize MAX7219 Display
    Serial.println("[INIT] Initializing MAX7219 Display...");
    Serial.println("Pin Configuration:");
    Serial.println("  CLK (SCK):  GPIO " + String(MAX_CLK_PIN));
    Serial.println("  DIN (MOSI): GPIO " + String(MAX_DATA_PIN));
    Serial.println("  CS:         GPIO " + String(MAX_CS_PIN));
    Serial.println("  Modules:    4 (rotated 90° clockwise)");
    Serial.println("  Modules:    4 x 8x8 = 32x8 total");
    
    // Set pin modes
    pinMode(MAX_CLK_PIN, OUTPUT);
    pinMode(MAX_DATA_PIN, OUTPUT);
    pinMode(MAX_CS_PIN, OUTPUT);
    
    // Initialize display
    mx.begin();
    delay(100);
    
    Serial.println("[INIT] Setting display parameters...");
    mx.control(MD_MAX72XX::INTENSITY, DISPLAY_BRIGHTNESS);
    delay(50);
    
    mx.clear();
    delay(100);
    
    // Test: Nyalakan semua LED untuk test hardware
    Serial.println("[TEST] Testing all LEDs ON...");
    mx.control(MD_MAX72XX::INTENSITY, 15);
    for(int dev = 0; dev < MAX_DEVICES; dev++) {
        for(int col = 0; col < 8; col++) {
            mx.setColumn(dev, col, 0xFF);
        }
    }
    delay(1500);
    
    mx.clear();
    delay(500);
    
    // Test display "8888"
    Serial.println("[TEST] Testing '8888'...");
    mx.control(MD_MAX72XX::INTENSITY, DISPLAY_BRIGHTNESS);
    displayText("8888");
    delay(2000);
    
    mx.clear();
    delay(500);
    
    Serial.println("[OK] MAX7219 initialized and tested");
    
    // Play startup buzzer sequence
    playStartupBuzzer();
    
    // Test RGB LED - cycle colors
    Serial.println("[TEST] Testing RGB LED...");
    setRGB(255, 0, 0); // Red
    delay(300);
    setRGB(0, 255, 0); // Green
    delay(300);
    setRGB(0, 0, 255); // Blue
    delay(300);
    setRGB(255, 255, 255); // White
    delay(300);
    setRGB(0, 0, 0); // Off
    Serial.println("[OK] RGB LED tested");
    
    // Setup Servo
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    myServo.setPeriodHertz(50);
    myServo.attach(SERVO_PIN, 500, 2400);
    
    // Test servo
    Serial.println("[TEST] Testing servo...");
    myServo.write(0);
    delay(500);
    myServo.write(90);
    delay(500);
    myServo.write(0);
    delay(500);
    Serial.println("[OK] Servo initialized and tested");
    
    // WiFi Connection
    WiFi.disconnect(true);
    delay(1000);
    WiFi.mode(WIFI_STA);
    delay(100);
    
    Serial.println("\n[WiFi] Connecting to: " + String(ssid));
    WiFi.begin(ssid, password);
    //By Zmc18_Robotics Zminecrafter @mc.zminecrafter_18 @Zmc18_Roboticz
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
        
        if (attempts % 5 == 0) {
            Serial.print(" [" + String(attempts) + "/30]");
        }
    }
    
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[OK] WiFi Connected!");
        Serial.println("[INFO] IP Address: " + WiFi.localIP().toString());
        Serial.println("[INFO] Signal Strength: " + String(WiFi.RSSI()) + " dBm");
        
        displayText("HALO");
        delay(2000);
        mx.clear();
        
        // Success RGB flash - Green
        for(int i = 0; i < 3; i++) {
            setRGB(0, 255, 0);
            delay(150);
            setRGB(0, 0, 0);
            delay(100);
        }
        
        for(int i = 0; i < 2; i++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(150);
            digitalWrite(BUZZER_PIN, LOW);
            delay(100);
        }
    } else {
        Serial.println("[ERROR] WiFi Connection Failed!");
        displayText("FAIL");
        delay(2000);
        mx.clear();
        
        // Error RGB flash - Red
        for(int i = 0; i < 5; i++) {
            setRGB(255, 0, 0);
            delay(100);
            setRGB(0, 0, 0);
            delay(100);
        }
        
        for(int i = 0; i < 5; i++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(50);
            digitalWrite(BUZZER_PIN, LOW);
            delay(50);
        }
    }
    
    // Setup Web Server
    server.on("/", handleRoot);
    server.on("/toggle-swing", handleToggleSwing);
    server.on("/test-servo", handleTestServo);
    server.on("/test-display", handleTestDisplay);
    server.on("/test-display2", handleTestDisplay2);
    server.on("/clear-display", handleClearDisplay);
    server.on("/stop-all", handleStopAll);
    server.on("/status", handleStatus);
    server.on("/speed", handleSpeed);
    server.on("/change-char", handleChangeChar);
    server.on("/show-custom-text", handleShowCustomText);
    server.on("/stop-custom-text", handleStopCustomText);
    server.on("/toggle-pixel", handleTogglePixel);
    server.on("/show-pixel-art", handleShowPixelArt);
    server.on("/clear-pixel-art", handleClearPixelArt);
    server.on("/reset-pixel-editor", handleResetPixelEditor);
    
    // RGB LED handlers
    server.on("/rgb-on", handleRgbOn);
    server.on("/rgb-off", handleRgbOff);
    server.on("/rgb-color", handleRgbColor);
    server.on("/rgb-blink", handleRgbBlink);
    server.on("/rgb-random", handleRgbRandom);
    server.on("/rgb-fade", handleRgbFade);
    server.on("/rgb-police", handleRgbPolice);
    
    server.begin();
    Serial.println("[OK] Web server started");
    
    displayText("INIT");
    delay(1500);
    mx.clear();
    
    Serial.println("\n=== System Ready ===");
    Serial.println("Access web interface at: http://" + WiFi.localIP().toString());
    Serial.println("RGB LED: R=13, G=12, B=14");
    Serial.println();
    
    digitalWrite(BUZZER_PIN, HIGH);
    delay(300);
    digitalWrite(BUZZER_PIN, LOW);
}

void loop() {
    server.handleClient();
    
    // Update RGB LED effects
    updateRgbEffects();
    
    // Check Ultrasonic Sensor (only if custom text and pixel editor are not active)
    if (!customTextActive && !pixelEditorActive && millis() - lastSensorCheck >= SENSOR_CHECK_INTERVAL) {
        lastSensorCheck = millis();
        long distance = getDistance();
        lastDistance = distance;
        
        if (distance >= SENSOR_MIN_DISTANCE_CM && distance <= SENSOR_MAX_DISTANCE_CM) {
            if (!sensorTriggered) {
                sensorTriggered = true;
                sensorTriggerTime = millis();
                displayStartTime = millis();
                displayActive = true;
                
                Serial.println("[DETECT] Object at " + String(distance) + " cm - Showing HALO");
                displayText("HALO");
                
                playDetectionTone();
            }
        } else {
            if (sensorTriggered && (millis() - sensorTriggerTime >= SENSOR_DISPLAY_DURATION)) {
                sensorTriggered = false;
                displayActive = false;
                mx.clear();
                Serial.println("[CLEAR] Display cleared");
            }
        }
    }
    //By Zmc18_Robotics Zminecrafter @mc.zminecrafter_18 @Zmc18_Roboticz
    // Update Servo
    updateServo();
}

String getCurrentText() {
    String text = "";
    for(int i = 0; i < 4; i++) {
        text += CHAR_SET[charIndices[i]];
    }
    return text;
}

void displayText(const char* text) {
    mx.clear();
    
    int len = strlen(text);
    if (len > 4) len = 4;
    
    for(int i = 0; i < len; i++) {
        int reversedIndex = (len - 1) - i;
        displayChar(i, text[reversedIndex]);
    }
}

void displayChar(int module, char ch) {
    if (module >= MAX_DEVICES) return;
    
    const uint8_t* charData = NULL;
    
    switch(ch) {
        case 'A': charData = A; break;
        case 'B': charData = B; break;
        case 'C': charData = C; break;
        case 'D': charData = D; break;
        case 'E': charData = E; break;
        case 'F': charData = F; break;
        case 'G': charData = G; break;
        case 'H': charData = H; break;
        case 'I': charData = I; break;
        case 'J': charData = J; break;
        case 'K': charData = K; break;
        case 'L': charData = L; break;
        case 'M': charData = M; break;
        case 'N': charData = N; break;
        case 'O': charData = O; break;
        case 'P': charData = P; break;
        case 'Q': charData = Q; break;
        case 'R': charData = R; break;
        case 'S': charData = S; break;
        case 'T': charData = T; break;
        case 'U': charData = U; break;
        case 'V': charData = V; break;
        case 'W': charData = W; break;
        case 'X': charData = X; break;
        case 'Y': charData = Y; break;
        case 'Z': charData = Z; break;
        case '0': charData = DIGIT_0; break;
        case '1': charData = DIGIT_1; break;
        case '2': charData = DIGIT_2; break;
        case '3': charData = DIGIT_3; break;
        case '4': charData = DIGIT_4; break;
        case '5': charData = DIGIT_5; break;
        case '6': charData = DIGIT_6; break;
        case '7': charData = DIGIT_7; break;
        case '8': charData = DIGIT_8; break;
        case '9': charData = DIGIT_9; break;
        case ' ': charData = BLANK; break;
        default: charData = BLANK; break;
    }
    
    for(uint8_t row = 0; row < 8; row++) {
        mx.setRow(module, row, charData[row]);
    }
}

void playStartupBuzzer() {
    Serial.println("[BUZZER] Playing startup sequence...");
    
    int melody[] = {262, 330, 392, 523};
    int noteDuration = 150;
    
    for(int i = 0; i < 4; i++) {
        int period = 1000000 / melody[i];
        int halfPeriod = period / 2;
        
        unsigned long startTime = millis();
        while(millis() - startTime < noteDuration) {
            digitalWrite(BUZZER_PIN, HIGH);
            delayMicroseconds(halfPeriod);
            digitalWrite(BUZZER_PIN, LOW);
            delayMicroseconds(halfPeriod);
        }
        
        delay(50);
    }
    
    Serial.println("[OK] Startup buzzer sequence completed");
}

void playDetectionTone() {
    int melody[] = {523, 659, 784};
    int noteDuration = 100;
    
    for(int i = 0; i < 3; i++) {
        int period = 1000000 / melody[i];
        int halfPeriod = period / 2;
        
        unsigned long startTime = millis();
        while(millis() - startTime < noteDuration) {
            digitalWrite(BUZZER_PIN, HIGH);
            delayMicroseconds(halfPeriod);
            digitalWrite(BUZZER_PIN, LOW);
            delayMicroseconds(halfPeriod);
        }
        
        delay(30);
    }
}

void playCustomTextTone() {
    // Special melody for custom text
    int melody[] = {659, 784, 880, 1047, 880, 784, 659};
    int noteDuration = 120;
    
    for(int i = 0; i < 7; i++) {
        int period = 1000000 / melody[i];
        int halfPeriod = period / 2;
        
        unsigned long startTime = millis();
        while(millis() - startTime < noteDuration) {
            digitalWrite(BUZZER_PIN, HIGH);
            delayMicroseconds(halfPeriod);
            digitalWrite(BUZZER_PIN, LOW);
            delayMicroseconds(halfPeriod);
        }
        
        delay(40);
    }
}

long getDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration == 0) return -1;
    
    long distance = duration * 0.034 / 2;
    return distance;
}

void updateServo() {
    if (!swingMode) return;
    
    if (millis() - lastSwingTime >= swingDelay) {
        lastSwingTime = millis();
        
        if (servoDirection) {
            servoAngle += SERVO_STEP;
            if (servoAngle >= 90) {
                servoAngle = 90;
                servoDirection = false;
            }
        } else {
            servoAngle -= SERVO_STEP;
            if (servoAngle <= 0) {
                servoAngle = 0;
                servoDirection = true;
            }
        }
        
        myServo.write(servoAngle);
    }
}

void handleRoot() {
    server.send(200, "text/html", webpage);
}

void handleToggleSwing() {
    swingMode = !swingMode;
    if (!swingMode) {
        myServo.write(0);
        servoAngle = 0;
        servoDirection = true;
        Serial.println("[CONTROL] Swing mode STOPPED");
    } else {
        lastSwingTime = millis();
        servoAngle = 0;
        servoDirection = true;
        myServo.write(0);
        Serial.println("[CONTROL] Swing mode STARTED (Speed: " + String(swingDelay) + " ms)");
    }
    server.send(200, "text/plain", "OK");
}

void handleTestServo() {
    Serial.println("[TEST] Testing servo 90°");
    myServo.write(90);
    servoAngle = 90;
    delay(100);
    server.send(200, "text/plain", "OK");
}
//By Zmc18_Robotics Zminecrafter @mc.zminecrafter_18 @Zmc18_Roboticz
void handleTestDisplay() {
    Serial.println("[TEST] Testing display - HALO");
    displayActive = true;
    displayStartTime = millis();
    sensorTriggerTime = millis();
    customTextActive = false;
    
    displayText("HALO");
    
    playDetectionTone();
    server.send(200, "text/plain", "OK");
}

void handleTestDisplay2() {
    Serial.println("[TEST] Testing display - TEST");
    displayActive = true;
    displayStartTime = millis();
    sensorTriggerTime = millis();
    customTextActive = false;
    
    displayText("TEST");
    
    playDetectionTone();
    server.send(200, "text/plain", "OK");
}

void handleChangeChar() {
    if (server.hasArg("ch") && server.hasArg("dir")) {
        int channel = server.arg("ch").toInt();
        int direction = server.arg("dir").toInt();
        
        if (channel >= 0 && channel < 4) {
            charIndices[channel] += direction;
            
            // Wrap around
            if (charIndices[channel] < 0) {
                charIndices[channel] = CHAR_SET_SIZE - 1;
            } else if (charIndices[channel] >= CHAR_SET_SIZE) {
                charIndices[channel] = 0;
            }
            
            Serial.println("[CHAR] Channel " + String(channel) + " changed to: " + String(CHAR_SET[charIndices[channel]]));
            
            // Play quick beep
            digitalWrite(BUZZER_PIN, HIGH);
            delay(30);
            digitalWrite(BUZZER_PIN, LOW);
            
            server.send(200, "text/plain", "OK");
            return;
        }
    }
    server.send(400, "text/plain", "Invalid parameters");
}

void handleShowCustomText() {
    String text = getCurrentText();
    
    customTextActive = true;
    displayActive = false;
    sensorTriggered = false;
    
    Serial.println("[CUSTOM] Displaying custom text: '" + text + "'");
    displayText(text.c_str());
    
    playCustomTextTone();
    
    server.send(200, "text/plain", "OK");
}

void handleStopCustomText() {
    Serial.println("[CUSTOM] Stopping custom text display");
    customTextActive = false;
    mx.clear();
    server.send(200, "text/plain", "OK");
}

void handleClearDisplay() {
    Serial.println("[CONTROL] Clearing display");
    displayActive = false;
    sensorTriggered = false;
    customTextActive = false;
    mx.clear();
    server.send(200, "text/plain", "OK");
}

void handleStopAll() {
    swingMode = false;
    myServo.write(0);
    servoAngle = 0;
    servoDirection = true;
    displayActive = false;
    sensorTriggered = false;
    customTextActive = false;
    pixelEditorActive = false;
    rgbActive = false;
    rgbMode = 0;
    mx.clear();
    setRGB(0, 0, 0);
    
    Serial.println("[CONTROL] All systems STOPPED");
    server.send(200, "text/plain", "OK");
}

void handleSpeed() {
    if (server.hasArg("val")) {
        int newSpeed = server.arg("val").toInt();
        if (newSpeed >= 5 && newSpeed <= 100) {
            swingDelay = newSpeed;
            Serial.println("[SPEED] Changed to: " + String(swingDelay) + " ms");
        }
    }
    server.send(200, "text/plain", String(swingDelay));
}

void handleStatus() {
    String json = "{";
    json += "\"swing\":" + String(swingMode ? "true" : "false") + ",";
    json += "\"angle\":" + String(servoAngle) + ",";
    json += "\"distance\":" + String(lastDistance) + ",";
    json += "\"speed\":" + String(swingDelay) + ",";
    json += "\"displayActive\":" + String(displayActive ? "true" : "false") + ",";
    json += "\"customTextActive\":" + String(customTextActive ? "true" : "false") + ",";
    json += "\"pixelEditorActive\":" + String(pixelEditorActive ? "true" : "false") + ",";
    
    // RGB LED status
    json += "\"rgbActive\":" + String(rgbActive ? "true" : "false") + ",";
    json += "\"rgbMode\":" + String(rgbMode) + ",";
    json += "\"rgbR\":" + String(currentR) + ",";
    json += "\"rgbG\":" + String(currentG) + ","; //By Zmc18_Robotics Zminecrafter @mc.zminecrafter_18 @Zmc18_Roboticz
    json += "\"rgbB\":" + String(currentB) + ",";
    
    json += "\"chars\":[";
    for(int i = 0; i < 4; i++) {
        json += "\"" + String(CHAR_SET[charIndices[i]]) + "\"";
        if(i < 3) json += ",";
    }
    json += "],";
    
    // Add pixel matrix data
    json += "\"pixels\":[";
    for(int col = 0; col < 32; col++) {
        json += "[";
        for(int row = 0; row < 8; row++) {
            json += String(pixelMatrix[col][row]);
            if(row < 7) json += ",";
        }
        json += "]";
        if(col < 31) json += ",";
    }
    json += "]";
    
    json += "}";
    
    server.send(200, "application/json", json);
}

void displayPixelArt() {
    mx.clear();
    
    // Display pixel art dengan ROTASI 90° ke KANAN
    // Canvas web: 32 kolom (horizontal) x 8 row (vertical)
    // Display fisik: Dirotasi 90° clockwise - sama seperti displayHalo()
    
    // PENTING: Module fisik tersambung terbalik (module 0 di kanan)
    // Web canvas: ABCD (kiri ke kanan)
    //By Zmc18_Robotics Zminecrafter @mc.zminecrafter_18 @Zmc18_Roboticz
    // Module fisik: 3-2-1-0 (kiri ke kanan)
    // Jadi kita perlu balik urutan module
    
    for(int module = 0; module < MAX_DEVICES; module++) {
        // Balik urutan module: 0->3, 1->2, 2->1, 3->0
        int reversedModule = (MAX_DEVICES - 1) - module;
        
        for(int row = 0; row < 8; row++) {
            // Ambil column dari pixel matrix
            int webCol = (reversedModule * 8) + row;
            
            // Build byte dari 8 pixels di column tersebut
            uint8_t columnData = 0;
            for(int webRow = 0; webRow < 8; webRow++) {
                if(pixelMatrix[webCol][webRow] == 1) {
                    columnData |= (1 << webRow);
                }
            }
            
            // Set row dengan data column
            mx.setRow(module, row, columnData);
        }
    }
    
    Serial.println("[PIXEL] Pixel art displayed (rotated 90° clockwise)");
}

void handleTogglePixel() {
    if (server.hasArg("col") && server.hasArg("row")) {
        int col = server.arg("col").toInt();
        int row = server.arg("row").toInt();
        
        if (col >= 0 && col < 32 && row >= 0 && row < 8) {
            // Toggle pixel
            pixelMatrix[col][row] = (pixelMatrix[col][row] == 0) ? 1 : 0;
            
            Serial.println("[PIXEL] Toggle pixel [" + String(col) + "," + String(row) + "] = " + String(pixelMatrix[col][row]));
            
            // Play quick beep
            digitalWrite(BUZZER_PIN, HIGH);
            delay(20);
            digitalWrite(BUZZER_PIN, LOW);
            
            server.send(200, "text/plain", String(pixelMatrix[col][row]));
            return;
        }
    }
    server.send(400, "text/plain", "Invalid parameters");
}

void handleShowPixelArt() {
    Serial.println("[PIXEL] Showing pixel art");
    
    pixelEditorActive = true;
    customTextActive = false;
    displayActive = false;
    sensorTriggered = false;
    
    displayPixelArt();
    
    // Play success tone
    for(int i = 0; i < 3; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(50);
        digitalWrite(BUZZER_PIN, LOW);
        delay(50);
    }
    
    server.send(200, "text/plain", "OK");
}

void handleClearPixelArt() {
    Serial.println("[PIXEL] Clearing pixel art display");
    
    pixelEditorActive = false;
    mx.clear();
    
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    
    server.send(200, "text/plain", "OK");
}

void handleResetPixelEditor() {
    Serial.println("[PIXEL] Resetting pixel editor");
    
    initPixelMatrix();
    pixelEditorActive = false;
    mx.clear();
    
    // Play reset tone
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
    
    server.send(200, "text/plain", "OK");
}

void setRGB(int r, int g, int b) {
    analogWrite(LED_R, r);
    analogWrite(LED_G, g);
    analogWrite(LED_B, b);
    currentR = r;
    currentG = g;
    currentB = b;
}

void updateRgbEffects() {
    if (!rgbActive) return;
    
    unsigned long now = millis(); //By Zmc18_Robotics Zminecrafter @mc.zminecrafter_18 @Zmc18_Roboticz
    
    switch(rgbMode) {
        case 2: // Blink mode
            if (now - lastRgbUpdate >= 500) {
                lastRgbUpdate = now;
                if (rgbBlinkState == 0) {
                    setRGB(currentR, currentG, currentB);
                    rgbBlinkState = 1;
                } else {
                    setRGB(0, 0, 0);
                    rgbBlinkState = 0;
                }
            }
            break;
            
        case 3: // Random blink mode
            if (now - lastRgbUpdate >= 300) {
                lastRgbUpdate = now;
                setRGB(random(0, 256), random(0, 256), random(0, 256));
            }
            break;
            
        case 4: // Fade in/out mode
            if (now - lastRgbUpdate >= 20) {
                lastRgbUpdate = now;
                rgbFadeValue += (5 * rgbFadeDirection);
                //By Zmc18_Robotics Zminecrafter @mc.zminecrafter_18 @Zmc18_Roboticz
                if (rgbFadeValue >= 255) {
                    rgbFadeValue = 255;
                    rgbFadeDirection = -1;
                } else if (rgbFadeValue <= 0) {
                    rgbFadeValue = 0;
                    rgbFadeDirection = 1;
                }
                
                int r = map(rgbFadeValue, 0, 255, 0, currentR);
                int g = map(rgbFadeValue, 0, 255, 0, currentG);
                int b = map(rgbFadeValue, 0, 255, 0, currentB);
                setRGB(r, g, b);
            }
            break;
            
        case 5: // Police siren mode
            if (now - lastRgbUpdate >= 200) {
                lastRgbUpdate = now;
                
                if (rgbPoliceCount < 6) {
                    // Red phase (3 blinks)
                    if (rgbBlinkState == 0) {
                        setRGB(255, 0, 0);
                        rgbBlinkState = 1;
                    } else {
                        setRGB(0, 0, 0);
                        rgbBlinkState = 0;
                    }
                    rgbPoliceCount++;
                } else if (rgbPoliceCount < 12) {
                    // Blue phase (3 blinks)
                    if (rgbBlinkState == 0) {
                        setRGB(0, 0, 255);
                        rgbBlinkState = 1;
                    } else {
                        setRGB(0, 0, 0);
                        rgbBlinkState = 0;
                    }
                    rgbPoliceCount++;
                } else {
                    rgbPoliceCount = 0;
                }
            }
            break;
    }
}

void handleRgbOn() {
    rgbActive = true;
    rgbMode = 1;
    setRGB(255, 255, 255); // Default white
    Serial.println("[RGB] LED ON - White");
    server.send(200, "text/plain", "OK");
}

void handleRgbOff() {
    rgbActive = false;
    rgbMode = 0;
    setRGB(0, 0, 0);
    Serial.println("[RGB] LED OFF");
    server.send(200, "text/plain", "OK");
}

void handleRgbColor() {
    if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
        int r = server.arg("r").toInt();
        int g = server.arg("g").toInt();
        int b = server.arg("b").toInt();
        
        r = constrain(r, 0, 255);
        g = constrain(g, 0, 255);
        b = constrain(b, 0, 255);
        
        rgbActive = true;
        rgbMode = 1;
        setRGB(r, g, b);
        
        Serial.println("[RGB] Color set: R=" + String(r) + " G=" + String(g) + " B=" + String(b));
        server.send(200, "text/plain", "OK");
        return;
    }
    server.send(400, "text/plain", "Invalid parameters");
}

void handleRgbBlink() {
    rgbActive = true;
    rgbMode = 2;
    rgbBlinkState = 0;
    lastRgbUpdate = 0;
    
    // Use current color or default to white
    if (currentR == 0 && currentG == 0 && currentB == 0) {
        currentR = 255;
        currentG = 255;
        currentB = 255;
    }
    
    Serial.println("[RGB] Blink mode activated");
    server.send(200, "text/plain", "OK");
}

void handleRgbRandom() {
    rgbActive = true;
    rgbMode = 3;
    lastRgbUpdate = 0;
    Serial.println("[RGB] Random blink mode activated");
    server.send(200, "text/plain", "OK");
}

void handleRgbFade() {
    if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
        int r = server.arg("r").toInt();
        int g = server.arg("g").toInt();
        int b = server.arg("b").toInt();
        
        currentR = constrain(r, 0, 255);
        currentG = constrain(g, 0, 255);
        currentB = constrain(b, 0, 255);
    } else {
        // Default to white if no color specified
        currentR = 255;
        currentG = 255;
        currentB = 255;
    }
    
    rgbActive = true;
    rgbMode = 4;
    rgbFadeValue = 0;
    rgbFadeDirection = 1;
    lastRgbUpdate = 0;
    //By Zmc18_Robotics Zminecrafter @mc.zminecrafter_18 @Zmc18_Roboticz
    Serial.println("[RGB] Fade mode activated");
    server.send(200, "text/plain", "OK");
}

void handleRgbPolice() {
    rgbActive = true;
    rgbMode = 5;
    rgbPoliceCount = 0;
    rgbBlinkState = 0;
    lastRgbUpdate = 0;
    Serial.println("[RGB] Police siren mode activated");
    server.send(200, "text/plain", "OK");
}

