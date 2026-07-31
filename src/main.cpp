#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// Kart üzerindeki dahili amfi pini
#define SPEAKER_PIN 26

// Sizden Alınan Donanımsal Dokunmatik Kalibrasyon Verisi (Milimetrik Hassasiyet)
uint16_t calData[8] = {3842, 3882, 3847, 174, 378, 3803, 374, 145};

// C4 - C5 Notalarının Temel Frekansları (Hz)
const int BASE_FREQS[] = {262, 294, 330, 349, 392, 440, 494, 523};
const char* NOTE_NAMES[] = {"DO", "RE", "MI", "FA", "SOL", "LA", "SI", "DO+"};

// Tuş Görsel Renkleri
const uint16_t KEY_COLORS[] = {
  TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN,
  TFT_CYAN, TFT_BLUE, TFT_MAGENTA, TFT_PINK
};

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796  _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_XPT2046 _touch_instance;

public:
  LGFX() {
    // 1. SPI Otobüsü
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = HSPI_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;
      cfg.pin_sclk = 14;
      cfg.pin_mosi = 13;
      cfg.pin_miso = 12;
      cfg.pin_dc   = 2;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    // 2. ST7796S Ekran
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = 15;
      cfg.pin_rst          = -1;
      cfg.pin_busy         = -1;
      cfg.panel_width      = 320;
      cfg.panel_height     = 480;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.readable         = false;
      cfg.invert           = false;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = true;
      _panel_instance.config(cfg);
    }
    // 3. Arka Aydınlatma
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = 27;
      cfg.invert = false;
      cfg.freq   = 44100;
      cfg.pwm_channel = 7;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }
    // 4. Dokunmatik (XPT2046)
    {
      auto cfg = _touch_instance.config();
      cfg.x_min      = 0;
      cfg.x_max      = 4095;
      cfg.y_min      = 0;
      cfg.y_max      = 4095;
      cfg.pin_int    = -1;
      cfg.bus_shared = true;
      cfg.spi_host   = HSPI_HOST;
      cfg.pin_sclk   = 25;
      cfg.pin_mosi   = 32;
      cfg.pin_miso   = 39;
      cfg.pin_cs     = 33;
      cfg.freq       = 2500000;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }
    setPanel(&_panel_instance);
  }
};

LGFX lcd;

int keyWidth;
int keyHeight;
int currentOctave = 0; // -1: Pes, 0: Normal, 1: Tiz

enum InstrumentMode { MODE_ORGAN, MODE_SYNTH, MODE_PIANO };
InstrumentMode currentMode = MODE_ORGAN;

unsigned long noteStartTime = 0;

void updateHeader() {
  lcd.fillRect(0, 0, lcd.width(), 50, TFT_DARKGREY);
  lcd.drawRect(0, 0, lcd.width(), 50, TFT_WHITE);

  // OCT-
  lcd.fillRect(10, 8, 60, 34, TFT_NAVY);
  lcd.drawRect(10, 8, 60, 34, TFT_WHITE);
  lcd.setTextColor(TFT_WHITE, TFT_NAVY);
  lcd.setTextSize(2);
  lcd.setCursor(22, 17);
  lcd.println("OCT-");

  // OCT+
  lcd.fillRect(80, 8, 60, 34, TFT_NAVY);
  lcd.drawRect(80, 8, 60, 34, TFT_WHITE);
  lcd.setTextColor(TFT_WHITE, TFT_NAVY);
  lcd.setTextSize(2);
  lcd.setCursor(92, 17);
  lcd.println("OCT+");

  // Oktav Yazısı
  lcd.setTextColor(TFT_YELLOW, TFT_DARKGREY);
  lcd.setTextSize(2);
  lcd.setCursor(155, 17);
  if (currentOctave == -1) lcd.print("PES   ");
  else if (currentOctave == 0) lcd.print("NOR   ");
  else if (currentOctave == 1) lcd.print("TIZ   ");

  // Enstrüman Modu
  uint16_t btnColor = TFT_DARKGREEN;
  const char* modeText = "ORGAN";

  if (currentMode == MODE_SYNTH) {
    btnColor = TFT_PURPLE;
    modeText = "SYNTH";
  } else if (currentMode == MODE_PIANO) {
    btnColor = TFT_MAROON;
    modeText = "PIANO";
  }

  lcd.fillRect(340, 8, 130, 34, btnColor);
  lcd.drawRect(340, 8, 130, 34, TFT_WHITE);
  lcd.setTextColor(TFT_WHITE, btnColor);
  lcd.setTextSize(2);
  lcd.setCursor(355, 17);
  lcd.println(modeText);
}

void drawOrganUI() {
  lcd.fillScreen(TFT_BLACK);
  updateHeader();

  keyWidth = lcd.width() / 8; // 60px
  keyHeight = lcd.height() - 55;

  for (int i = 0; i < 8; i++) {
    int x = i * keyWidth;
    int y = 53;

    lcd.fillRect(x + 2, y, keyWidth - 4, keyHeight - 4, KEY_COLORS[i]);
    lcd.drawRect(x + 2, y, keyWidth - 4, keyHeight - 4, TFT_WHITE);

    lcd.setTextColor(TFT_BLACK, KEY_COLORS[i]);
    lcd.setTextSize(2);
    lcd.setCursor(x + 12, y + keyHeight - 40);
    lcd.println(NOTE_NAMES[i]);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(SPEAKER_PIN, OUTPUT);

  lcd.init();
  lcd.setRotation(1); 
  lcd.setBrightness(160);

  // DONANIMSAL KALİBRASYON YÜKLENİYOR
  lcd.setTouchCalibrate(calData);

  drawOrganUI();
}

uint16_t touchX = 0, touchY = 0;
int activeKey = -1;
unsigned long lastBtnTouch = 0;

void loop() {
  if (lcd.getTouch(&touchX, &touchY)) {
    
    // Üst Buton Paneli (Y < 50)
    if (touchY < 50) {
      if (millis() - lastBtnTouch > 300) {
        // OCT-
        if (touchX >= 10 && touchX <= 70) {
          if (currentOctave > -1) currentOctave--;
          updateHeader();
          tone(SPEAKER_PIN, 150, 60);
        }
        // OCT+
        else if (touchX >= 80 && touchX <= 140) {
          if (currentOctave < 1) currentOctave++;
          updateHeader();
          tone(SPEAKER_PIN, 600, 60);
        }
        // Enstrüman Modu Değiştirme
        else if (touchX >= 340 && touchX <= 470) {
          if (currentMode == MODE_ORGAN) currentMode = MODE_SYNTH;
          else if (currentMode == MODE_SYNTH) currentMode = MODE_PIANO;
          else currentMode = MODE_ORGAN;

          updateHeader();
          tone(SPEAKER_PIN, 800, 60);
        }
        lastBtnTouch = millis();
      }
    } 
    // Piyano Tuşları (Y >= 53)
    else if (touchY >= 53) {
      int keyIndex = touchX / keyWidth;

      if (keyIndex >= 0 && keyIndex < 8) {
        int targetFreq = BASE_FREQS[keyIndex];

        // Oktav Modifikasyonu
        if (currentOctave == -1) targetFreq /= 2;
        else if (currentOctave == 1) targetFreq *= 2;

        // Tuşa Ilk Vuruş
        if (keyIndex != activeKey) {
          activeKey = keyIndex;
          noteStartTime = millis();

          int x = keyIndex * keyWidth;
          lcd.fillRect(x + 2, 53, keyWidth - 4, keyHeight - 4, TFT_WHITE);
          lcd.setTextColor(TFT_BLACK, TFT_WHITE);
          lcd.setTextSize(2);
          lcd.setCursor(x + 12, 53 + keyHeight - 40);
          lcd.println(NOTE_NAMES[keyIndex]);
        }

        // Enstrüman Ses Karakterleri
        if (currentMode == MODE_ORGAN) {
          tone(SPEAKER_PIN, targetFreq);
        } 
        else if (currentMode == MODE_SYNTH) {
          int vibrato = sin(millis() / 18.0) * 9; 
          tone(SPEAKER_PIN, targetFreq + vibrato);
        } 
        else if (currentMode == MODE_PIANO) {
          unsigned long elapsed = millis() - noteStartTime;
          if (elapsed < 450) { // Akustik Sönümlenme Zarfı
            int pitchDecay = (450 - elapsed) / 80;
            tone(SPEAKER_PIN, targetFreq + pitchDecay);
          } else {
            noTone(SPEAKER_PIN); // Sütun ısınmasını önleyen otomatik susturma
          }
        }
      }
    }
  } else {
    // Parmağı Ekrandan Çekince Sesi Kes
    if (activeKey != -1) {
      noTone(SPEAKER_PIN);

      int x = activeKey * keyWidth;
      lcd.fillRect(x + 2, 53, keyWidth - 4, keyHeight - 4, KEY_COLORS[activeKey]);
      lcd.drawRect(x + 2, 53, keyWidth - 4, keyHeight - 4, TFT_WHITE);
      lcd.setTextColor(TFT_BLACK, KEY_COLORS[activeKey]);
      lcd.setTextSize(2);
      lcd.setCursor(x + 12, 53 + keyHeight - 40);
      lcd.println(NOTE_NAMES[activeKey]);

      activeKey = -1;
    }
  }

  delay(10);
}