/* 7 color E-Ink Desk Calendar DIsplay
  Uses the 5.63" 7 color display from waveshare

  https://github.com/rhovious/7ColorEinkDeskCalendar
*/

// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable or disable GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_7C.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h> //CURRENT DAY
#include <Fonts/FreeSansBold18pt7b.h> //MONTH NAME
#include <Fonts/FreeSans12pt7b.h> //DAYS OF WEEK SUNDAY THRU FRIDAY
#include <U8g2_for_Adafruit_GFX.h>
//U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

#include "secrets.h"
#include "GxEPD2_github_raw_certs.h"
#include "personalcerts.h"

// TREE INCLUDES
#include "Geometry_1.2.h" // geometry file

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~SETTINGS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int MODESET = 1; //1 = piccal, 2 = tree

uint16_t calendarMonthColor = GxEPD_BLACK; //GxEPD_WHITE
uint16_t calendarDaysOfWeekColor = GxEPD_BLACK;
uint16_t calendarAllDaysColor = GxEPD_BLACK;
uint16_t calendarLineColor = GxEPD_BLACK;
uint16_t calendarCurrentDayColor = GxEPD_RED;
uint16_t complementToBGColor = GxEPD_BLACK; //uaed for text printing other than the calendar

// settings for tree only
int randomBGMode = 0; //0 - no randomg BG, settings below. 1 = random BG color
uint16_t mainBGColor = GxEPD_GREEN;
uint16_t mainBranchColor = GxEPD_BLACK;

long SleepDuration = 30; // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -28800;   // offset hours *60 *60
const int daylightOffset_sec = 3600; // set to 3600 if your contry observes DST

const char *ssid = SECRETS_SSID;
const char *password = SECRETS_PW;

/*
  // Connections for LOLIN D32
  static const uint8_t EPD_BUSY = 4;  // to EPD BUSY
  static const uint8_t EPD_CS = 5;    // to EPD CS
  static const uint8_t EPD_RST = 19;  // to EPD RST
  static const uint8_t EPD_DC = 21;   // to EPD DC
  static const uint8_t EPD_SCK = 18;  // to EPD CLK
  static const uint8_t EPD_MISO = 16; // Master-In Slave-Out not used, as no data from display
  static const uint8_t EPD_MOSI = 23; // to EPD DIN
*/

// Connections for Waveshare ESP32 e-Paper Driver Board
static const uint8_t EPD_BUSY = 25;
static const uint8_t EPD_CS   = 15;
static const uint8_t EPD_RST  = 26;
static const uint8_t EPD_DC   = 27;
static const uint8_t EPD_SCK  = 13;
static const uint8_t EPD_MISO = 12; // Master-In Slave-Out not used, as no data from display
static const uint8_t EPD_MOSI = 14;


#define LED_BUILTIN 2

GxEPD2_7C < GxEPD2_565c, GxEPD2_565c::HEIGHT / 2 > display(GxEPD2_565c(/*CS=*/EPD_CS, /*DC=*/EPD_DC, /*RST=*/EPD_RST, /*BUSY=*/EPD_BUSY)); // Waveshare 5.65" 7-color

const int httpPort = 80;
const int httpsPort = 443;

// note: the certificates have been moved to a separate header file, as R"CERT( destroys IDE Auto Format capability

const char *certificate_rawcontent = personal_cert; // ok, should work until 2031-04-13 23:59:59 //cert_DigiCert_TLS_RSA_SHA256_2020_CA1
// const char* certificate_rawcontent = github_io_chain_pem_first;  // ok, should work until Tue, 21 Mar 2023 23:59:59 GMT
// const char* certificate_rawcontent = github_io_chain_pem_second;  // ok, should work until Tue, 21 Mar 2023 23:59:59 GMT
// const char* certificate_rawcontent = github_io_chain_pem_third;  // ok, should work until Tue, 21 Mar 2023 23:59:59 GMT

const char *fp_rawcontent = "8F 0E 79 24 71 C5 A7 D2 A7 46 76 30 C1 3C B7 2A 13 B0 01 B2"; // as of 29.7.2022

// note that BMP bitmaps are drawn at physical position in physical orientation of the screen
// void showBitmapFrom_HTTP(const char* host, const char* path, const char* filename, int16_t x, int16_t y, bool with_color = true);
// void showBitmapFrom_HTTPS(const char* host, const char* path, const char* filename, const char* fingerprint, int16_t x, int16_t y, bool with_color = true,
// const char* certificate = certificate_rawcontent);

// draws BMP bitmap according to set orientation
void showBitmapFrom_HTTP_Buffered(const char *host, const char *path, const char *filename, int16_t x, int16_t y, bool with_color = true);
void showBitmapFrom_HTTPS_Buffered(const char *host, const char *path, const char *filename, const char *fingerprint, int16_t x, int16_t y, bool with_color = true,
                                   const char *certificate = certificate_rawcontent);

Point b1, b2; // needed for tree

char timeCurrentYear[5];
// char timeCurrentDayNum[10];
char timeCurrentMonth[10];
char timeWeekDayName[10];

int timeCurrentDayOfWeekNum;
int timeCurrentDayofMonthNum;
int timeDaysInMonth;

int timecurrentHour;
int timecurrentMinute;
int timecurrentSecond;


int CurrentMinForSleep = 0, CurrentSecForSleep = 0;
long StartTime = 0;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~SETUP~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setup()
{
  //NEEDED FOR WAVESHARE BOARDS
  SPI.end(); // release standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  // SPI: void begin(int8_t sck=-1, int8_t miso=-1, int8_t mosi=-1, int8_t ss=-1);
  SPI.begin(13, 12, 14, 15); // map and init SPI pins SCK(13), MISO(12), MOSI(14), SS(15)

  Serial.begin(115200);
  Serial.println();
  Serial.println("7 Color eInk Desk Calendar");
  StartTime = millis();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)

  initWiFi();

  display.init(115200); // default 10ms reset pulse, e.g. for bare panels with DESPI-C02
  // display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  display.setRotation(3);

  setClock();

  if (MODESET == 1) //PIC CAL DRAW
  {
    drawPicCalScreen();
  }
  else if (MODESET == 2) //TREE DRAW
  {
    if (randomBGMode == 1) //selects random BG color. Otherwise use from settings
    {
      mainBGColor = selectRandomColor();
    }
    drawTreeCalScreen(mainBGColor);
  }
  delay(1000);                    // wait for 1 second
  digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
  Serial.println("Start Sleep");
  BeginSleep();
}

void loop(void)
{
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~TREE DRAW FUNCTIONS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void drawTreeCalScreen(uint16_t BGColor)
{
  Serial.println("drawTreeCalScreen running...");

  b1.X() = 0;
  b1.Y() = display.height() / 2; //make b2 y the same. moves where tree is located in screen
  b2.X() = display.width() / 3; //tree height
  b2.Y() = display.height() / 2;

  display.setRotation(3);

  Serial.println("\"");
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(BGColor);
    drawTestColorBoxes();

    drawTree(b1, b2, mainBranchColor);
    displayText();

    drawCalendar(50, 50);
  } while (display.nextPage());
  delay(2000);
}

// #########################################################################################
//  Recursive function to to draw tree

void drawTree(Point branchStart, Point branchEnd, uint16_t branchColor)
{
  display.drawLine(branchStart.X(), branchStart.Y(), branchEnd.X(), branchEnd.Y(), branchColor);
  // Create new branches
  Point d1 = branchEnd - branchStart;
  Point newBranch = d1 * 0.65;

  int tempMagnitude = random(1, 10); // 5 default

  if (newBranch.Magnitude() > tempMagnitude)
  {

    // Rotation
    Rotation R1, R2;

    int tempRotate1 = random(1, 100); // 25 default
    int tempRotate2 = random(1, 100); // 25 default

    tempRotate1 = tempRotate1 / 100;
    tempRotate2 = tempRotate2 / 100;
    R1.RotateZ(0.25);
    R2.RotateZ(-0.25);

    // New branch 1
    Point newEnd1 = R1 * newBranch; // rotated clockwise
    newEnd1 += branchEnd;
    // display.drawLine(branchEnd.X(), branchEnd.Y(), newEnd1.X(), newEnd1.Y(), branchColor);
    drawTree(branchEnd, newEnd1, branchColor);

    // New branch 2
    Point newEnd2 = R2 * newBranch; // rotated counter-clockwise
    newEnd2 += branchEnd;
    // display.drawLine(branchEnd.X(), branchEnd.Y(), newEnd2.X(), newEnd2.Y(), branchColor);
    drawTree(branchEnd, newEnd2, branchColor);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~PICTURE DRAW FUNCTIONS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void drawPicCalScreen()
{

  Serial.println("drawPicCalScreen running...");
  Serial.println("Pic 1");
  showBitmapFrom_HTTPS_Buffered(host_rawcontent, path_toPicFolder, "test1-out-gimp.bmp", fp_rawcontent, 0, 0);
  delay(2000);

    Serial.println("Pic 2");
  showBitmapFrom_HTTPS_Buffered(host_rawcontent, path_toPicFolder, "pic1_rotated.bmp", fp_rawcontent, 0, 0);
  delay(2000);
}

static const uint16_t input_buffer_pixels = 800; // may affect performance

static const uint16_t max_row_width = 1872;     // for up to 7.8" display 1872x1404
static const uint16_t max_palette_pixels = 256; // for depth <= 8

uint8_t input_buffer[3 * input_buffer_pixels];        // up to depth 24
uint8_t output_row_mono_buffer[max_row_width / 8];    // buffer for at least one row of b/w bits
uint8_t output_row_color_buffer[max_row_width / 8];   // buffer for at least one row of color bits
uint8_t mono_palette_buffer[max_palette_pixels / 8];  // palette buffer for depth <= 8 b/w
uint8_t color_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 c/w
uint16_t rgb_palette_buffer[max_palette_pixels];      // palette buffer for depth <= 8 for buffered graphics, needed for 7-color display
// #########################################################################################

void drawBitmapFrom_HTTP_ToBuffer(const char *host, const char *path, const char *filename, int16_t x, int16_t y, bool with_color)
{
  WiFiClient client;
  bool connection_ok = false;
  bool valid = false; // valid format to be handled
  bool flip = true;   // bitmap is stored bottom-to-top
  bool has_multicolors = (display.epd2.panel == GxEPD2::ACeP565) || (display.epd2.panel == GxEPD2::GDEY073D46);
  uint32_t startTime = millis();
  if ((x >= display.width()) || (y >= display.height()))
    return;
  display.fillScreen(GxEPD_WHITE);
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpPort))
  {
    Serial.println("connection failed");
    return;
  }
  Serial.print("requesting URL: ");
  Serial.println(String("http://") + host + path + filename);
  client.print(String("GET ") + path + filename + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: GxEPD2_WiFi_Example\r\n" +
               "Connection: close\r\n\r\n");
  Serial.println("request sent");
  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    if (!connection_ok)
    {
      connection_ok = line.startsWith("HTTP/1.1 200 OK");
      if (connection_ok)
        Serial.println(line);
      // if (!connection_ok) Serial.println(line);
    }
    if (!connection_ok)
      Serial.println(line);
    // Serial.println(line);
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
  }
  if (!connection_ok)
    return;
  // Parse BMP header
  if (read16(client) == 0x4D42) // BMP signature
  {
    uint32_t fileSize = read32(client);
    uint32_t creatorBytes = read32(client);
    (void)creatorBytes;                    // unused
    uint32_t imageOffset = read32(client); // Start of image data
    uint32_t headerSize = read32(client);
    uint32_t width = read32(client);
    int32_t height = (int32_t)read32(client);
    uint16_t planes = read16(client);
    uint16_t depth = read16(client); // bits per pixel
    uint32_t format = read32(client);
    uint32_t bytes_read = 7 * 4 + 3 * 2;                   // read so far
    if ((planes == 1) && ((format == 0) || (format == 3))) // uncompressed is handled, 565 also
    {
      Serial.print("File size: ");
      Serial.println(fileSize);
      Serial.print("Image Offset: ");
      Serial.println(imageOffset);
      Serial.print("Header size: ");
      Serial.println(headerSize);
      Serial.print("Bit Depth: ");
      Serial.println(depth);
      Serial.print("Image size: ");
      Serial.print(width);
      Serial.print('x');
      Serial.println(height);
      // BMP rows are padded (if needed) to 4-byte boundary
      uint32_t rowSize = (width * depth / 8 + 3) & ~3;
      if (depth < 8)
        rowSize = ((width * depth + 8 - depth) / 8 + 3) & ~3;
      if (height < 0)
      {
        height = -height;
        flip = false;
      }
      uint16_t w = width;
      uint16_t h = height;
      if ((x + w - 1) >= display.width())
        w = display.width() - x;
      if ((y + h - 1) >= display.height())
        h = display.height() - y;
      // if (w <= max_row_width) // handle with direct drawing
      {
        valid = true;
        uint8_t bitmask = 0xFF;
        uint8_t bitshift = 8 - depth;
        uint16_t red, green, blue;
        bool whitish = false;
        bool colored = false;
        if (depth == 1)
          with_color = false;
        if (depth <= 8)
        {
          if (depth < 8)
            bitmask >>= depth;
          // bytes_read += skip(client, 54 - bytes_read); //palette is always @ 54
          bytes_read += skip(client, imageOffset - (4 << depth) - bytes_read); // 54 for regular, diff for colorsimportant
          for (uint16_t pn = 0; pn < (1 << depth); pn++)
          {
            blue = client.read();
            green = client.read();
            red = client.read();
            client.read();
            bytes_read += 4;
            whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
            colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
            if (0 == pn % 8)
              mono_palette_buffer[pn / 8] = 0;
            mono_palette_buffer[pn / 8] |= whitish << pn % 8;
            if (0 == pn % 8)
              color_palette_buffer[pn / 8] = 0;
            color_palette_buffer[pn / 8] |= colored << pn % 8;
            // Serial.print("0x00"); Serial.print(red, HEX); Serial.print(green, HEX); Serial.print(blue, HEX);
            // Serial.print(" : "); Serial.print(whitish); Serial.print(", "); Serial.println(colored);
            rgb_palette_buffer[pn] = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
          }
        }
        uint32_t rowPosition = flip ? imageOffset + (height - h) * rowSize : imageOffset;
        // Serial.print("skip "); Serial.println(rowPosition - bytes_read);
        bytes_read += skip(client, rowPosition - bytes_read);
        for (uint16_t row = 0; row < h; row++, rowPosition += rowSize) // for each line
        {
          if (!connection_ok || !(client.connected() || client.available()))
            break;
          delay(1); // yield() to avoid WDT
          uint32_t in_remain = rowSize;
          uint32_t in_idx = 0;
          uint32_t in_bytes = 0;
          uint8_t in_byte = 0; // for depth <= 8
          uint8_t in_bits = 0; // for depth <= 8
          uint16_t color = GxEPD_WHITE;
          for (uint16_t col = 0; col < w; col++) // for each pixel
          {
            yield();
            if (!connection_ok || !(client.connected() || client.available()))
              break;
            // Time to read more pixel data?
            if (in_idx >= in_bytes) // ok, exact match for 24bit also (size IS multiple of 3)
            {
              uint32_t get = in_remain > sizeof(input_buffer) ? sizeof(input_buffer) : in_remain;
              uint32_t got = read8n(client, input_buffer, get);
              while ((got < get) && connection_ok)
              {
                // Serial.print("got "); Serial.print(got); Serial.print(" < "); Serial.print(get); Serial.print(" @ "); Serial.println(bytes_read);
                uint32_t gotmore = read8n(client, input_buffer + got, get - got);
                got += gotmore;
                connection_ok = gotmore > 0;
              }
              in_bytes = got;
              in_remain -= got;
              bytes_read += got;
            }
            if (!connection_ok)
            {
              Serial.print("Error: got no more after ");
              Serial.print(bytes_read);
              Serial.println(" bytes read!");
              break;
            }
            switch (depth)
            {
              case 32:
                blue = input_buffer[in_idx++];
                green = input_buffer[in_idx++];
                red = input_buffer[in_idx++];
                in_idx++;                                                                                                     // skip alpha
                whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
                color = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
                break;
              case 24:
                blue = input_buffer[in_idx++];
                green = input_buffer[in_idx++];
                red = input_buffer[in_idx++];
                whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
                color = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
                break;
              case 16:
                {
                  uint8_t lsb = input_buffer[in_idx++];
                  uint8_t msb = input_buffer[in_idx++];
                  if (format == 0) // 555
                  {
                    blue = (lsb & 0x1F) << 3;
                    green = ((msb & 0x03) << 6) | ((lsb & 0xE0) >> 2);
                    red = (msb & 0x7C) << 1;
                    color = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
                  }
                  else // 565
                  {
                    blue = (lsb & 0x1F) << 3;
                    green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
                    red = (msb & 0xF8);
                    color = (msb << 8) | lsb;
                  }
                  whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                  colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
                }
                break;
              case 1:
              case 2:
              case 4:
              case 8:
                {
                  if (0 == in_bits)
                  {
                    in_byte = input_buffer[in_idx++];
                    in_bits = 8;
                  }
                  uint16_t pn = (in_byte >> bitshift) & bitmask;
                  whitish = mono_palette_buffer[pn / 8] & (0x1 << pn % 8);
                  colored = color_palette_buffer[pn / 8] & (0x1 << pn % 8);
                  in_byte <<= depth;
                  in_bits -= depth;
                  color = rgb_palette_buffer[pn];
                }
                break;
            }
            if (with_color && has_multicolors)
            {
              // keep color
            }
            else if (whitish)
            {
              color = GxEPD_WHITE;
            }
            else if (colored && with_color)
            {
              color = GxEPD_COLORED;
            }
            else
            {
              color = GxEPD_BLACK;
            }
            uint16_t yrow = y + (flip ? h - row - 1 : row);
            display.drawPixel(x + col, yrow, color);
          } // end pixel
        }   // end line
      }
      Serial.print("bytes read ");
      Serial.println(bytes_read);
    }
  }
  Serial.print("loaded in ");
  Serial.print(millis() - startTime);
  Serial.println(" ms");
  client.stop();
  if (!valid)
  {
    Serial.println("bitmap format not handled.");
  }
}

void showBitmapFrom_HTTP_Buffered(const char *host, const char *path, const char *filename, int16_t x, int16_t y, bool with_color)
{
  Serial.println();
  Serial.print("downloading file \"");
  Serial.print(filename);
  Serial.println("\"");
  display.setFullWindow();
  display.firstPage();
  do
  {
    drawBitmapFrom_HTTP_ToBuffer(host, path, filename, x, y, with_color);
  } while (display.nextPage());
}
// #########################################################################################

void drawBitmapFrom_HTTPS_ToBuffer(const char *host, const char *path, const char *filename, const char *fingerprint, int16_t x, int16_t y, bool with_color, const char *certificate)
{
  // Use WiFiClientSecure class to create TLS connection
#if defined(ESP8266)
  BearSSL::WiFiClientSecure client;
  BearSSL::X509List cert(certificate ? certificate : certificate_rawcontent);
#else
  WiFiClientSecure client;
#endif
  bool connection_ok = false;
  bool valid = false; // valid format to be handled
  bool flip = true;   // bitmap is stored bottom-to-top
  bool has_multicolors = (display.epd2.panel == GxEPD2::ACeP565) || (display.epd2.panel == GxEPD2::GDEY073D46);
  uint32_t startTime = millis();
  if ((x >= display.width()) || (y >= display.height()))
    return;
  display.fillScreen(GxEPD_WHITE);
  Serial.print("connecting to ");
  Serial.println(host);
#if defined(ESP8266)
  client.setBufferSizes(4096, 4096); // required
  // client.setBufferSizes(8192, 4096); // may help for some sites
  if (certificate)
    client.setTrustAnchors(&cert);
  else if (fingerprint)
    client.setFingerprint(fingerprint);
  else
    client.setInsecure();
#elif defined(ESP32)
  if (certificate)
    client.setCACert(certificate);
#endif
  if (!client.connect(host, httpsPort))
  {
    Serial.println("connection failed");
    return;
  }
  Serial.print("requesting URL: ");
  Serial.println(String("https://") + host + path + filename);
  client.print(String("GET ") + path + filename + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: GxEPD2_WiFi_Example\r\n" +
               "Connection: close\r\n\r\n");
  Serial.println("request sent");
  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    if (!connection_ok)
    {
      connection_ok = line.startsWith("HTTP/1.1 200 OK");
      if (connection_ok)
        Serial.println(line);
      // if (!connection_ok) Serial.println(line);
    }
    if (!connection_ok)
      Serial.println(line);
    // Serial.println(line);
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
  }
  if (!connection_ok)
    return;
  // Parse BMP header
  // if (read16(client) == 0x4D42) // BMP signature
  uint16_t signature = 0;
  for (int16_t i = 0; i < 50; i++)
  {
    if (!client.available())
      delay(100);
    else
      signature = read16(client);
    if (signature == 0x4D42)
    {
      // Serial.print("signature wait loops: "); Serial.println(i);
      break;
    }
  }
  if (signature == 0x4D42) // BMP signature
  {
    uint32_t fileSize = read32(client);
    uint32_t creatorBytes = read32(client);
    (void)creatorBytes;                    // unused
    uint32_t imageOffset = read32(client); // Start of image data
    uint32_t headerSize = read32(client);
    uint32_t width = read32(client);
    int32_t height = (int32_t)read32(client);
    uint16_t planes = read16(client);
    uint16_t depth = read16(client); // bits per pixel
    uint32_t format = read32(client);
    uint32_t bytes_read = 7 * 4 + 3 * 2;                   // read so far
    if ((planes == 1) && ((format == 0) || (format == 3))) // uncompressed is handled, 565 also
    {
      Serial.print("File size: ");
      Serial.println(fileSize);
      Serial.print("Image Offset: ");
      Serial.println(imageOffset);
      Serial.print("Header size: ");
      Serial.println(headerSize);
      Serial.print("Bit Depth: ");
      Serial.println(depth);
      Serial.print("Image size: ");
      Serial.print(width);
      Serial.print('x');
      Serial.println(height);
      // BMP rows are padded (if needed) to 4-byte boundary
      uint32_t rowSize = (width * depth / 8 + 3) & ~3;
      if (depth < 8)
        rowSize = ((width * depth + 8 - depth) / 8 + 3) & ~3;
      if (height < 0)
      {
        height = -height;
        flip = false;
      }
      uint16_t w = width;
      uint16_t h = height;
      if ((x + w - 1) >= display.width())
        w = display.width() - x;
      if ((y + h - 1) >= display.height())
        h = display.height() - y;
      // if (w <= max_row_width) // handle with direct drawing
      {
        valid = true;
        uint8_t bitmask = 0xFF;
        uint8_t bitshift = 8 - depth;
        uint16_t red, green, blue;
        bool whitish = false;
        bool colored = false;
        if (depth == 1)
          with_color = false;
        if (depth <= 8)
        {
          if (depth < 8)
            bitmask >>= depth;
          // bytes_read += skip(client, 54 - bytes_read); //palette is always @ 54
          bytes_read += skip(client, imageOffset - (4 << depth) - bytes_read); // 54 for regular, diff for colorsimportant
          for (uint16_t pn = 0; pn < (1 << depth); pn++)
          {
            blue = client.read();
            green = client.read();
            red = client.read();
            client.read();
            bytes_read += 4;
            whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
            colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
            if (0 == pn % 8)
              mono_palette_buffer[pn / 8] = 0;
            mono_palette_buffer[pn / 8] |= whitish << pn % 8;
            if (0 == pn % 8)
              color_palette_buffer[pn / 8] = 0;
            color_palette_buffer[pn / 8] |= colored << pn % 8;
            // Serial.print("0x00"); Serial.print(red, HEX); Serial.print(green, HEX); Serial.print(blue, HEX);
            // Serial.print(" : "); Serial.print(whitish); Serial.print(", "); Serial.println(colored);
            rgb_palette_buffer[pn] = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
          }
        }
        uint32_t rowPosition = flip ? imageOffset + (height - h) * rowSize : imageOffset;
        // Serial.print("skip "); Serial.println(rowPosition - bytes_read);
        bytes_read += skip(client, rowPosition - bytes_read);
        for (uint16_t row = 0; row < h; row++, rowPosition += rowSize) // for each line
        {
          if (!connection_ok || !(client.connected() || client.available()))
            break;
          delay(1); // yield() to avoid WDT
          uint32_t in_remain = rowSize;
          uint32_t in_idx = 0;
          uint32_t in_bytes = 0;
          uint8_t in_byte = 0; // for depth <= 8
          uint8_t in_bits = 0; // for depth <= 8
          uint16_t color = GxEPD_WHITE;
          for (uint16_t col = 0; col < w; col++) // for each pixel
          {
            yield();
            if (!connection_ok || !(client.connected() || client.available()))
              break;
            // Time to read more pixel data?
            if (in_idx >= in_bytes) // ok, exact match for 24bit also (size IS multiple of 3)
            {
              uint32_t get = in_remain > sizeof(input_buffer) ? sizeof(input_buffer) : in_remain;
              uint32_t got = read8n(client, input_buffer, get);
              while ((got < get) && connection_ok)
              {
                // Serial.print("got "); Serial.print(got); Serial.print(" < "); Serial.print(get); Serial.print(" @ "); Serial.println(bytes_read);
                uint32_t gotmore = read8n(client, input_buffer + got, get - got);
                got += gotmore;
                connection_ok = gotmore > 0;
              }
              in_bytes = got;
              in_remain -= got;
              bytes_read += got;
            }
            if (!connection_ok)
            {
              Serial.print("Error: got no more after ");
              Serial.print(bytes_read);
              Serial.println(" bytes read!");
              break;
            }
            switch (depth)
            {
              case 32:
                blue = input_buffer[in_idx++];
                green = input_buffer[in_idx++];
                red = input_buffer[in_idx++];
                in_idx++;                                                                                                     // skip alpha
                whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
                color = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
                break;
              case 24:
                blue = input_buffer[in_idx++];
                green = input_buffer[in_idx++];
                red = input_buffer[in_idx++];
                whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
                color = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
                break;
              case 16:
                {
                  uint8_t lsb = input_buffer[in_idx++];
                  uint8_t msb = input_buffer[in_idx++];
                  if (format == 0) // 555
                  {
                    blue = (lsb & 0x1F) << 3;
                    green = ((msb & 0x03) << 6) | ((lsb & 0xE0) >> 2);
                    red = (msb & 0x7C) << 1;
                    color = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
                  }
                  else // 565
                  {
                    blue = (lsb & 0x1F) << 3;
                    green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
                    red = (msb & 0xF8);
                    color = (msb << 8) | lsb;
                  }
                  whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                  colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
                }
                break;
              case 1:
              case 2:
              case 4:
              case 8:
                {
                  if (0 == in_bits)
                  {
                    in_byte = input_buffer[in_idx++];
                    in_bits = 8;
                  }
                  uint16_t pn = (in_byte >> bitshift) & bitmask;
                  whitish = mono_palette_buffer[pn / 8] & (0x1 << pn % 8);
                  colored = color_palette_buffer[pn / 8] & (0x1 << pn % 8);
                  in_byte <<= depth;
                  in_bits -= depth;
                  color = rgb_palette_buffer[pn];
                }
                break;
            }
            if (with_color && has_multicolors)
            {
              // keep color
            }
            else if (whitish)
            {
              color = GxEPD_WHITE;
            }
            else if (colored && with_color)
            {
              color = GxEPD_COLORED;
            }
            else
            {
              color = GxEPD_BLACK;
            }
            uint16_t yrow = y + (flip ? h - row - 1 : row);
            display.drawPixel(x + col, yrow, color);
          } // end pixel
        }   // end line
      }
      Serial.print("bytes read ");
      Serial.println(bytes_read);
    }
  }
  Serial.print("loaded in ");
  Serial.print(millis() - startTime);
  Serial.println(" ms");
  client.stop();
  if (!valid)
  {
    Serial.println("bitmap format not handled.");
  }
}
// #########################################################################################

void showBitmapFrom_HTTPS_Buffered(const char *host, const char *path, const char *filename, const char *fingerprint, int16_t x, int16_t y, bool with_color, const char *certificate)
{
  Serial.println();
  Serial.print("downloading file \"");
  Serial.print(filename);
  Serial.println("\"");
  display.setFullWindow();
  display.firstPage();
  do
  {
    drawTestColorBoxes();
    drawBitmapFrom_HTTPS_ToBuffer(host, path, filename, fingerprint, x, y, with_color, certificate);
    displayText();

    drawCalendar(50, 50);
  } while (display.nextPage());
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~OTHER FUNCTIONS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void InitialiseDisplay()
{
}
// #########################################################################################
void initWiFi()
{

#ifdef RE_INIT_NEEDED
  WiFi.persistent(true);
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.disconnect();
#endif

  if (!WiFi.getAutoConnect() || (WiFi.getMode() != WIFI_STA) || ((WiFi.SSID() != ssid) && String(ssid) != "........"))
  {
    Serial.println();
    Serial.print("WiFi.getAutoConnect() = ");
    Serial.println(WiFi.getAutoConnect());
    Serial.print("WiFi.SSID() = ");
    Serial.println(WiFi.SSID());
    WiFi.mode(WIFI_STA); // switch off AP
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
  }
  int ConnectTimeout = 30; // 15 seconds
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    Serial.print(WiFi.status());
    if (--ConnectTimeout <= 0)
    {
      Serial.println();
      Serial.println("WiFi connect timeout");
      return;
    }
  }
  Serial.println();
  Serial.println("WiFi connected");

  // Print the IP address
  Serial.println(WiFi.localIP());
}
// #########################################################################################

void BeginSleep()
{
  display.powerOff();
  long SleepTimer = (SleepDuration * 60 - ((CurrentMinForSleep % SleepDuration) * 60 + CurrentSecForSleep)); // Some ESP32 are too fast to maintain accurate time
  esp_sleep_enable_timer_wakeup((SleepTimer + 20) * 1000000LL);                                              // Added extra 20-secs of sleep to allow for slow ESP32 RTC timers
#ifdef BUILTIN_LED
  pinMode(BUILTIN_LED, INPUT); // If it's On, turn it off and some boards use GPIO-5 for SPI-SS, which remains low after screen use
  digitalWrite(BUILTIN_LED, HIGH);
#endif
  Serial.println("Entering " + String(SleepTimer) + "-secs of sleep time");
  Serial.println("Awake for : " + String((millis() - StartTime) / 1000.0, 3) + "-secs");
  Serial.println("Starting deep-sleep period...");
  esp_deep_sleep_start(); // Sleep for e.g. 30 minutes
}
// #########################################################################################
void drawString(int x, int y, int rotationSet, String inputText, uint16_t textColor)
{
  // u8g2Fonts.setFontMode(1);                   // use u8g2 transparent mode (this is default)
  // u8g2Fonts.setFontDirection(0);              // left to right (this is default)
  // u8g2Fonts.setForegroundColor(textColor);  // apply Adafruit GFX color


  display.setRotation(rotationSet); // rotates display for text printing
  // display.setTextSize(1);
  display.setTextColor(textColor);
  display.setCursor(x, y); // start writing at this position
  display.print(inputText);
  display.setRotation(3); // sets back to 3 after printing
}
// #########################################################################################

// Displays text on screen other than the calendar
void displayText()
{

  //display.setRotation(0); // rotates display for text printing
  String timeCurrentDayNumString = String(timeCurrentDayofMonthNum);

  display.setFont(&FreeSansBold24pt7b);
  Serial.println("Printing current month displayText: ");
  drawString(10, 35, 0, timeCurrentMonth, complementToBGColor);

  Serial.println("Printing current day number displayText: ");
  drawString(30, 75, 0, timeCurrentDayNumString, complementToBGColor);

  Serial.println("Printing year displayText: ");
  //display.setFont(&FreeMono12pt7b);

  //u8g2Fonts.setFontMode(1);                   // use u8g2 transparent mode (this is default)
  //u8g2Fonts.setFontDirection(0);              // left to right (this is default)
  //u8g2Fonts.setForegroundColor(GxEPD_BLACK);  // apply Adafruit GFX color
  //u8g2Fonts.setBackgroundColor(GxEPD_WHITE);  // apply Adafruit GFX color
  //u8g2Fonts.setFont(u8g2_font_helvR14_tf); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall

  int16_t fx, fy;
  uint16_t w, h;
  display.getTextBounds((char *)timeCurrentYear, 0, 0, &fx, &fy, &w, &h);

  drawString((display.width() - w) - 10, display.height() - 10, 3, timeCurrentYear, complementToBGColor);

  //drawString(10, 90, timeCurrentYear, complementToBGColor);
  Serial.println("Printing week day name displayText: ");
  //drawString(50, display.width() - 20,0, timeWeekDayName, complementToBGColor);

  display.getTextBounds((char *)timeWeekDayName, 0, 0, &fx, &fy, &w, &h);
  drawString((display.width() - w)-30, 420, 0, timeWeekDayName, complementToBGColor);

    //u8g2Fonts.setCursor((display.width() - 20), display.height() - 25); //PLACEMENT OF YEAR
  //u8g2Fonts.println("2023");

}
// #########################################################################################
void drawTestColorBoxes()
{
  display.fillRect(0, 550, 50, 50, GxEPD_RED);
  display.fillRect(50, 550, 50, 50, GxEPD_ORANGE);
  display.fillRect(100, 550, 50, 50, GxEPD_YELLOW);
  display.fillRect(150, 550, 50, 50, GxEPD_GREEN);
  display.fillRect(200, 550, 50, 50, GxEPD_BLUE);
  display.fillRect(250, 550, 50, 50, GxEPD_BLACK);
  display.fillRect(300, 550, 50, 50, GxEPD_WHITE);
  display.drawRect(0, 550, 350, 50, GxEPD_BLACK); // outline for the color boxes
}
// #########################################################################################

uint16_t selectRandomColor()
{
  int tempRandom = random(1, 6); // print a random number from 1 to 5
  uint16_t returnedColor;

  switch (tempRandom)
  {
    case 1:
      returnedColor = GxEPD_RED;
      complementToBGColor = GxEPD_YELLOW;
      break;
    case 2:
      returnedColor = GxEPD_ORANGE;
      complementToBGColor = GxEPD_BLUE;
      break;
    case 3:
      returnedColor = GxEPD_YELLOW;
      complementToBGColor = GxEPD_ORANGE;
      break;
    case 4:
      returnedColor = GxEPD_GREEN;
      complementToBGColor = GxEPD_YELLOW;
      break;
    case 5:
      returnedColor = GxEPD_BLUE;
      complementToBGColor = GxEPD_WHITE;
      break;
    default:
      returnedColor = GxEPD_WHITE;
      complementToBGColor = GxEPD_BLACK;
      break;
  }
  return returnedColor;
}

// #########################################################################################

uint16_t read16(WiFiClient &client)
{
  // BMP data is stored little-endian, same as Arduino.
  uint16_t result;
  ((uint8_t *)&result)[0] = client.read(); // LSB
  ((uint8_t *)&result)[1] = client.read(); // MSB
  return result;
}
// #########################################################################################

uint32_t read32(WiFiClient &client)
{
  // BMP data is stored little-endian, same as Arduino.
  uint32_t result;
  ((uint8_t *)&result)[0] = client.read(); // LSB
  ((uint8_t *)&result)[1] = client.read();
  ((uint8_t *)&result)[2] = client.read();
  ((uint8_t *)&result)[3] = client.read(); // MSB
  return result;
}

// #########################################################################################

uint32_t skip(WiFiClient &client, int32_t bytes)
{
  int32_t remain = bytes;
  uint32_t start = millis();
  while ((client.connected() || client.available()) && (remain > 0))
  {
    if (client.available())
    {
      client.read();
      remain--;
    }
    else
      delay(1);
    if (millis() - start > 2000)
      break; // don't hang forever
  }
  return bytes - remain;
}
// #########################################################################################

uint32_t read8n(WiFiClient &client, uint8_t *buffer, int32_t bytes)
{
  int32_t remain = bytes;
  uint32_t start = millis();
  while ((client.connected() || client.available()) && (remain > 0))
  {
    if (client.available())
    {
      int16_t v = client.read();
      *buffer++ = uint8_t(v);
      remain--;
    }
    else
      delay(1);
    if (millis() - start > 2000)
      break; // don't hang forever
  }
  return bytes - remain;
}
// #########################################################################################

int getDaysInMonth(String Month)
{
  if (Month == "January" || Month == "March" || Month == "May" || Month == "July" || Month == "August" || Month == "October" || Month == "December")
  {
    return 31;
  }
  else if (Month == "April" || Month == "June" || Month == "September" || Month == "November")
  {
    return 30;
  }
  else
  {
    return 28;
  }
}
// #########################################################################################
//  Set time via NTP, as required for x.509 validation
void setClock()
{
  // configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.print("Waiting for NTP time sync: ");
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }

  Serial.println("####### TIME #######");
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));

  Serial.println("Time variables");

  strftime(timeWeekDayName, 10, "%A", &timeinfo);
  Serial.println(timeWeekDayName);

  strftime(timeCurrentMonth, 10, "%B", &timeinfo);
  Serial.println(timeCurrentMonth);

  timeDaysInMonth = getDaysInMonth(timeCurrentMonth);
  Serial.println("days in month: ");
  Serial.println(timeDaysInMonth);

  strftime(timeCurrentYear, 5, "%Y", &timeinfo);
  Serial.println(timeCurrentYear);

  timeCurrentDayOfWeekNum = timeinfo.tm_wday; // days since Sunday 0-6
  Serial.println(timeCurrentDayOfWeekNum);

  timeCurrentDayofMonthNum = timeinfo.tm_mday; /**< day of the month - [ 1 to 31 ] */
  Serial.println(timeCurrentDayofMonthNum);


  timecurrentHour = timeinfo.tm_hour; // hours
  Serial.println(timecurrentHour);
  timecurrentMinute = timeinfo.tm_min; // minutes
  Serial.println(timecurrentMinute);
  timecurrentSecond = timeinfo.tm_sec; // seconds
  Serial.println(timecurrentSecond);

  Serial.println("####### TIME #######");
  Serial.println();
}

// #########################################################################################

int CalendarYOffset = 200;
// draws calendar
void drawCalendar(int x, int y)
{
  display.setRotation(0); // rotates display for text printing

  //String stryear = timeCurrentYear;
  //Serial.println("printing year to screen" + stryear);
  //display.setTextColor(GxEPD_BLACK);
  //display.setFont();
  //display.setCursor(8, 4);
  //display.print(stryear);

  String strmonth = timeCurrentMonth;
  //int dow = timeCurrentDayOfWeekNum - 1;
  int dow = timeCurrentDayOfWeekNum;
  int today = timeCurrentDayofMonthNum;
  int daysinmonth = timeDaysInMonth;

  Serial.println("printing month to screen " + strmonth);
  display.setTextColor(calendarMonthColor);
  display.setFont(&FreeSansBold18pt7b); //9->18

  int16_t fx, fy;
  uint16_t w, h;
  display.getTextBounds((char *)strmonth.c_str(), 0, 0, &fx, &fy, &w, &h);

  display.setCursor((display.width() - w) / 2, 12 + CalendarYOffset);
  display.print(strmonth);

  Serial.println("day of week is " + String(dow));

  int curday = today - dow;
  while (curday > 1)
    curday -= 7;

  char weekDayList[8] = "SMTWTFS";

  x = 0;
  y = 24 + CalendarYOffset + 25; //PLACES WEEK DAY LABEL Y

  for (int i = 0; i < 7; i++)
  {
    Serial.println("printing DOW to screen " + weekDayList[i]);
    x = 4 + i * (display.width() - 8) / 7;
    display.setTextColor(calendarDaysOfWeekColor);
    display.setFont();
    display.setFont(&FreeSans9pt7b); //?->12
    display.getTextBounds(String(weekDayList[i]), 0, 0, &fx, &fy, &w, &h);
    display.setCursor(x + (display.width() / 7 - w) / 2, y - 8);
    display.print(String(weekDayList[i]));
  }
  display.drawLine(0, 24 + CalendarYOffset + 23, display.width(), 24 + CalendarYOffset + 23, calendarLineColor);

  y = 32 + CalendarYOffset + 30; //PLACES START OF DAY LIST Y

  while (curday < daysinmonth)
  {
    for (int i = 0; i < 7; i++)
    {
      x = 4 + i * (display.width() - 8) / 7;
      if (curday >= 1 && curday <= daysinmonth)
      {
        display.setCursor(x, y);
        display.setTextColor(calendarAllDaysColor);
        display.setFont(&FreeSans12pt7b); //9->12 //FreeSans9pt7b
        int16_t fx, fy;
        uint16_t w, h;
        String strday = String(curday);
        if (curday == today)
        {
          display.setFont(&FreeSansBold12pt7b);//9->12 //FreeSans9pt7b
          // display.fillCircle(x + (display.width()-8)/7/2, y, 8, EPD_RED);
          display.setTextColor(calendarCurrentDayColor);
        }
        display.getTextBounds(strday.c_str(), 0, 0, &fx, &fy, &w, &h);
        display.setCursor(x + (display.width() - 8) / 7 / 2 - w / 2 - fx, y + h / 2);
        display.print(curday);
      }
      curday++;
    }
    y += 16 + 5; //INCREASES Y SPACING BETWEEN DAY NUMBERS
  }
  display.setRotation(3); // rotates back at end
}
