// GxEPD2_WiFi_Example : Display Library example for SPI e-paper panels from Dalian Good Display and boards from Waveshare.

// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable or disable GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0

// uncomment next line to use class GFX of library GFX_Root instead of Adafruit_GFX
// #include <GFX.h>
// Note: if you use this with ENABLE_GxEPD2_GFX 1:
//       uncomment it in GxEPD2_GFX.h too, or add #include <GFX.h> before any #include <GxEPD2_GFX.h>

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_7C.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <Fonts/FreeMono12pt7b.h> //https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#include "secrets.h"
#include "GxEPD2_github_raw_certs.h"

// Connections for LOLIN D32
static const uint8_t EPD_BUSY = 4;  // to EPD BUSY
static const uint8_t EPD_CS = 5;    // to EPD CS
static const uint8_t EPD_RST = 19;  // to EPD RST
static const uint8_t EPD_DC = 21;   // to EPD DC
static const uint8_t EPD_SCK = 18;  // to EPD CLK
static const uint8_t EPD_MISO = 16; // Master-In Slave-Out not used, as no data from display
static const uint8_t EPD_MOSI = 23; // to EPD DIN
#define LED_BUILTIN 2

GxEPD2_7C < GxEPD2_565c, GxEPD2_565c::HEIGHT / 2 > display(GxEPD2_565c(/*CS=*/EPD_CS, /*DC=*/EPD_DC, /*RST=*/EPD_RST, /*BUSY=*/EPD_BUSY)); // Waveshare 5.65" 7-color

const char *ssid = SECRETS_SSID;
const char *password = SECRETS_PW;

const int httpPort = 80;
const int httpsPort = 443;

// note: the certificates have been moved to a separate header file, as R"CERT( destroys IDE Auto Format capability

const char *certificate_rawcontent = cert_DigiCert_TLS_RSA_SHA256_2020_CA1; // ok, should work until 2031-04-13 23:59:59
// const char* certificate_rawcontent = github_io_chain_pem_first;  // ok, should work until Tue, 21 Mar 2023 23:59:59 GMT
// const char* certificate_rawcontent = github_io_chain_pem_second;  // ok, should work until Tue, 21 Mar 2023 23:59:59 GMT
// const char* certificate_rawcontent = github_io_chain_pem_third;  // ok, should work until Tue, 21 Mar 2023 23:59:59 GMT

const char *host_rawcontent = "raw.githubusercontent.com";
const char* path_toPicFolder   = "/rhovious/7ColorEinkDeskCalendar/main/publicImages/";
const char *fp_rawcontent = "8F 0E 79 24 71 C5 A7 D2 A7 46 76 30 C1 3C B7 2A 13 B0 01 B2"; // as of 29.7.2022

// note that BMP bitmaps are drawn at physical position in physical orientation of the screen
//void showBitmapFrom_HTTP(const char* host, const char* path, const char* filename, int16_t x, int16_t y, bool with_color = true);
//void showBitmapFrom_HTTPS(const char* host, const char* path, const char* filename, const char* fingerprint, int16_t x, int16_t y, bool with_color = true,
//const char* certificate = certificate_rawcontent);

// draws BMP bitmap according to set orientation
void showBitmapFrom_HTTP_Buffered(const char *host, const char *path, const char *filename, int16_t x, int16_t y, bool with_color = true);
void showBitmapFrom_HTTPS_Buffered(const char *host, const char *path, const char *filename, const char *fingerprint, int16_t x, int16_t y, bool with_color = true,
                                   const char *certificate = certificate_rawcontent);

char timeCurrentYear[5];
//char timeCurrentDayNum[10];
char timeCurrentMonth[10];
char timeWeekDayName[10];

int timeCurrentDayOfWeekNum;
int timeCurrentDayofMonthNum;
int timeDaysInMonth;

int CurrentMinForSleep = 0, CurrentSecForSleep = 0;
long StartTime = 0;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~SETTINGS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
uint16_t mainBGColor = GxEPD_GREEN;
uint16_t complementToBGColor = GxEPD_BLACK;
uint16_t mainBranchColor = GxEPD_WHITE;
long SleepDuration = 30; // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -28800; //offset hours *60 *60
const int   daylightOffset_sec = 3600; //set to 3600 if your contry observes DST

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~SETUP~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("7 Color eInk Desk Calendar");

  display.init(115200); // default 10ms reset pulse, e.g. for bare panels with DESPI-C02
  //display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  display.setRotation(3);
#ifdef REMAP_SPI_FOR_WAVESHARE_ESP32_DRIVER_BOARD
  SPI.end(); // release standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  // SPI: void begin(int8_t sck=-1, int8_t miso=-1, int8_t mosi=-1, int8_t ss=-1);
  SPI.begin(13, 12, 14, 15); // map and init SPI pins SCK(13), MISO(12), MOSI(14), SS(15)
#endif

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

  setClock();
  drawBitmapsBuffered_7C();
  Serial.println("GxEPD2_WiFi_Example done");
  Serial.println("Start Sleep");
  BeginSleep();
}

void loop(void)
{
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~DRAW FUNCTIONS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void drawBitmapsBuffered_7C()
{

  Serial.println("drawBitmapsBuffered_7C running...");
  Serial.println("Pic 1");
  showBitmapFrom_HTTPS_Buffered(host_rawcontent, path_toPicFolder, "pic1_rotated.bmp", fp_rawcontent, 0, 0);
  delay(2000);

}
//#########################################################################################

static const uint16_t input_buffer_pixels = 800; // may affect performance

static const uint16_t max_row_width = 1872;     // for up to 7.8" display 1872x1404
static const uint16_t max_palette_pixels = 256; // for depth <= 8

uint8_t input_buffer[3 * input_buffer_pixels];        // up to depth 24
uint8_t output_row_mono_buffer[max_row_width / 8];    // buffer for at least one row of b/w bits
uint8_t output_row_color_buffer[max_row_width / 8];   // buffer for at least one row of color bits
uint8_t mono_palette_buffer[max_palette_pixels / 8];  // palette buffer for depth <= 8 b/w
uint8_t color_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 c/w
uint16_t rgb_palette_buffer[max_palette_pixels];      // palette buffer for depth <= 8 for buffered graphics, needed for 7-color display
//#########################################################################################

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
        }     // end line
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
//#########################################################################################

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
        }     // end line
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
//#########################################################################################

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
    //display.setRotation(3); //sets back to 3 after printing
    drawBitmapFrom_HTTPS_ToBuffer(host, path, filename, fingerprint, x, y, with_color, certificate);
    //displayText();
    drawTestColorBoxes();
    drawCalendar(50, 50);
  } while (display.nextPage());
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~OTHER FUNCTIONS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void InitialiseDisplay()
{
}
//#########################################################################################

void BeginSleep()
{
  display.powerOff();
  long SleepTimer = (SleepDuration * 60 - ((CurrentMinForSleep % SleepDuration) * 60 + CurrentSecForSleep)); // Some ESP32 are too fast to maintain accurate time
  esp_sleep_enable_timer_wakeup((SleepTimer + 20) * 1000000LL);                              // Added extra 20-secs of sleep to allow for slow ESP32 RTC timers
#ifdef BUILTIN_LED
  pinMode(BUILTIN_LED, INPUT); // If it's On, turn it off and some boards use GPIO-5 for SPI-SS, which remains low after screen use
  digitalWrite(BUILTIN_LED, HIGH);
#endif
  Serial.println("Entering " + String(SleepTimer) + "-secs of sleep time");
  Serial.println("Awake for : " + String((millis() - StartTime) / 1000.0, 3) + "-secs");
  Serial.println("Starting deep-sleep period...");
  esp_deep_sleep_start(); // Sleep for e.g. 30 minutes
}
//#########################################################################################
void drawString(int x, int y, String inputText, uint16_t textColor)
{
  display.setRotation(0); //rotates display for text printing
  display.setTextSize(1);
  display.setTextColor(textColor);
  display.setCursor(x, y); // start writing at this position
  display.print(inputText);
  display.setRotation(3); //sets back to 3 after printing
}
//#########################################################################################

//Displays text on screen other than the calendar
void displayText()
{

  String timeCurrentDayNumString = String(timeCurrentDayofMonthNum);

  display.setFont(&FreeSansBold24pt7b);
  drawString(10, 35, timeCurrentMonth, complementToBGColor);
  drawString(15, 65, timeCurrentDayNumString, complementToBGColor);

  display.setFont(&FreeMono12pt7b);
  drawString(10, 90, timeCurrentYear, complementToBGColor);
  drawString(50, 105, timeWeekDayName, complementToBGColor);

}
//#########################################################################################
void drawTestColorBoxes()
{
  display.fillRect(0, 550, 50, 50, GxEPD_RED);
  display.fillRect(50, 550, 50, 50, GxEPD_ORANGE);
  display.fillRect(100, 550, 50, 50, GxEPD_YELLOW);
  display.fillRect(150, 550, 50, 50, GxEPD_GREEN);
  display.fillRect(200, 550, 50, 50, GxEPD_BLUE);
  display.fillRect(250, 550, 50, 50, GxEPD_BLACK);
  display.fillRect(300, 550, 50, 50, GxEPD_WHITE);
  display.drawRect(0, 550, 350, 50, GxEPD_BLACK); //outline for the color boxes
}
//#########################################################################################

uint16_t selectRandomColor()
{
  int tempRandom = random(1, 6);// print a random number from 1 to 5
  uint16_t returnedColor;

  switch (tempRandom) {
    case 1:
      returnedColor =  GxEPD_RED;
      complementToBGColor = GxEPD_YELLOW;
      break;
    case 2:
      returnedColor =  GxEPD_ORANGE;
      complementToBGColor = GxEPD_BLUE;
      break;
    case 3:
      returnedColor =  GxEPD_YELLOW;
      complementToBGColor = GxEPD_ORANGE;
      break;
    case 4:
      returnedColor =  GxEPD_GREEN;
      complementToBGColor = GxEPD_YELLOW;
      break;
    case 5:
      returnedColor =  GxEPD_BLUE;
      complementToBGColor = GxEPD_WHITE;
      break;
    default:
      returnedColor =  GxEPD_WHITE;
      complementToBGColor = GxEPD_BLACK;
      break;
  }
  return returnedColor;
}

//#########################################################################################

uint16_t read16(WiFiClient &client)
{
  // BMP data is stored little-endian, same as Arduino.
  uint16_t result;
  ((uint8_t *)&result)[0] = client.read(); // LSB
  ((uint8_t *)&result)[1] = client.read(); // MSB
  return result;
}
//#########################################################################################

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

//#########################################################################################

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
//#########################################################################################

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
//#########################################################################################

int getDaysInMonth(String Month)
{
  if (Month == "January" || Month == "March" || Month == "May" || Month == "July" || Month == "August" || Month == "October" || Month == "December") {
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
//#########################################################################################
// Set time via NTP, as required for x.509 validation
void setClock()
{
  //configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.print("Waiting for NTP time sync: ");
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
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

  timeCurrentDayOfWeekNum = timeinfo.tm_wday;// days since Sunday 0-6
  Serial.println(timeCurrentDayOfWeekNum);

  timeCurrentDayofMonthNum = timeinfo.tm_mday;/**< day of the month - [ 1 to 31 ] */
  Serial.println(timeCurrentDayofMonthNum);


  Serial.println("####### TIME #######");
  Serial.println();

}

//#########################################################################################

int CalendarYOffset = 200;
// draws calendar
void drawCalendar(int x, int y)
{
  display.setRotation(0); //rotates display for text printing

  String stryear = timeCurrentYear;
  Serial.println("printing year to screen" + stryear);
  display.setTextColor(GxEPD_BLACK);
  display.setFont();
  display.setCursor(8, 4);
  display.print(stryear);

  String strmonth = timeCurrentMonth;
  Serial.println("printing month to screen " + strmonth);
  int dow = timeCurrentDayOfWeekNum - 1;

  int today = timeCurrentDayofMonthNum;
  int daysinmonth = timeDaysInMonth;
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSansBold9pt7b);
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
  y = 24 + CalendarYOffset;

  for (int i = 0 ; i < 7; i++)
  {
    Serial.println("printing DOW to screen " + weekDayList[i]);
    x = 4 + i * (display.width() - 8) / 7;
    display.setTextColor(GxEPD_BLACK);
    display.setFont();
    display.getTextBounds(String(weekDayList[i]), 0, 0, &fx, &fy, &w, &h);
    display.setCursor(x + (display.width() / 7 - w) / 2, y - 8);
    display.print(String(weekDayList[i]));

  }
  display.drawLine(0, 24 + CalendarYOffset, display.width(), 24 + CalendarYOffset, GxEPD_BLACK);
  y = 32 + CalendarYOffset;
  while (curday < daysinmonth)
  {
    for (int i = 0; i < 7; i++)
    {
      x = 4 + i * (display.width() - 8) / 7;
      if (curday >= 1 && curday <= daysinmonth)
      {
        display.setCursor(x, y);
        display.setTextColor(GxEPD_BLACK);
        display.setFont(&FreeSans9pt7b);
        //if(iotd.isWeekday(i))
        //  display.setFont(&FreeSansBold9pt7b);
        int16_t fx, fy;
        uint16_t w, h;
        String strday = String(curday);
        if (curday == today)
        {
          display.setFont(&FreeSansBold9pt7b);
          //display.fillCircle(x + (display.width()-8)/7/2, y, 8, EPD_RED);
          display.setTextColor(GxEPD_RED);
        }
        display.getTextBounds(strday.c_str(), 0, 0, &fx, &fy, &w, &h);
        display.setCursor(x + (display.width() - 8) / 7 / 2 - w / 2 - fx, y + h / 2);
        display.print(curday);
      }
      curday++;
    }
    y += 16;
  }
  display.setRotation(3); //rotates back at end
}
