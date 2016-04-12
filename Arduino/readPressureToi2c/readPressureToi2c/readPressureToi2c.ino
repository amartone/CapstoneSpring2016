#include <Wire.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1305.h>

// Used for software SPI
//#define OLED_CLK 2
//#define OLED_MOSI 1

// Used for software or hardware SPI
//#define OLED_CS 4
//#define OLED_DC 0

// Used for I2C or SPI
//#define OLED_RESET 5

const int kTransducer = A3;

const int kI2cSlaveAddress = 0x77;
const char kResponsePressureNotYetAvailable[] = {(char) 0x23, (char) 0x23, (char) 0x23};
const char kResponsePressureAvailable = (char) 0x24;

void OnReceive(int numBytes) {
  Serial.println("Received data");
  while (Wire.available()) {
    Wire.read();
  }
}

void OnRequest() {
  // Read from the transducer; this is the only value that the master needs.
  int val = analogRead(kTransducer);

  unsigned char to_write[3];
  to_write[0] = kResponsePressureAvailable;
  to_write[1] = (unsigned char) val;
  to_write[2] = (unsigned char) (val >> 8);

  Serial.println("Sent data");
  Wire.write(to_write, 3);
}

void setup() {

  // Set up the serial connection.
  Serial.begin(9600);

  // Set up the display.
  /*Serial.println("Firing up display...");
  Adafruit_SSD1305 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
  display.begin();
  display.display();
  delay(1000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(BLACK, WHITE);
  display.setCursor(0,0);
  display.println("Hello, world!");
  display.display();*/

  // Set up the I2C connection.
  Wire.begin(kI2cSlaveAddress);
  Wire.onRequest(OnRequest);
  Wire.onReceive(OnReceive);
  Serial.println("Registered I2C connection...");
}

void loop() {
  // Unused.
}
