#include <Arduino.h>
#include "HardwareConfig.h"
#include "TimezoneMapper.h"
#include "WordClockApp.h"
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <time.h>
#include "esp_sntp.h"
#include <esp_sleep.h>
#include <string.h>
#include <math.h>
#include <Wire.h>
#include <driver/gpio.h>

#include "esp_wifi.h"
#if __has_include("esp_eap_client.h")
  #include "esp_eap_client.h"
#else
  #include "esp_wpa2.h"
#endif

#include <ElegantOTA.h>

#include "wordclock_font.h"   // contains wordclock_woff2 + len
#include "page_html.h"        // contains PAGE_HTML PROGMEM

// -------------------- Hardware --------------------
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// -------------------- Persistente Settings --------------------
enum FadeMode : uint8_t { FADE_OFF = 0, FADE_2PHASE = 1, FADE_CROSS = 2, FADE_WIPE = 3 };

enum NightModeBehavior : uint8_t { NM_DOTS_ONLY = 0, NM_HALF_HOUR_ONLY = 1 , NM_NOTHING = 2 };

static const uint32_t CFG_MAGIC   = 0x57434C4B; // 'WCLK'
static const uint16_t CFG_VERSION = 10;

struct Settings {
  uint32_t magic;
  uint16_t version;

  // Basic
  uint8_t  maxBrightness;         // 1..255
  uint8_t  disableAutoBrightness; // 0/1 (0 = AutoBrightness AN)
  uint8_t  fadeMode;              // FadeMode
  uint16_t fadeMs;                // 0..2000ms (step 100)
  uint32_t colorM;                // 0xRRGGBB
  uint32_t colorH;                // 0xRRGGBB
  char     tzName[32];            // IANA

  // Nightmode (Basic)
  uint8_t  enableNightmode;       // 0/1
  uint8_t  nightBehavior;         // NightModeBehavior
  uint16_t nightStartMin;         // minutes since 00:00 (0..1439)
  uint16_t nightEndMin;           // minutes since 00:00 (0..1439)

  // Advanced
  uint32_t bootColor;             // SETUP (Portal/NoCreds)
  uint16_t bootFadeMs;

  uint32_t connectColor;          // SETUP (connecting)
  uint16_t connectPulseMs;        // pulse period ms

  uint8_t  ntpSyncHours;
  uint8_t  connectionTimeoutSec;
  uint8_t  enableMinuteDots;
  uint16_t stepMs;                // animation step

  // AutoBrightness tuning (Advanced)
  uint16_t luxCheckIntervalSec;   // 1..60
  uint8_t  hysteresisDelta;       // 1..50 (used 1:1 for brightness + nightmode hysteresis)
  uint32_t portalInactiveMaxSec;  // 30..3600

  float luxMin;
  float luxMax;

  // Nightmode tuning (Advanced)
  float    nightLuxThreshold;     // lux threshold for nightmode activation
};

static Settings cfg;
static Preferences prefsCfg;
static Preferences prefsWifi;

// -------------------- WiFi Credentials (PSK vs Enterprise) --------------------
enum WifiMode : uint8_t { WIFI_MODE_PSK = 0, WIFI_MODE_ENTERPRISE = 1 };

struct WifiCreds {
  WifiMode mode = WIFI_MODE_PSK;
  String ssid;

  // PSK
  String psk;

  // Enterprise (PEAP/MSCHAPv2 typical)
  String eapIdentity; // outer identity (optional; fallback to eapUser)
  String eapUser;     // inner username
  String eapPass;     // password
};

static bool loadWifiCredsEx(WifiCreds &out) {
  prefsWifi.begin("wifi", true);

  out.mode = (WifiMode)prefsWifi.getUChar("mode", (uint8_t)WIFI_MODE_PSK);
  out.ssid = prefsWifi.getString("ssid", "");

  // Keep your old key names for compatibility:
  out.psk        = prefsWifi.getString("pass", "");
  out.eapIdentity= prefsWifi.getString("eap_id", "");
  out.eapUser    = prefsWifi.getString("eap_user", "");
  out.eapPass    = prefsWifi.getString("eap_pass", "");

  prefsWifi.end();
  return out.ssid.length() > 0;
}

static void saveWifiCredsEx(const WifiCreds &in) {
  prefsWifi.begin("wifi", false);

  prefsWifi.putUChar("mode", (uint8_t)in.mode);
  prefsWifi.putString("ssid", in.ssid);

  // PSK
  prefsWifi.putString("pass", in.psk);

  // Enterprise
  prefsWifi.putString("eap_id", in.eapIdentity);
  prefsWifi.putString("eap_user", in.eapUser);
  prefsWifi.putString("eap_pass", in.eapPass);

  prefsWifi.end();
}

// -------------------- Portal globals --------------------
static WebServer server(80);
static DNSServer dnsServer;

static bool portalActive = false;
static uint32_t portalLastActivityMs = 0;

static bool portalDemoActive = false;
static const uint32_t PORTAL_DEMO_IDLE_MS = 25000;

static Settings previewCfg;   // nur RAM, nicht speichern
static bool previewDirty = false;

// Demo time
static uint8_t demoHour = 19;
static uint8_t demoMin  = 25;
static uint32_t nextDemoTickMs = 0;

// Portal exit ohne Reboot
enum PortalExitReason : uint8_t { PEXIT_NONE=0, PEXIT_SAVED_OK=1 };
static volatile PortalExitReason g_portalExit = PEXIT_NONE;
static uint32_t g_portalExitAtMs = 0;

enum SaveJobState : uint8_t { SJ_IDLE=0, SJ_CONNECTING=1, SJ_OK=2, SJ_FAIL=3 };

static SaveJobState g_saveState = SJ_IDLE;
static uint32_t g_saveStartedMs = 0;
static String g_saveMsg;

static WifiCreds g_pendingCreds;
static bool g_pendingCredsValid = false;

static uint32_t g_saveTimeoutMs = 20000;

// -------------------- Framebuffer --------------------
static uint32_t g_current[LED_COUNT];
static bool g_hasCurrent = false;

// Brightness State
static uint8_t g_briNow    = 255;
static uint8_t g_briTarget = 255;

// -------------------- Connection / NTP Status --------------------
static uint8_t wifiFailCount = 0;
static bool wifiFailOverlay = false;
static time_t lastNTPSyncEpoch = 0;
static bool g_initialSyncDone = false;

static volatile bool g_ntpSynced = false;

static void timeSyncNotificationCallback(struct timeval *tv) {
  g_ntpSynced = true;
  lastNTPSyncEpoch = time(nullptr);
}

// -------------------- BH1750 Auto-Brightness --------------------
static bool     g_bhOk = false;
static uint8_t  g_bhAddr = 0x23;

static float    g_luxLast = -1.0f; 
static uint32_t g_nextLuxCheckMs = 0;

static bool g_nightLatched = false;
static bool lastNightShown = false;
static bool nightToggled = false;

// -------------------- Portal Lux (live) --------------------
static bool     g_portalLuxOk = false;
static float    g_portalLuxLast = -1.0f;
static uint32_t g_portalNextLuxPollMs = 0;
static const uint32_t PORTAL_LUX_POLL_MS = 2000;

// -------------------- Forward declarations --------------------
static void markPortalClientSeen();

// -------------------- Utils --------------------
static bool strToU16(const String& s, uint16_t &out) {
  char *endptr = nullptr;
  long v = strtol(s.c_str(), &endptr, 10);
  if (endptr == s.c_str() || *endptr != '\0') return false;
  if (v < 0) v = 0;
  if (v > 65535) v = 65535;
  out = (uint16_t)v;
  return true;
}
static bool strToU8(const String& s, uint8_t &out) {
  char *endptr = nullptr;
  long v = strtol(s.c_str(), &endptr, 10);
  if (endptr == s.c_str() || *endptr != '\0') return false;
  if (v < 0) v = 0;
  if (v > 255) v = 255;
  out = (uint8_t)v;
  return true;
}
static bool strToFloat(const String& s, float &out) {
  char *endptr = nullptr;
  float v = strtof(s.c_str(), &endptr);
  if (endptr == s.c_str() || *endptr != '\0') return false;
  out = v;
  return true;
}
static bool parseTimeHHMM(const String& s, uint16_t &outMin) {
  int colon = s.indexOf(':');
  if (colon < 0) return false;

  String sh = s.substring(0, colon);
  String sm = s.substring(colon + 1);

  if (sh.length() == 0 || sm.length() == 0) return false;

  uint16_t hhRaw, mmRaw;
  if (!strToU16(sh, hhRaw)) return false;
  if (!strToU16(sm, mmRaw)) return false;

  if (hhRaw > 23 || mmRaw > 59) return false;

  outMin = (uint16_t)(hhRaw * 60 + mmRaw);
  return true;
}
static String formatTimeHHMM(uint16_t minOfDay) {
  minOfDay %= 1440;
  uint16_t hh = minOfDay / 60;
  uint16_t mm = minOfDay % 60;
  char buf[6];
  snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)hh, (unsigned)mm);
  return String(buf);
}
static void safeCopy(char *dst, size_t dstSize, const String& src) {
  if (!dst || dstSize == 0) return;
  size_t n = src.length();
  if (n >= dstSize) n = dstSize - 1;
  memcpy(dst, src.c_str(), n);
  dst[n] = '\0';
}
static uint32_t parseColorHex(String s) {
  s.trim();
  if (s.startsWith("#")) s = s.substring(1);
  if (s.length() < 6) return 0;
  char buf[9] = {0};
  s.substring(0, 6).toCharArray(buf, sizeof(buf));
  uint32_t v = (uint32_t)strtoul(buf, nullptr, 16);
  return v & 0xFFFFFF;
}
static String colorToHex(uint32_t c) {
  char out[8];
  snprintf(out, sizeof(out), "%06lX", (unsigned long)(c & 0xFFFFFF));
  return String(out);
}
static inline float easeInOut(float t) {
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  return t * t * (3.0f - 2.0f * t);
}
static inline uint8_t lerpU8(uint8_t a, uint8_t b, float t) {
  int v = (int)lroundf((float)a + ((float)b - (float)a) * t);
  if (v < 0) v = 0;
  if (v > 255) v = 255;
  return (uint8_t)v;
}
static inline float srgb8_to_linear(uint8_t v) {
  float x = v / 255.0f;
  return powf(x, 2.2f);
}
static inline uint8_t linear_to_srgb8(float x) {
  if (x < 0.0f) x = 0.0f;
  if (x > 1.0f) x = 1.0f;
  float s = powf(x, 1.0f / 2.2f);
  return (uint8_t)lroundf(s * 255.0f);
}
static uint32_t lerpColorGamma(uint32_t c0, uint32_t c1, float t) {
  uint8_t r0=(c0>>16)&255, g0=(c0>>8)&255, b0=c0&255;
  uint8_t r1=(c1>>16)&255, g1=(c1>>8)&255, b1=c1&255;

  float R0 = srgb8_to_linear(r0), G0 = srgb8_to_linear(g0), B0 = srgb8_to_linear(b0);
  float R1 = srgb8_to_linear(r1), G1 = srgb8_to_linear(g1), B1 = srgb8_to_linear(b1);

  float R = R0 + (R1 - R0) * t;
  float G = G0 + (G1 - G0) * t;
  float B = B0 + (B1 - B0) * t;

  return strip.Color(linear_to_srgb8(R), linear_to_srgb8(G), linear_to_srgb8(B));
}

// --- Perceived normalization ---
static uint32_t normalizePerceived(uint32_t rgb, float targetLuma = 0.35f) {
  float r = ((rgb >> 16) & 0xFF) / 255.0f;
  float g = ((rgb >> 8)  & 0xFF) / 255.0f;
  float b = ( rgb        & 0xFF) / 255.0f;

  float luma = 0.2126f * r + 0.7152f * g + 0.0722f * b;
  if (luma < 0.0001f) return rgb;

  float k = targetLuma / luma;
  if (k > 1.0f) k = 1.0f;

  r = fminf(1.0f, r * k);
  g = fminf(1.0f, g * k);
  b = fminf(1.0f, b * k);

  uint32_t rr = (uint32_t)lroundf(r * 255.0f);
  uint32_t gg = (uint32_t)lroundf(g * 255.0f);
  uint32_t bb = (uint32_t)lroundf(b * 255.0f);

  return (rr << 16) | (gg << 8) | bb;
}
static uint32_t rgbToNeoComp(uint32_t rgb) {
  rgb = normalizePerceived(rgb, 0.35f);
  return strip.Color((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

static inline uint16_t snapFadeMs(uint16_t v) {
  if (v > 2000) v = 2000;
  v = (uint16_t)((v / 100) * 100);
  return v;
}
static inline uint8_t scaleChanMin1(uint8_t v, uint8_t bri) {
  if (v == 0 || bri == 0) return 0;
  uint16_t x = (uint16_t)v * (uint16_t)bri;
  uint8_t out = (uint8_t)((x + 127) / 255);
  if (out == 0) out = 1;
  return out;
}
static inline uint32_t applyBrightnessMin1(uint32_t c, uint8_t bri) {
  uint8_t r = (c >> 16) & 0xFF;
  uint8_t g = (c >> 8)  & 0xFF;
  uint8_t b =  c        & 0xFF;

  r = scaleChanMin1(r, bri);
  g = scaleChanMin1(g, bri);
  b = scaleChanMin1(b, bri);

  return strip.Color(r, g, b);
}

// ---------- Clamps (now works for cfg and previewCfg) ----------
static void applyClamps(Settings& s) {
  s.fadeMs = snapFadeMs(s.fadeMs);
  if (s.maxBrightness < 1) s.maxBrightness = 1;

  if (s.luxCheckIntervalSec < 1) s.luxCheckIntervalSec = 1;
  if (s.luxCheckIntervalSec > 60) s.luxCheckIntervalSec = 60;

  if (s.hysteresisDelta < 1) s.hysteresisDelta = 1;
  if (s.hysteresisDelta > 50) s.hysteresisDelta = 50;

  if (s.portalInactiveMaxSec < 30UL) s.portalInactiveMaxSec = 30UL;
  if (s.portalInactiveMaxSec > 3600UL) s.portalInactiveMaxSec = 3600UL;

  if (s.stepMs < 5) s.stepMs = 5;

  if (s.nightBehavior > NM_NOTHING) s.nightBehavior = NM_DOTS_ONLY;
  if (s.nightStartMin > 1439) s.nightStartMin = 0;
  if (s.nightEndMin > 1439) s.nightEndMin = 0;

  if (s.nightLuxThreshold < 0.0f) s.nightLuxThreshold = 0.0f;
  if (s.nightLuxThreshold > 500.0f) s.nightLuxThreshold = 500.0f;
}
static void applyRuntimeClamps() { applyClamps(cfg); }

// -------------------- Timezone --------------------
static void applyTimezone() {
  const char* posix = tzPosixFromIana(cfg.tzName);
  setenv("TZ", posix, 1);
  tzset();
}

// -------------------- Settings load/save --------------------
static void setDefaultSettings() {
  memset(&cfg, 0, sizeof(cfg));
  cfg.magic = CFG_MAGIC;
  cfg.version = CFG_VERSION;

  cfg.maxBrightness = 50;
  cfg.disableAutoBrightness = 0;

  cfg.fadeMode = FADE_2PHASE;
  cfg.fadeMs = 500;

  cfg.colorM = 0x808000;
  cfg.colorH = 0xFF0000;

  strncpy(cfg.tzName, "Europe/Berlin", sizeof(cfg.tzName) - 1);

  // Nightmode defaults
  cfg.enableNightmode = 0;
  cfg.nightBehavior   = NM_DOTS_ONLY;
  cfg.nightStartMin   = 22 * 60;
  cfg.nightEndMin     = 7 * 60;

  cfg.bootColor = 0x0000FF;
  cfg.bootFadeMs = 200;

  cfg.connectColor = 0x00FF00;
  cfg.connectPulseMs = 1200;

  cfg.ntpSyncHours = 1;
  cfg.connectionTimeoutSec = 20;
  cfg.enableMinuteDots = 1;

  cfg.stepMs = 20;

  cfg.luxCheckIntervalSec  = 5;
  cfg.hysteresisDelta      = 2;
  cfg.portalInactiveMaxSec = 3UL * 60UL;

  cfg.nightLuxThreshold = 3.0f;
  cfg.luxMin = 2.0f;
  cfg.luxMax = 800.0f;
}

static bool loadSettings() {
  prefsCfg.begin("wordclk", true);
  size_t len = prefsCfg.getBytesLength("cfg");
  if (len != sizeof(Settings)) {
    prefsCfg.end();
    return false;
  }
  Settings tmp;
  prefsCfg.getBytes("cfg", &tmp, sizeof(tmp));
  prefsCfg.end();

  if (tmp.magic != CFG_MAGIC || tmp.version != CFG_VERSION) return false;
  cfg = tmp;
  return true;
}

static void saveSettings() {
  cfg.magic = CFG_MAGIC;
  cfg.version = CFG_VERSION;
  prefsCfg.begin("wordclk", false);
  prefsCfg.putBytes("cfg", &cfg, sizeof(cfg));
  prefsCfg.end();
}

// -------------------- WiFi helpers --------------------
static void wifiOff() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(30);
}
static void noteWifiAttemptResult(bool ok) {
  if (ok) {
    wifiFailCount = 0;
    wifiFailOverlay = false;
  } else {
    wifiFailCount++;
    if (wifiFailCount >= 4) wifiFailOverlay = true;
  }
}

// -------------------- NTP --------------------
static void initNTP() {
  const char* posix = tzPosixFromIana(cfg.tzName);

  setenv("TZ", posix, 1);
  tzset();

  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, (char*)"pool.ntp.org");
  sntp_setservername(1, (char*)"time.nist.gov");
  sntp_setservername(2, (char*)"time.google.com");

  sntp_set_time_sync_notification_cb(timeSyncNotificationCallback);

  // Intervall in ms, Minimum laut SNTP beachten
  sntp_set_sync_interval((uint32_t)cfg.ntpSyncHours * 3600UL * 1000UL);

  sntp_init();
}

static bool syncTimeNTP(uint32_t timeoutMs = 15000) {
  g_ntpSynced = false;
  sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);

  sntp_restart();

  uint32_t start = millis();

  while ((millis() - start) < timeoutMs) {
    sntp_sync_status_t st = sntp_get_sync_status();

    if (g_ntpSynced || st == SNTP_SYNC_STATUS_COMPLETED) {
      lastNTPSyncEpoch = time(nullptr);
      return true;
    }

    delay(200);
  }
  return false;
}

static bool needsNtpSync() {
  if (cfg.ntpSyncHours == 0) return false;
  if (!g_initialSyncDone) return true;

  time_t now = time(nullptr);
  const time_t interval = (time_t)cfg.ntpSyncHours * 3600;
  return (now - lastNTPSyncEpoch) >= interval;
}

// -------------------- WordClock helpers --------------------
static inline void setIdxBuf(uint32_t* buf, uint16_t idx, uint32_t c) {
  if (idx < LED_COUNT) buf[idx] = c;
}
static void lightRangeBuf(uint32_t* buf, uint8_t row, uint8_t colStart, uint8_t len, uint32_t c) {
  uint16_t base = (uint16_t)row * 10;
  for (uint8_t k = 0; k < len; k++) {
    uint16_t idx = base + (uint16_t)(colStart + k);
    if (idx < LED_COUNT) buf[idx] = c;
  }
}

static inline bool btnPressed(int pin) {
  return digitalRead(pin) == LOW;
}

static bool waitReleaseDebounced(int pin, uint16_t stableMs = 25) {
  uint32_t lastChange = millis();
  bool last = btnPressed(pin);

  while (true) {
    bool now = btnPressed(pin);
    if (now != last) {
      last = now;
      lastChange = millis();
    }
    if ((millis() - lastChange) >= stableMs) {
      return !now; // true if released
    }
    delay(1);
  }
}

// -------------------- Wort-Mappings --------------------
static void W_FUENF(uint32_t* b, uint32_t c)   { lightRangeBuf(b, 0, 1, 4, c); }
static void W_ZEHN(uint32_t* b, uint32_t c)    { lightRangeBuf(b, 0, 5, 4, c); }
static void W_VIERTEL(uint32_t* b, uint32_t c) { lightRangeBuf(b, 1, 3, 7, c); }
static void W_ZWANZIG(uint32_t* b, uint32_t c) { lightRangeBuf(b, 2, 0, 7, c); }

static void W_SET(uint32_t* b, uint32_t c)     { lightRangeBuf(b, 2, 7, 3, c); }
static void W_UP(uint32_t* b, uint32_t c)      { lightRangeBuf(b, 3, 8, 2, c); }

static void W_VOR(uint32_t* b, uint32_t c)     { lightRangeBuf(b, 3, 0, 3, c); }
static void W_NACH(uint32_t* b, uint32_t c)    { lightRangeBuf(b, 3, 4, 4, c); }
static void W_HALB(uint32_t* b, uint32_t c)    { lightRangeBuf(b, 4, 0, 4, c); }
static void W_ZWOELF(uint32_t* b, uint32_t c)  { lightRangeBuf(b, 4, 5, 5, c); }
static void W_SIEBEN(uint32_t* b, uint32_t c)  { lightRangeBuf(b, 5, 0, 6, c); }
static void W_ZEHN_H(uint32_t* b, uint32_t c)  { lightRangeBuf(b, 5, 6, 4, c); }
static void W_NEUN(uint32_t* b, uint32_t c)    { lightRangeBuf(b, 6, 0, 4, c); }
static void W_DREI(uint32_t* b, uint32_t c)    { lightRangeBuf(b, 6, 4, 4, c); }
static void W_EINS(uint32_t* b, uint32_t c)    { lightRangeBuf(b, 6, 6, 4, c); }
static void W_EIN(uint32_t* b, uint32_t c)     { lightRangeBuf(b, 6, 6, 3, c); }
static void W_ACHT(uint32_t* b, uint32_t c)    { lightRangeBuf(b, 7, 0, 4, c); }
static void W_ELF(uint32_t* b, uint32_t c)     { lightRangeBuf(b, 7, 4, 3, c); }
static void W_FUENF_H(uint32_t* b, uint32_t c) { lightRangeBuf(b, 7, 6, 4, c); }
static void W_ZWEI(uint32_t* b, uint32_t c)    { lightRangeBuf(b, 8, 0, 4, c); }
static void W_SECHS(uint32_t* b, uint32_t c)   { lightRangeBuf(b, 8, 5, 5, c); }
static void W_VIER(uint32_t* b, uint32_t c)    { lightRangeBuf(b, 9, 1, 4, c); }
static void W_UHR(uint32_t* b, uint32_t c)     { lightRangeBuf(b, 9, 6, 3, c); }

static void lightHourWord(uint32_t* b, uint8_t hour12, uint8_t minute5, uint32_t c) {
  switch (hour12) {
    case 0:  W_ZWOELF(b, c); break;
    case 1:  if (minute5 == 0) W_EIN(b, c); else W_EINS(b, c); break;
    case 2:  W_ZWEI(b, c); break;
    case 3:  W_DREI(b, c); break;
    case 4:  W_VIER(b, c); break;
    case 5:  W_FUENF_H(b, c); break;
    case 6:  W_SECHS(b, c); break;
    case 7:  W_SIEBEN(b, c); break;
    case 8:  W_ACHT(b, c); break;
    case 9:  W_NEUN(b, c); break;
    case 10: W_ZEHN_H(b, c); break;
    case 11: W_ELF(b, c); break;
  }
}

static void drawSetupOverlay(uint32_t* frame, uint32_t rgb) {
  uint32_t c = rgbToNeoComp(rgb);
  W_SET(frame, c);
  W_UP(frame, c);
}

static void renderWordClockWith(const Settings& s, uint8_t hour24, uint8_t minute, uint32_t* out) {
  for (int i = 0; i < LED_COUNT; i++) out[i] = 0;

  if (s.enableMinuteDots) {
    const uint16_t corners[4] = { 0, 9, 90, 99 };
    uint8_t extra = minute % 5;
    uint32_t cDot = rgbToNeoComp(s.colorM);
    for (uint8_t i = 0; i < extra; i++) setIdxBuf(out, corners[i], cDot);
  }

  uint8_t minute5    = (minute / 5) * 5;
  uint8_t hour12     = hour24 % 12;
  uint8_t nextHour12 = (hour12 + 1) % 12;

  uint32_t cM = rgbToNeoComp(s.colorM);
  uint32_t cH = rgbToNeoComp(s.colorH);

  switch (minute5) {
    case 0:
      lightHourWord(out, hour12, minute5, cH);
      W_UHR(out, cM);
      break;
    case 5:
      W_FUENF(out, cM); W_NACH(out, cM);
      lightHourWord(out, hour12, minute5, cH);
      break;
    case 10:
      W_ZEHN(out, cM); W_NACH(out, cM);
      lightHourWord(out, hour12, minute5, cH);
      break;
    case 15:
      W_VIERTEL(out, cM); W_NACH(out, cM);
      lightHourWord(out, hour12, minute5, cH);
      break;
    case 20:
      W_ZWANZIG(out, cM); W_NACH(out, cM);
      lightHourWord(out, hour12, minute5, cH);
      break;
    case 25:
      W_FUENF(out, cM); W_VOR(out, cM); W_HALB(out, cM);
      lightHourWord(out, nextHour12, minute5, cH);
      break;
    case 30:
      W_HALB(out, cM);
      lightHourWord(out, nextHour12, minute5, cH);
      break;
    case 35:
      W_FUENF(out, cM); W_NACH(out, cM); W_HALB(out, cM);
      lightHourWord(out, nextHour12, minute5, cH);
      break;
    case 40:
      W_ZWANZIG(out, cM); W_VOR(out, cM);
      lightHourWord(out, nextHour12, minute5, cH);
      break;
    case 45:
      W_VIERTEL(out, cM); W_VOR(out, cM);
      lightHourWord(out, nextHour12, minute5, cH);
      break;
    case 50:
      W_ZEHN(out, cM); W_VOR(out, cM);
      lightHourWord(out, nextHour12, minute5, cH);
      break;
    case 55:
      W_FUENF(out, cM); W_VOR(out, cM);
      lightHourWord(out, nextHour12, minute5, cH);
      break;
  }
}

// -------------------- Fade Engine (mit Portal-Yield) --------------------
static void portalYield(uint16_t ms) {
  const uint32_t endAt = millis() + ms;
  while ((int32_t)(millis() - endAt) < 0) {
    if (portalActive) {
      dnsServer.processNextRequest();
      server.handleClient();
    }
    delay(1);
  }
}

// Forward for fadeBrightnessOnly
static void fadeBrightnessOnly(uint8_t briTo, uint16_t fadeMs, uint16_t stepMs);

static void showFrameInstant(const uint32_t* frameRaw, uint8_t briTo,
                             uint16_t baseMs, uint16_t stepMs) {
  for (int i = 0; i < LED_COUNT; i++) g_current[i] = frameRaw[i];
  g_hasCurrent = true;

  if (g_briNow < 1) g_briNow = briTo;
  for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, applyBrightnessMin1(g_current[i], g_briNow));
  strip.show();

  fadeBrightnessOnly(briTo, baseMs, stepMs);
}

static void fadeCross(const uint32_t* fromRaw, const uint32_t* toRaw,
                      uint16_t fadeMs, uint16_t stepMs,
                      uint8_t briFrom, uint8_t briTo) {
  if (fadeMs == 0) { showFrameInstant(toRaw, briTo, fadeMs, stepMs); return; }

  uint16_t step  = (stepMs < 1) ? 1 : stepMs;
  uint16_t steps = (uint16_t)max(1, (int)(fadeMs / step));

  for (uint16_t s = 1; s <= steps; s++) {
    float t = easeInOut((float)s / (float)steps);
    uint8_t bri = lerpU8(briFrom, briTo, t);
    g_briNow = bri;

    for (int i = 0; i < LED_COUNT; i++) {
      uint32_t raw = lerpColorGamma(fromRaw[i], toRaw[i], t);
      g_current[i] = raw;
      strip.setPixelColor(i, applyBrightnessMin1(raw, bri));
    }
    g_hasCurrent = true;
    strip.show();
    portalYield(step);
  }
}

static void fade2Phase(const uint32_t* fromFrame, const uint32_t* toFrame,
                       uint16_t fadeOutMs, uint16_t fadeInMs, uint16_t stepMs,
                       uint8_t briFrom, uint8_t briTo) {
  static bool offMask[LED_COUNT];
  static bool onMask[LED_COUNT];

  for (int i = 0; i < LED_COUNT; i++) {
    bool curOn  = (fromFrame[i] != 0);
    bool nextOn = (toFrame[i] != 0);
    offMask[i] = (curOn && !nextOn);
    onMask[i]  = (!curOn && nextOn);
  }

  uint16_t step = (stepMs < 1) ? 1 : stepMs;

  if (fadeOutMs > 0) {
    uint16_t steps = (uint16_t)max(1, (int)(fadeOutMs / step));
    for (uint16_t s = 1; s <= steps; s++) {
      float t = easeInOut((float)s / (float)steps);
      float tb = (float)s / (float)steps;
      uint8_t bri = lerpU8(briFrom, briTo, tb);
      g_briNow = bri;

      for (int i = 0; i < LED_COUNT; i++) {
        uint32_t raw = offMask[i] ? lerpColorGamma(fromFrame[i], 0, t) : fromFrame[i];
        g_current[i] = raw;
        strip.setPixelColor(i, applyBrightnessMin1(raw, bri));
      }
      g_hasCurrent = true;
      strip.show();
      portalYield(step);
    }
  }

  if (fadeInMs > 0) {
    uint16_t steps = (uint16_t)max(1, (int)(fadeInMs / step));
    for (uint16_t s = 1; s <= steps; s++) {
      float t = easeInOut((float)s / (float)steps);
      uint8_t bri = briTo;
      g_briNow = bri;

      for (int i = 0; i < LED_COUNT; i++) {
        uint32_t raw = onMask[i] ? lerpColorGamma(0, toFrame[i], t) : toFrame[i];
        g_current[i] = raw;
        strip.setPixelColor(i, applyBrightnessMin1(raw, bri));
      }
      g_hasCurrent = true;
      strip.show();
      portalYield(step);
    }
  }
}

static void fadeWipe(const uint32_t* fromFrame, const uint32_t* toFrame,
                     uint16_t wipeOutMs, uint16_t wipeInMs, uint16_t stepMs,
                     uint8_t briFrom, uint8_t briTo) {
  static bool offMask[LED_COUNT];
  static bool onMask[LED_COUNT];

  for (int i = 0; i < LED_COUNT; i++) {
    bool curOn  = (fromFrame[i] != 0);
    bool nextOn = (toFrame[i] != 0);
    offMask[i] = (curOn && !nextOn);
    onMask[i]  = (!curOn && nextOn);
  }

  uint16_t step = (stepMs < 1) ? 1 : stepMs;

  auto colOf = [](int idx) -> int { return idx % 10; };

  if (wipeOutMs > 0) {
    uint16_t steps = (uint16_t)max(1, (int)(wipeOutMs / step));

    const float fadeLen = 0.55f;
    const float delaySpan = 1.0f - fadeLen;
    const float delayStep = delaySpan / 9.0f;

    for (uint16_t s = 1; s <= steps; s++) {
      float tGlobal = (float)s / (float)steps;
      float tb = tGlobal;
      uint8_t bri = lerpU8(briFrom, briTo, tb);
      g_briNow = bri;

      for (int i = 0; i < LED_COUNT; i++) {
        uint32_t raw;
        if (offMask[i]) {
          int col = colOf(i);
          float delay = (float)(9 - col) * delayStep;
          float local = (tGlobal - delay) / fadeLen;
          raw = lerpColorGamma(fromFrame[i], 0, easeInOut(local));
        } else {
          raw = fromFrame[i];
        }
        g_current[i] = raw;
        strip.setPixelColor(i, applyBrightnessMin1(raw, bri));
      }

      g_hasCurrent = true;
      strip.show();
      portalYield(step);
    }
  }

  if (wipeInMs > 0) {
    uint16_t steps = (uint16_t)max(1, (int)(wipeInMs / step));

    const float fadeLen = 0.55f;
    const float delaySpan = 1.0f - fadeLen;
    const float delayStep = delaySpan / 9.0f;

    for (uint16_t s = 1; s <= steps; s++) {
      float tGlobal = (float)s / (float)steps;
      uint8_t bri = briTo;
      g_briNow = bri;

      for (int i = 0; i < LED_COUNT; i++) {
        uint32_t raw;
        if (onMask[i]) {
          int col = colOf(i);
          float delay = (float)col * delayStep;
          float local = (tGlobal - delay) / fadeLen;
          raw = lerpColorGamma(0, toFrame[i], easeInOut(local));
        } else {
          raw = toFrame[i];
        }
        g_current[i] = raw;
        strip.setPixelColor(i, applyBrightnessMin1(raw, bri));
      }

      g_hasCurrent = true;
      strip.show();
      portalYield(step);
    }
  }
}

static void transitionToFrame(const uint32_t* targetRaw, FadeMode mode,
                              uint16_t baseMs, uint16_t stepMs,
                              uint8_t briTo) {
  static uint32_t blackRaw[LED_COUNT] = {0};
  const uint32_t* fromRaw = g_hasCurrent ? g_current : blackRaw;

  uint8_t briFrom = g_hasCurrent ? g_briNow : briTo;

  if (mode == FADE_OFF || baseMs == 0) { 
    if (briTo != briFrom) {
      fadeBrightnessOnly(briTo, baseMs, stepMs);
    }
    showFrameInstant(targetRaw, briTo, baseMs, stepMs); return;
  }

  if (mode == FADE_CROSS) {
    fadeCross(fromRaw, targetRaw, baseMs, stepMs, briFrom, briTo);
  } else if (mode == FADE_WIPE) {
    uint16_t outMs = baseMs / 2;
    uint16_t inMs  = baseMs - outMs;
    fadeWipe(fromRaw, targetRaw, outMs, inMs, stepMs, briFrom, briTo);
  } else {
    uint16_t outMs = baseMs / 2;
    uint16_t inMs  = baseMs - outMs;
    fade2Phase(fromRaw, targetRaw, outMs, inMs, stepMs, briFrom, briTo);
  }
}

// -------------------- UX Helpers --------------------
static void showSetupSolidCrossFade(uint32_t rgb, uint16_t fadeMs, bool clear) {
  if (clear) {
    static uint32_t f[LED_COUNT];
    for (int i = 0; i < LED_COUNT; i++) f[i] = 0;
    drawSetupOverlay(f, rgb);
    transitionToFrame(f, FADE_CROSS, fadeMs, cfg.stepMs, g_briTarget);
  }else{
    drawSetupOverlay(g_current, rgb);
    transitionToFrame(g_current, FADE_CROSS, fadeMs, cfg.stepMs, g_briTarget);
  }
}

static void fadeToBlack(uint16_t fadeMs, uint16_t stepMs) {
  static uint32_t black[LED_COUNT] = {0};

  if (!g_hasCurrent) {
    showFrameInstant(black, 0, fadeMs, stepMs);
    return;
  }
  bool alreadyBlack = true;
  for (int i = 0; i < LED_COUNT; i++) { if (g_current[i] != 0) { alreadyBlack = false; break; } }
  if (alreadyBlack) return;

  fadeCross(g_current, black, fadeMs, stepMs, g_briNow, g_briNow);
}

// Brightness-only Fade
static void fadeBrightnessOnly(uint8_t briTo, uint16_t fadeMs, uint16_t stepMs) {
  if (!g_hasCurrent) { g_briNow = briTo; return; }
  if (fadeMs == 0 || briTo == g_briNow) { g_briNow = briTo; return; }

  uint16_t step  = (stepMs < 1) ? 1 : stepMs;
  uint16_t steps = (uint16_t)max(1, (int)(fadeMs / step));
  uint8_t briFrom = g_briNow;

  for (uint16_t s = 1; s <= steps; s++) {
    float t = easeInOut((float)s / (float)steps);
    uint8_t bri = lerpU8(briFrom, briTo, t);
    g_briNow = bri;

    for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, applyBrightnessMin1(g_current[i], bri));
    strip.show();
    portalYield(step);
  }
}

// -------------------- Anzeige-Helfer --------------------
static void showSetupPulse(uint32_t rgbColor, uint16_t pulseMs, uint32_t durationMs) {
  static uint32_t setupFrame[LED_COUNT];
  for (int i = 0; i < LED_COUNT; i++) setupFrame[i] = 0;
  drawSetupOverlay(setupFrame, rgbColor);

  uint8_t bri = cfg.maxBrightness;

  uint32_t start = millis();
  while (millis() - start < durationMs) {
    uint32_t tms = millis() - start;
    float phase = (pulseMs == 0) ? 0.0f : (float)(tms % pulseMs) / (float)pulseMs;
    float tri = (phase < 0.5f) ? (phase * 2.0f) : (2.0f - phase * 2.0f);
    float scale = 0.15f + 0.85f * easeInOut(tri);

    g_briNow = bri;
    for (int i = 0; i < LED_COUNT; i++) {
      uint32_t px = setupFrame[i];
      if (!px) { strip.setPixelColor(i, 0); g_current[i] = 0; continue; }
      uint8_t r = (px >> 16) & 0xFF, g = (px >> 8) & 0xFF, b = px & 0xFF;
      uint32_t raw = strip.Color((uint8_t)(r * scale), (uint8_t)(g * scale), (uint8_t)(b * scale));
      g_current[i] = raw;
      strip.setPixelColor(i, applyBrightnessMin1(raw, bri));
    }
    g_hasCurrent = true;
    strip.show();
    portalYield(20);
  }
}

static void showClockFrameWithFade(const Settings& s,
                                  uint8_t hour24, uint8_t minute,
                                  bool setupOverlay,
                                  bool failOverlayRed,
                                  uint8_t briTarget) {
  static uint32_t frame[LED_COUNT];
  renderWordClockWith(s, hour24, minute, frame);

  if (setupOverlay) drawSetupOverlay(frame, s.bootColor);
  if (failOverlayRed) drawSetupOverlay(frame, 0xFF0000);

  FadeMode fm = (FadeMode)s.fadeMode;
  transitionToFrame(frame, fm, s.fadeMs, s.stepMs, briTarget);
}

// -------------------- Nightmode logic --------------------
static bool isWithinNightWindow(uint16_t nowMin, uint16_t startMin, uint16_t endMin) {
  if (startMin == endMin) return true; // full day window
  if (startMin < endMin) return (nowMin >= startMin && nowMin < endMin);
  return (nowMin >= startMin || nowMin < endMin);
}

// -------------------- BH1750 low-level (CONTINUOUS mode) --------------------
static void bh1750DviResetSequence() {
  pinMode(BH1750_DVI_PIN, OUTPUT);
  digitalWrite(BH1750_DVI_PIN, LOW);
  delayMicroseconds(10);
  digitalWrite(BH1750_DVI_PIN, HIGH);
  delay(2);
}
static bool i2cPing(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}
static bool bh1750Write(uint8_t cmd) {
  Wire.beginTransmission(g_bhAddr);
  Wire.write(cmd);
  return (Wire.endTransmission() == 0);
}

static bool bh1750TriggerOneShot() {
  // One-Time H-Resolution mode (1 lx resolution), typical 120ms
  // 0x20 = ONE_TIME_H_RES_MODE
  return bh1750Write(0x20);
}

static void bh1750PowerDown() {
  // 0x00 = POWER DOWN
  bh1750Write(0x00);
}

static bool bh1750Init() {
  // Optional: if you want to be extra aggressive, recover bus first
  // (only if you have i2cBusRecovery() from earlier; otherwise omit)
  // i2cBusRecovery();

  bh1750DviResetSequence();

  Wire.end();
  delay(2);

  Wire.begin(BH1750_SDA_PIN, BH1750_SCL_PIN);
  Wire.setClock(100000);

  g_bhAddr = 0x23;
  if (!i2cPing(g_bhAddr)) return false;

  // POWER ON
  if (!bh1750Write(0x01)) return false;

  // RESET (valid after power on)
  bh1750Write(0x07);

  // Do NOT start continuous here.
  return true;
}

static bool bh1750ReadLux(float &luxOut) {
  // Ensure sensor is initialized
  if (!g_bhOk) {
    g_bhOk = bh1750Init();
    if (!g_bhOk) return false;
  }

  // Trigger one-shot measurement
  if (!bh1750TriggerOneShot()) {
    // try hard re-init once
    g_bhOk = bh1750Init();
    if (!g_bhOk) return false;
    if (!bh1750TriggerOneShot()) return false;
  }

  // Wait for conversion (120ms typical; 180ms robust)
  delay(180);

  // Read 2 bytes
  Wire.requestFrom((int)g_bhAddr, 2);
  if (Wire.available() < 2) {
    // hard re-init once after sleep can be necessary
    g_bhOk = bh1750Init();
    if (!g_bhOk) return false;

    if (!bh1750TriggerOneShot()) return false;
    delay(180);

    Wire.requestFrom((int)g_bhAddr, 2);
    if (Wire.available() < 2) return false;
  }

  uint16_t raw = ((uint16_t)Wire.read() << 8) | (uint16_t)Wire.read();
  luxOut = (float)raw / 1.2f;

  // Optional: power down to reduce weird states across sleep
  bh1750PowerDown();

  return true;
}

static uint8_t luxToBrightness(float lux, uint8_t maxBri, float luxMin, float luxMax) {
  if (lux < 0.0f) lux = 0.0f;

  // Tuning:
  // - gamma:  >1 drückt niedrige Lux stärker nach unten (dunkler), <1 macht's früher hell
  constexpr float gamma  = 2.2f;     // <- größer = dunkler bei low-lux

  if (maxBri < 1) return 1;
  if (lux <= luxMin) return 1;
  if (lux >= luxMax) return maxBri;

  // Log-mapping zwischen luxMin und luxMax
  const float a = log10f(luxMin);
  const float b = log10f(luxMax);
  float n = (log10f(lux) - a) / (b - a);     // 0..1
  if (n < 0.0f) n = 0.0f;
  if (n > 1.0f) n = 1.0f;

  // Optional: smoothstep, damit der Übergang am Anfang/Ende weniger "sprunghaft" wirkt
  n = n * n * (3.0f - 2.0f * n);

  const float curve = powf(n, gamma);
  int bri = (int)lroundf(1.0f + curve * (float)(maxBri - 1));
  if (bri < 1) bri = 1;
  if (bri > maxBri) bri = maxBri;
  return (uint8_t)bri;
}

// -------------------- Brightness helpers --------------------
static inline bool autoBrightnessEnabledForCfg(const Settings& s) {
  return (s.disableAutoBrightness == 0);
}

// --- Portal preview brightness (uses live lux) ---
static uint8_t computeEffectiveBrightnessForPortalPreview() {
  const uint8_t maxBri = previewCfg.maxBrightness;

  if (!autoBrightnessEnabledForCfg(previewCfg)) return maxBri;
  if (!(g_bhOk && g_portalLuxOk && g_portalLuxLast >= 0.0f)) return maxBri;

  uint8_t bri = luxToBrightness(g_portalLuxLast, maxBri, previewCfg.luxMin, previewCfg.luxMax);
  if (bri < 1) bri = 1;
  if (bri > maxBri) bri = maxBri;
  return bri;
}

static bool isNightmodeActive(const struct tm& tmInfo) {
  if (!cfg.enableNightmode) { g_nightLatched = false; return false; }

  const uint16_t nowMin = (uint16_t)(tmInfo.tm_hour * 60 + tmInfo.tm_min);
  if (!isWithinNightWindow(nowMin, cfg.nightStartMin, cfg.nightEndMin)) {
    g_nightLatched = false;
    return false;
  }

  if (!(g_bhOk && g_luxLast >= 0.0f)) { g_nightLatched = false; return false; }

  const float onTh  = cfg.nightLuxThreshold;
  const float delta = (float)cfg.hysteresisDelta;
  const float offTh = onTh + delta;

  if (!g_nightLatched) {
    if (g_luxLast <= onTh) g_nightLatched = true;
  } else {
    if (g_luxLast >= offTh) g_nightLatched = false;
  }
  return g_nightLatched;
}

static void showClockNowRuntime(const struct tm& tmInfo, uint8_t briTarget) {
  Settings s = cfg;

  uint8_t hour24 = (uint8_t)tmInfo.tm_hour;
  uint8_t minute = (uint8_t)tmInfo.tm_min;

  if (isNightmodeActive(tmInfo)) {

    if (s.nightBehavior == NM_NOTHING) {
      fadeToBlack(s.fadeMs, s.stepMs);
      return;
    }

    s.enableMinuteDots = 0;

    if (s.nightBehavior == NM_HALF_HOUR_ONLY) {
      const uint8_t realMin = minute;

      if (realMin >= 15 && realMin <= 44) {
        minute = 30;
      } else {
        minute = 0;
        if (realMin >= 45) {
          hour24 = (uint8_t)((hour24 + 1) % 24);
        }
      }
    }
  }

  showClockFrameWithFade(s, hour24, minute, false, wifiFailOverlay, briTarget);
}

static void applyBrightnessFromCfgNow() {
  bool autoBri = autoBrightnessEnabledForCfg(cfg);

  if (!autoBri) {
    g_briTarget = cfg.maxBrightness;
  } else {
    if (g_luxLast >= 0.0f) g_briTarget = luxToBrightness(g_luxLast, cfg.maxBrightness, cfg.luxMin, cfg.luxMax);
    else g_briTarget = cfg.maxBrightness;
  }

  if (g_briTarget < 1) g_briTarget = 1;
  if (g_briTarget > cfg.maxBrightness) g_briTarget = cfg.maxBrightness;

  fadeBrightnessOnly(g_briTarget, cfg.fadeMs, cfg.stepMs);
}

static uint8_t computeTargetBrightnessWithMax(uint8_t maxBriCandidate) {
  if (maxBriCandidate < 1) maxBriCandidate = 1;

  bool autoBri = autoBrightnessEnabledForCfg(cfg);
  if (!autoBri) return maxBriCandidate;

  if (g_luxLast >= 0.0f) {
    uint8_t b = luxToBrightness(g_luxLast, maxBriCandidate, cfg.luxMin, cfg.luxMax);
    if (b < 1) b = 1;
    if (b > maxBriCandidate) b = maxBriCandidate;
    return b;
  }
  return maxBriCandidate;
}

// -------------------- WPA2 Enterprise helpers (NO CERT CHECK) --------------------
static void wifiEnterpriseDisable() {
#if __has_include("esp_eap_client.h")
  esp_wifi_sta_enterprise_disable();
#else
  esp_wifi_sta_wpa2_ent_disable();
#endif
}

static void wifiEnterpriseConfigureNoCert(const WifiCreds &c) {
  String ident = c.eapIdentity;
  if (ident.length() == 0) ident = c.eapUser;

#if __has_include("esp_eap_client.h")
  // No CA => no server cert validation
  esp_eap_client_set_ca_cert(NULL, 0);

  esp_eap_client_set_identity((const unsigned char*)ident.c_str(), ident.length());
  esp_eap_client_set_username((const unsigned char*)c.eapUser.c_str(), c.eapUser.length());
  esp_eap_client_set_password((const unsigned char*)c.eapPass.c_str(), c.eapPass.length());

  esp_wifi_sta_enterprise_enable();
#else
  esp_wpa2_config_t wpa2_config = WPA2_CONFIG_INIT_DEFAULT();

  esp_wifi_sta_wpa2_ent_set_ca_cert(NULL, 0);

  esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)ident.c_str(), ident.length());
  esp_wifi_sta_wpa2_ent_set_username((uint8_t*)c.eapUser.c_str(), c.eapUser.length());
  esp_wifi_sta_wpa2_ent_set_password((uint8_t*)c.eapPass.c_str(), c.eapPass.length());

  esp_wifi_sta_wpa2_ent_enable(&wpa2_config);
#endif
}

static bool wifiCredsLookValid(const WifiCreds &c) {
  if (c.ssid.isEmpty()) return false;

  if (c.mode == WIFI_MODE_ENTERPRISE) {
    if (c.eapUser.isEmpty()) return false;
    if (c.eapPass.isEmpty()) return false;
  }
  return true;
}

// Start connect (does not block)
static void startWiFiConnectWithCreds(const WifiCreds &c, bool AP) {
  WiFi.disconnect(true, true);
  delay(50);

  if(AP){
    WiFi.mode(WIFI_AP_STA);
  }else{
    WiFi.mode(WIFI_STA);
  }

  WiFi.setSleep(true);

  if (c.mode == WIFI_MODE_ENTERPRISE) {
    wifiEnterpriseDisable();
    wifiEnterpriseConfigureNoCert(c);
    WiFi.begin(c.ssid.c_str());           // Enterprise => no PSK arg
  } else {
    wifiEnterpriseDisable();
    WiFi.begin(c.ssid.c_str(), c.psk.c_str());
  }
}

// Wait for connect while pulsing SETUP (green)
static bool waitForWiFiConnectWithPulse(uint32_t timeoutMs, bool pulse) {
  const uint32_t start = millis();

  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    if(pulse){
      uint32_t remaining = timeoutMs - (millis() - start);
      uint32_t chunk = cfg.connectPulseMs;
      if (chunk < 120) chunk = 120;
      if (chunk > remaining) chunk = remaining;
      showSetupPulse(cfg.connectColor, cfg.connectPulseMs, chunk);
    }else{
      delay(50);
    }
  }
  return (WiFi.status() == WL_CONNECTED);
}

static bool connectWiFiWithCredsBlockingPulsed(const WifiCreds &c, uint32_t timeoutMs, bool pulse, bool AP) {
  startWiFiConnectWithCreds(c, AP);
  return waitForWiFiConnectWithPulse(timeoutMs, pulse);
}

// -------------------- Portal endpoints --------------------
static void markPortalClientSeen() {
  portalLastActivityMs = millis();

  if (!portalDemoActive) {
    portalDemoActive = true;

    previewCfg = cfg;
    previewDirty = true;

    demoHour = random(0, 23);
    demoMin  = random(0, 60);

    uint8_t bri = computeEffectiveBrightnessForPortalPreview();
    showClockFrameWithFade(previewCfg, demoHour, demoMin, false, false, bri);

    nextDemoTickMs = millis() + 2000;
  }
}

static void handleRoot() {
  markPortalClientSeen();
  server.send_P(200, "text/html; charset=utf-8", PAGE_HTML);
}

static void handleFontWoff2() {
  server.setContentLength(wordclock_woff2_len);
  server.send(200, "font/woff2", "");

  const uint8_t* p = wordclock_woff2;
  size_t left = wordclock_woff2_len;

  while (left) {
    size_t n = left > 1024 ? 1024 : left;
    server.sendContent_P((PGM_P)p, n);
    p += n;
    left -= n;
    delay(0);
  }
}

static void handleCfg() {
  markPortalClientSeen();

  WifiCreds wc;
  loadWifiCredsEx(wc);

  uint64_t mac = ESP.getEfuseMac();
  char chip[32];
  snprintf(chip, sizeof(chip), "ESP32-%04X", (uint16_t)(mac & 0xFFFF));

  String json = "{";
  json += "\"chip\":\"" + String(chip) + "\",";
  json += "\"tzName\":\"" + String(cfg.tzName) + "\",";

  json += "\"maxBrightness\":" + String(cfg.maxBrightness) + ",";
  json += "\"disableAutoBrightness\":" + String(cfg.disableAutoBrightness) + ",";

  json += "\"fadeMode\":" + String(cfg.fadeMode) + ",";
  json += "\"fadeMs\":" + String(cfg.fadeMs) + ",";

  json += "\"colorM\":\"" + colorToHex(cfg.colorM) + "\",";
  json += "\"colorH\":\"" + colorToHex(cfg.colorH) + "\",";

  json += "\"enableNightmode\":" + String(cfg.enableNightmode) + ",";
  json += "\"nightBehavior\":" + String(cfg.nightBehavior) + ",";
  json += "\"nightStart\":\"" + formatTimeHHMM(cfg.nightStartMin) + "\",";
  json += "\"nightEnd\":\"" + formatTimeHHMM(cfg.nightEndMin) + "\",";
  json += "\"nightLuxThreshold\":" + String(cfg.nightLuxThreshold, 2) + ",";

  json += "\"bootColor\":\"" + colorToHex(cfg.bootColor) + "\",";
  json += "\"bootFadeMs\":" + String(cfg.bootFadeMs) + ",";

  json += "\"connectColor\":\"" + colorToHex(cfg.connectColor) + "\",";
  json += "\"connectPulseMs\":" + String(cfg.connectPulseMs) + ",";

  json += "\"ntpSyncHours\":" + String(cfg.ntpSyncHours) + ",";
  json += "\"connectionTimeoutSec\":" + String(cfg.connectionTimeoutSec) + ",";
  json += "\"enableMinuteDots\":" + String(cfg.enableMinuteDots) + ",";
  json += "\"stepMs\":" + String(cfg.stepMs) + ",";

  json += "\"luxCheckIntervalSec\":" + String(cfg.luxCheckIntervalSec) + ",";
  json += "\"hysteresisDelta\":" + String(cfg.hysteresisDelta) + ",";
  json += "\"portalInactiveMaxSec\":" + String(cfg.portalInactiveMaxSec) + ",";

  json += "\"luxMin\":" + String(cfg.luxMin) + ",";
  json += "\"luxMax\":" + String(cfg.luxMax) + ",";

  // WiFi state (no passwords)
  json += "\"currentSsid\":\"" + wc.ssid + "\",";
  json += "\"wifiMode\":" + String((uint8_t)wc.mode) + ",";
  json += "\"eapIdentity\":\"" + wc.eapIdentity + "\",";
  json += "\"eapUser\":\"" + wc.eapUser + "\"";

  json += "}";
  server.send(200, "application/json; charset=utf-8", json);
}

static void handleScan() {
  markPortalClientSeen();
  int n = WiFi.scanNetworks(false, true);

  WifiCreds wc;
  loadWifiCredsEx(wc);

  String json = "{";
  json += "\"currentSsid\":\"" + wc.ssid + "\",";
  json += "\"networks\":[";
  for (int i = 0; i < n; i++) {
    if (i) json += ",";
    String s = WiFi.SSID(i);
    int rssi = WiFi.RSSI(i);
    s.replace("\"", "");
    json += "{\"ssid\":\"" + s + "\",\"rssi\":" + String(rssi) + "}";
  }
  json += "]}";
  server.send(200, "application/json; charset=utf-8", json);
  WiFi.scanDelete();
}

static void sendJson(bool ok, const String& msg) {
  String j = "{";
  j += "\"ok\":" + String(ok ? "true" : "false") + ",";
  j += "\"msg\":\"";
  String m = msg; m.replace("\"", "'");
  j += m + "\"}";
  server.send(ok ? 200 : 400, "application/json; charset=utf-8", j);
}

static void handlePreview() {
  markPortalClientSeen();
  previewCfg = cfg;

  uint8_t u8; uint16_t u16; float f;

  String tzName = server.arg("tzName");
  if (tzName.length() > 0) safeCopy(previewCfg.tzName, sizeof(previewCfg.tzName), tzName);

  if (strToU8(server.arg("maxBrightness"), u8)) {
    if (u8 < 1) u8 = 1;
    previewCfg.maxBrightness = u8;
  }

  if (server.hasArg("disableAutoBrightness")) {
    previewCfg.disableAutoBrightness = (server.arg("disableAutoBrightness") == "1") ? 1 : 0;
  }

  if (strToU8(server.arg("fadeMode"), u8)) previewCfg.fadeMode = u8;
  if (strToU16(server.arg("fadeMs"), u16)) previewCfg.fadeMs = snapFadeMs(u16);

  String cM = server.arg("colorM");
  String cH = server.arg("colorH");
  if (cM.length()) previewCfg.colorM = parseColorHex(cM);
  if (cH.length()) previewCfg.colorH = parseColorHex(cH);

  previewCfg.enableMinuteDots = (server.arg("enableMinuteDots") == "1") ? 1 : 0;
  if (strToU16(server.arg("stepMs"), u16)) previewCfg.stepMs = max<uint16_t>(5, u16);

  if (strToU16(server.arg("luxCheckIntervalSec"), u16)) {
    if (u16 < 1) u16 = 1;
    if (u16 > 60) u16 = 60;
    previewCfg.luxCheckIntervalSec = u16;
  }

  if (strToU8(server.arg("hysteresisDelta"), u8)) {
    if (u8 < 1) u8 = 1;
    if (u8 > 50) u8 = 50;
    previewCfg.hysteresisDelta = u8;
  }

  if (server.hasArg("portalInactiveMaxSec")) {
    uint32_t v = (uint32_t)strtoul(server.arg("portalInactiveMaxSec").c_str(), nullptr, 10);
    if (v < 30UL) v = 30UL;
    if (v > 3600UL) v = 3600UL;
    previewCfg.portalInactiveMaxSec = v;
  }

  if (strToFloat(server.arg("luxMin"), f)) {
    if (f < 0) f = 0;
    if (f > 50000) f = 50000;
    previewCfg.luxMin = f;
  }

  if (strToFloat(server.arg("luxMax"), f)) {
    if (f < 0) f = 0;
    if (f > 50000) f = 50000;
    previewCfg.luxMax = f;
  }

  // Nightmode preview fields
  previewCfg.enableNightmode = (server.arg("enableNightmode") == "1") ? 1 : 0;

  if (strToU8(server.arg("nightBehavior"), u8)) {
    if (u8 > NM_NOTHING) u8 = NM_DOTS_ONLY;
    previewCfg.nightBehavior = u8;
  }
  if (server.hasArg("nightStart")) {
    if (parseTimeHHMM(server.arg("nightStart"), u16)) previewCfg.nightStartMin = u16;
  }
  if (server.hasArg("nightEnd")) {
    if (parseTimeHHMM(server.arg("nightEnd"), u16)) previewCfg.nightEndMin = u16;
  }
  if (server.hasArg("nightLuxThreshold")) {
    if (strToFloat(server.arg("nightLuxThreshold"), f)) previewCfg.nightLuxThreshold = f;
  }

  applyClamps(previewCfg);

  previewDirty = true;
  server.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

static void handleSave() {
  markPortalClientSeen();

  // --- Build WifiCreds from request ---
  WifiCreds saved;
  bool haveSaved = loadWifiCredsEx(saved);

  WifiCreds wc;
  wc.ssid = server.arg("ssid");
  if (wc.ssid.length() == 0) { sendJson(false, "SSID is missing."); return; }

  // wifiMode: 0=PSK, 1=Enterprise (advanced)
  uint8_t wifiModeU8 = 0;
  if (strToU8(server.arg("wifiMode"), wifiModeU8)) {
    wc.mode = wifiModeU8 ? WIFI_MODE_ENTERPRISE : WIFI_MODE_PSK;
  } else {
    // default: PSK (keeps backward compatibility with old portal HTML)
    wc.mode = WIFI_MODE_PSK;
  }

  // PSK flow (keeps your old "unchanged" logic)
  if (wc.mode == WIFI_MODE_PSK) {
    String pass = server.arg("pass");
    String passUnchanged = server.arg("passUnchanged");

    const bool wantsReuse = (passUnchanged == "1") || (pass.length() == 0);

    if (wantsReuse) {
      if (haveSaved && saved.mode == WIFI_MODE_PSK && saved.ssid == wc.ssid && saved.psk.length() > 0) {
        wc.psk = saved.psk;
      } else {
        sendJson(false, "Password required for this SSID.");
        return;
      }
    } else {
      wc.psk = pass;
    }
  } else {
    // Enterprise flow (NO CERT CHECK in connect)
    wc.eapIdentity = server.arg("eapIdentity");
    wc.eapUser     = server.arg("eapUser");
    String eapPass = server.arg("eapPass");
    String eapPassUnchanged = server.arg("eapPassUnchanged");

    if (wc.eapUser.length() == 0) { sendJson(false, "Enterprise username is missing."); return; }

    const bool wantsReuseEap = (eapPassUnchanged == "1") || (eapPass.length() == 0);
    if (wantsReuseEap) {
      if (haveSaved && saved.mode == WIFI_MODE_ENTERPRISE && saved.ssid == wc.ssid && saved.eapPass.length() > 0) {
        wc.eapPass = saved.eapPass;
        if (wc.eapIdentity.length() == 0) wc.eapIdentity = saved.eapIdentity;
        if (wc.eapUser.length() == 0)     wc.eapUser     = saved.eapUser;
      } else {
        sendJson(false, "Enterprise password required for this SSID.");
        return;
      }
    } else {
      wc.eapPass = eapPass;
    }
  }

  // --- Now read cfg fields as before ---
  String tzName = server.arg("tzName");
  if (tzName.length() == 0) tzName = "Europe/Berlin";
  safeCopy(cfg.tzName, sizeof(cfg.tzName), tzName);

  uint8_t u8; uint16_t u16; float f;

  if (strToU8(server.arg("maxBrightness"), u8)) {
    if (u8 < 1) u8 = 1;
    cfg.maxBrightness = u8;
  }
  if (server.hasArg("disableAutoBrightness")) {
    cfg.disableAutoBrightness = (server.arg("disableAutoBrightness") == "1") ? 1 : 0;
  }

  if (strToU8(server.arg("fadeMode"), u8)) cfg.fadeMode = u8;
  if (strToU16(server.arg("fadeMs"), u16)) cfg.fadeMs = snapFadeMs(u16);

  String cM = server.arg("colorM");
  String cH = server.arg("colorH");
  if (cM.length()) cfg.colorM = parseColorHex(cM);
  if (cH.length()) cfg.colorH = parseColorHex(cH);

  cfg.enableNightmode = (server.arg("enableNightmode") == "1") ? 1 : 0;
  if (strToU8(server.arg("nightBehavior"), u8)) {
    if (u8 > NM_NOTHING) u8 = NM_DOTS_ONLY;
    cfg.nightBehavior = u8;
  }
  if (server.hasArg("nightStart")) {
    if (parseTimeHHMM(server.arg("nightStart"), u16)) cfg.nightStartMin = u16;
  }
  if (server.hasArg("nightEnd")) {
    if (parseTimeHHMM(server.arg("nightEnd"), u16)) cfg.nightEndMin = u16;
  }
  if (server.hasArg("nightLuxThreshold")) {
    if (strToFloat(server.arg("nightLuxThreshold"), f)) cfg.nightLuxThreshold = f;
  }

  String cBoot = server.arg("bootColor");
  if (cBoot.length()) cfg.bootColor = parseColorHex(cBoot);
  if (strToU16(server.arg("bootFadeMs"), u16)) cfg.bootFadeMs = u16;

  String cCon = server.arg("connectColor");
  if (cCon.length()) cfg.connectColor = parseColorHex(cCon);
  if (strToU16(server.arg("connectPulseMs"), u16)) cfg.connectPulseMs = u16;

  if (strToU8(server.arg("ntpSyncHours"), u8)) cfg.ntpSyncHours = max<uint8_t>(1, u8);
  if (strToU8(server.arg("connectionTimeoutSec"), u8)) cfg.connectionTimeoutSec = max<uint8_t>(3, u8);

  cfg.enableMinuteDots = (server.arg("enableMinuteDots") == "1") ? 1 : 0;
  if (strToU16(server.arg("stepMs"), u16)) cfg.stepMs = max<uint16_t>(5, u16);

  if (strToU16(server.arg("luxCheckIntervalSec"), u16)) {
    if (u16 < 1) u16 = 1;
    if (u16 > 60) u16 = 60;
    cfg.luxCheckIntervalSec = u16;
  }
  if (strToU8(server.arg("hysteresisDelta"), u8)) {
    if (u8 < 1) u8 = 1;
    if (u8 > 50) u8 = 50;
    cfg.hysteresisDelta = u8;
  }
  if (server.hasArg("portalInactiveMaxSec")) {
    uint32_t v = (uint32_t)strtoul(server.arg("portalInactiveMaxSec").c_str(), nullptr, 10);
    if (v < 30UL) v = 30UL;
    if (v > 3600UL) v = 3600UL;
    cfg.portalInactiveMaxSec = v;
  }
  if (strToFloat(server.arg("luxMin"), f)) {
    if (f < 0) f = 0;
    if (f > 50000) f = 50000;
    cfg.luxMin = f;
  }

  if (strToFloat(server.arg("luxMax"), f)) {
    if (f < 0) f = 0;
    if (f > 50000) f = 50000;
    cfg.luxMax = f;
  }

  applyRuntimeClamps();
  saveSettings();
  applyTimezone();

  // Start async connect job
  g_pendingCreds = wc;
  g_pendingCredsValid = true;

  g_saveTimeoutMs = (uint32_t)cfg.connectionTimeoutSec * 1000UL;
  g_saveStartedMs = millis();
  g_saveState = SJ_CONNECTING;
  g_saveMsg = "Connecting...";

  // STA connect starten (AP bleibt sowieso an, weil Portal aktiv ist)
  startWiFiConnectWithCreds(g_pendingCreds, true);

  // SOFORT antworten – kein langes Blockieren mehr
  server.send(200, "application/json; charset=utf-8",
              "{\"ok\":true,\"state\":\"connecting\",\"msg\":\"Connecting...\"}");
}

static void handlePing() {
  markPortalClientSeen();
  server.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

static void handleStatus() {
  markPortalClientSeen();

  String st = "idle";
  if (g_saveState == SJ_CONNECTING) st = "connecting";
  else if (g_saveState == SJ_OK) st = "ok";
  else if (g_saveState == SJ_FAIL) st = "fail";

  String json = "{";
  json += "\"state\":\"" + st + "\",";
  json += "\"msg\":\"" + g_saveMsg + "\",";
  json += "\"wifi\":" + String(WiFi.status() == WL_CONNECTED ? "true":"false");
  json += "}";

  server.send(200, "application/json; charset=utf-8", json);
}

static void handleLux() {
  markPortalClientSeen();

  const bool autoEn = autoBrightnessEnabledForCfg(previewCfg);
  const uint8_t maxBri = previewCfg.maxBrightness;
  const uint8_t effBri = computeEffectiveBrightnessForPortalPreview();

  String json = "{";
  json += "\"luxOk\":" + String(g_portalLuxOk ? "true" : "false") + ",";
  json += "\"lux\":" + String(g_portalLuxLast, 2) + ",";
  json += "\"autoEnabled\":" + String(autoEn ? "true" : "false") + ",";
  json += "\"maxBrightness\":" + String(maxBri) + ",";
  json += "\"effectiveBrightness\":" + String(effBri);
  json += "}";

  server.send(200, "application/json; charset=utf-8", json);
}

static void handleNotFound() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
  server.send(302, "text/plain", "");
}

static void tickSaveJob() {
  if (g_saveState != SJ_CONNECTING || !g_pendingCredsValid) return;

  // Timeout?
  if ((uint32_t)(millis() - g_saveStartedMs) > g_saveTimeoutMs) {
    WiFi.disconnect(false, false); // STA trennen, AP nicht anfassen
    g_saveState = SJ_FAIL;
    g_saveMsg = "WiFi connection failed (timeout).";
    return;
  }

  // Verbunden?
  if (WiFi.status() == WL_CONNECTED) {
    saveWifiCredsEx(g_pendingCreds); // erst JETZT persistieren
    g_saveState = SJ_OK;
    g_saveMsg = "WiFi connected";

    // Portal sauber schließen, aber mit etwas Puffer
    g_portalExit = PEXIT_SAVED_OK;
    g_portalExitAtMs = millis() + 2000;
    return;
  }
}

// -------------------- Portal Start/Run --------------------
static void startConfigPortal() {
  portalActive = true;
  portalLastActivityMs = millis();
  g_portalExit = PEXIT_NONE;

  portalDemoActive = false;
  previewCfg = cfg;
  previewDirty = true;

  // reset portal lux state
  g_portalLuxOk = false;
  g_portalLuxLast = -1.0f;
  g_portalNextLuxPollMs = 0;

  showSetupSolidCrossFade(cfg.bootColor, cfg.bootFadeMs, true);

  wifiOff();
  delay(50);

  uint64_t mac = ESP.getEfuseMac();
  char apName[32];
  snprintf(apName, sizeof(apName), "WortUhr-%04X", (uint16_t)(mac & 0xFFFF));

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName);

  IPAddress ip = WiFi.softAPIP();
  dnsServer.start(53, "*", ip);

  // Ensure BH1750 is up for live lux in portal
  g_bhOk = bh1750Init();
  g_luxLast = -1.0f;

  server.on("/", handleRoot);
  server.on("/wordclock.woff2", handleFontWoff2);
  server.on("/cfg", handleCfg);
  server.on("/scan", handleScan);
  server.on("/preview", HTTP_POST, handlePreview);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/ping", handlePing);
  server.on("/lux", handleLux);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);

  ElegantOTA.begin(&server);

  server.begin();
}

static void portalDemoTick() {
  if (previewDirty) {
    previewDirty = false;
    uint8_t bri = computeEffectiveBrightnessForPortalPreview();
    showClockFrameWithFade(previewCfg, demoHour, demoMin, false, false, bri);
  }

  uint32_t nowMs = millis();
  if ((int32_t)(nowMs - (int32_t)nextDemoTickMs) >= 0) {
    nextDemoTickMs = nowMs + 2000;

    demoMin++;
    if (demoMin >= 60) { demoMin = 0; demoHour = (demoHour + 1) % 24; }

    uint8_t bri = computeEffectiveBrightnessForPortalPreview();
    showClockFrameWithFade(previewCfg, demoHour, demoMin, false, false, bri);
  }
}

static void runConfigPortalLoop() {
  const uint32_t inactiveMaxMs = cfg.portalInactiveMaxSec * 1000UL;

  while (portalActive) {
    dnsServer.processNextRequest();
    server.handleClient();

    ElegantOTA.loop();

    tickSaveJob();

    // ---- Live Lux polling (portal) ----
    uint32_t nowMs = millis();
    if ((int32_t)(nowMs - (int32_t)g_portalNextLuxPollMs) >= 0) {
      g_portalNextLuxPollMs = nowMs + PORTAL_LUX_POLL_MS;

      if (!g_bhOk) {
        g_bhOk = bh1750Init();
        g_portalLuxOk = false;
        g_portalLuxLast = -1.0f;
      }

      if (g_bhOk) {
        float lux;
        if (bh1750ReadLux(lux)) {
          g_portalLuxLast = lux;
          g_portalLuxOk = true;
        }
      }
    }

    if (portalDemoActive) portalDemoTick();

    if (portalDemoActive && (uint32_t)(millis() - portalLastActivityMs) > PORTAL_DEMO_IDLE_MS) {
      portalDemoActive = false;
      previewDirty = false;
      showSetupSolidCrossFade(cfg.bootColor, cfg.bootFadeMs, true);
    }

    if (g_portalExit == PEXIT_SAVED_OK && (int32_t)(millis() - g_portalExitAtMs) >= 0) {
      server.stop();
      dnsServer.stop();
      WiFi.softAPdisconnect(true);
      wifiOff();
      portalActive = false;
      return;
    }

    if ((uint32_t)(millis() - portalLastActivityMs) > inactiveMaxMs) {
      showSetupSolidCrossFade(0xFF0000, cfg.bootFadeMs, true);
      delay(300);
      server.stop();
      dnsServer.stop();
      WiFi.softAPdisconnect(true);
      wifiOff();
      esp_deep_sleep_start();
    }

    delay(5);
  }
}

// -------------------- Boot Flow (WiFi connect else portal) --------------------
static bool ensureWiFiOrPortal() {
  WifiCreds wc;
  bool have = loadWifiCredsEx(wc);

  if (have  && wifiCredsLookValid(wc)) {
    uint32_t timeoutMs = (uint32_t)cfg.connectionTimeoutSec * 1000UL;

    bool ok = connectWiFiWithCredsBlockingPulsed(wc, timeoutMs, true, false);

    if (ok) {
      noteWifiAttemptResult(true);
      return true;
    }

    wifiOff();
    noteWifiAttemptResult(false);
    showSetupSolidCrossFade(cfg.bootColor, cfg.bootFadeMs, true);
  }

  g_portalExit = PEXIT_NONE;
  startConfigPortal();
  runConfigPortalLoop();

  if (g_portalExit == PEXIT_SAVED_OK) {
    g_portalExit = PEXIT_NONE;

    fadeToBlack(cfg.bootFadeMs, cfg.stepMs);
    showSetupPulse(cfg.connectColor, cfg.connectPulseMs, cfg.connectPulseMs);

    return true;
  }

  return false;
}

// -------------------- Zeit holen (Start + alle N Stunden) --------------------
static bool ensureTimeNow() {
  WifiCreds wc;
  if (!loadWifiCredsEx(wc) || !wifiCredsLookValid(wc)) {
    noteWifiAttemptResult(false);
    return false;
  }

  uint32_t timeoutMs = (uint32_t)cfg.connectionTimeoutSec * 1000UL;

  bool okWifi = (WiFi.status() == WL_CONNECTED) ? true : connectWiFiWithCredsBlockingPulsed(wc, timeoutMs, false, false);
  if (!okWifi) {
    wifiOff();
    noteWifiAttemptResult(false);
    return false;
  }

  bool okNtp = syncTimeNTP(timeoutMs);
  wifiOff();

  noteWifiAttemptResult(okNtp);
  if (okNtp) g_initialSyncDone = true;
  return okNtp;
}

static void enableBtnWakeup() {
  esp_sleep_enable_gpio_wakeup();
  gpio_wakeup_enable((gpio_num_t)BTN_SETTINGS, GPIO_INTR_LOW_LEVEL);
}

static void sleepUntilNextMinute(const struct tm &tmInfo) {
  int waitSec = 60 - tmInfo.tm_sec;
  if (waitSec <= 0) waitSec = 60;

  wifiOff();
  if (g_bhOk) {
    bh1750PowerDown();
  }
  Wire.end();
  enableBtnWakeup();
  esp_sleep_enable_timer_wakeup((uint64_t)waitSec * 1000000ULL);
  esp_light_sleep_start();

  g_bhOk = bh1750Init();
}

static void sleepUntilNextWakeAuto(const struct tm &tmInfo) {
  int toNextMinute = 60 - tmInfo.tm_sec;
  if (toNextMinute <= 0) toNextMinute = 60;

  uint32_t msToNextMinute = (uint32_t)toNextMinute * 1000UL;
  uint32_t waitMs = cfg.luxCheckIntervalSec * 1000UL;
  if (waitMs > msToNextMinute) waitMs = msToNextMinute;
  if (waitMs < 200) waitMs = 200;

  wifiOff();
  if (g_bhOk) {
    bh1750PowerDown();
  }
  Wire.end();
  enableBtnWakeup();
  esp_sleep_enable_timer_wakeup((uint64_t)waitMs * 1000ULL);
  esp_light_sleep_start();

  g_bhOk = bh1750Init();
}

static void quickAdjustModeBlocking() {
  // start value
  uint8_t pendingMax = cfg.maxBrightness;
  uint32_t lastActionMs = millis();

  // White SETUP overlay fade-in for quick adjust mode
  showSetupSolidCrossFade(0xFFFFFF, cfg.bootFadeMs, false);

  // repeat parameters
  const uint16_t firstRepeatDelayMs = 420;
  const uint16_t repeatEveryMs      = 80;

  uint32_t leftNext  = 0;
  uint32_t rightNext = 0;
  bool leftWasDown   = false;
  bool rightWasDown  = false;

  // Make sure BTN_SETTINGS is not stuck as "pressed" from wake
  // (optional but makes UX nicer)
  if (btnPressed(BTN_SETTINGS)) {
    // wait release (debounced)
    while (btnPressed(BTN_SETTINGS)) delay(1);
    delay(20);
  }

  while (true) {
    // Timeout: 30s no button activity => exit without saving
    if ((millis() - lastActionMs) > 30000UL) {
      return;
    }

    // --- BTN_SETTINGS: short press => commit, long press => portal
    if (btnPressed(BTN_SETTINGS)) {
      uint32_t t0 = millis();
      // wait while held, but allow long-press detection
      while (btnPressed(BTN_SETTINGS)) {
        if ((millis() - t0) >= 3000UL) {
          // LONG PRESS => open portal
          // (discard pending; portal can change cfg anyway)
          showSetupSolidCrossFade(cfg.bootColor, cfg.bootFadeMs, true);
          startConfigPortal();
          runConfigPortalLoop();

          // After portal: reload settings (portal may have changed them)
          if (!loadSettings()) { setDefaultSettings(); saveSettings(); }
          applyRuntimeClamps();
          applyTimezone();

          // Re-apply brightness target immediately
          g_briTarget = computeTargetBrightnessWithMax(cfg.maxBrightness);
          if (g_briTarget != g_briNow) fadeBrightnessOnly(g_briTarget, 160, cfg.stepMs);

          return;
        }
        delay(1);
      }

      // SHORT PRESS => commit pending maxBrightness
      if (pendingMax != cfg.maxBrightness) {
        cfg.maxBrightness = pendingMax;
        applyRuntimeClamps();
        saveSettings();
      }

      // Update target with new max right away
      g_briTarget = computeTargetBrightnessWithMax(cfg.maxBrightness);
      if (g_briTarget != g_briNow) fadeBrightnessOnly(g_briTarget, 160, cfg.stepMs);

      return;
    }

    // --- LEFT/RIGHT repeat handling (BTN_LEFT = -1, BTN_RIGHT = +1)
    const uint32_t now = millis();
    bool leftDown  = btnPressed(BTN_LEFT);
    bool rightDown = btnPressed(BTN_RIGHT);

    if (leftDown && !leftWasDown) {
      // initial press
      if (pendingMax > 1) pendingMax--;
      lastActionMs = now;
      leftNext = now + firstRepeatDelayMs;
    } else if (leftDown && (int32_t)(now - leftNext) >= 0) {
      if (pendingMax > 1) pendingMax--;
      lastActionMs = now;
      leftNext = now + repeatEveryMs;
    }
    if (!leftDown) leftNext = 0;
    leftWasDown = leftDown;

    if (rightDown && !rightWasDown) {
      if (pendingMax < 255) pendingMax++;
      lastActionMs = now;
      rightNext = now + firstRepeatDelayMs;
    } else if (rightDown && (int32_t)(now - rightNext) >= 0) {
      if (pendingMax < 255) pendingMax++;
      lastActionMs = now;
      rightNext = now + repeatEveryMs;
    }
    if (!rightDown) rightNext = 0;
    rightWasDown = rightDown;

    // If changed by +/- buttons: apply immediately (but do NOT persist)
    static uint8_t lastShownPending = 0xFF;
    if (pendingMax != lastShownPending) {
      lastShownPending = pendingMax;

      // Recompute target brightness using *pending* max
      uint8_t newTarget = computeTargetBrightnessWithMax(pendingMax);

      if (newTarget < 1) newTarget = 1;
      if (newTarget > pendingMax) newTarget = pendingMax;

      g_briTarget = newTarget;
      if (g_briTarget != g_briNow) {
        fadeBrightnessOnly(g_briTarget, 90, cfg.stepMs);
      }
    }

    delay(5);
  }
}

// -------------------- Arduino Setup/Loop --------------------
void wordClockSetup() {

  if (!loadSettings()) {
    setDefaultSettings();
    saveSettings();
  }
  applyRuntimeClamps();
  applyTimezone();

  pinMode(BTN_SETTINGS, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  randomSeed(analogRead(2));
  demoHour = random(0, 24);
  demoMin = random(0, 60);

  strip.begin();
  strip.clear();
  strip.setBrightness(255);
  strip.show();

  g_briNow = cfg.maxBrightness;
  g_briTarget = cfg.maxBrightness;

  bool ok = ensureWiFiOrPortal();

  if (ok && !g_initialSyncDone) {
    initNTP();
    showSetupSolidCrossFade(cfg.connectColor, cfg.bootFadeMs, true);
    ensureTimeNow();
  } else if (!ok) {
    showSetupSolidCrossFade(cfg.bootColor, cfg.bootFadeMs, true);
  }

  bool needLux = autoBrightnessEnabledForCfg(cfg) || (cfg.enableNightmode != 0);
  if (needLux) {
    g_bhOk = bh1750Init();
    g_luxLast = -1.0f;
    g_nextLuxCheckMs = millis();
    
    float lux;
    if (bh1750ReadLux(lux)) {
      g_luxLast = lux;
      g_briTarget = luxToBrightness(lux, cfg.maxBrightness, cfg.luxMin, cfg.luxMax);
    } else {
      g_briTarget = cfg.maxBrightness;
    }
  } else {
    g_bhOk = false;
    g_luxLast = -1.0f;
  }

  struct tm tmInfo;
  if (getLocalTime(&tmInfo, 2000)) {
    showClockNowRuntime(tmInfo, g_briTarget);
  }
}

void wordClockLoop() {

  struct tm tmInfo;
  if (!getLocalTime(&tmInfo, 1000)) {
    ensureTimeNow();
    delay(200);
    return;
  }

  // --- Button 5: short press => quick adjust, long press => portal
  if (btnPressed(BTN_SETTINGS)) {
    uint32_t t0 = millis();
    while (btnPressed(BTN_SETTINGS)) {
      if ((millis() - t0) >= 3000UL) {
        // LONG PRESS => portal
        showSetupSolidCrossFade(cfg.bootColor, cfg.bootFadeMs, true);
        startConfigPortal();
        runConfigPortalLoop();

        // reload settings after portal
        if (!loadSettings()) { setDefaultSettings(); saveSettings(); }
        applyRuntimeClamps();
        applyTimezone();

        // ensure time (optional, but nice)
        if (!g_initialSyncDone) {
          showSetupSolidCrossFade(cfg.connectColor, cfg.bootFadeMs, true);
          ensureTimeNow();
        }

        // show clock again
        g_briTarget = computeTargetBrightnessWithMax(cfg.maxBrightness);
        showClockNowRuntime(tmInfo, g_briTarget);
        return;
      }
      delay(1);
    }

    // SHORT PRESS => quick adjust mode
    quickAdjustModeBlocking();

    // After returning: re-render once (in case brightness changed)
    g_briTarget = computeTargetBrightnessWithMax(cfg.maxBrightness);
    showClockNowRuntime(tmInfo, g_briTarget);
    return;
  }

  static int lastShownMinute = -1;
  static int lastShownHour   = -1;

  const bool minuteChanged =
      (tmInfo.tm_min != lastShownMinute) || (tmInfo.tm_hour != lastShownHour);

  const bool autoBriEnabled = (cfg.disableAutoBrightness == 0);

  uint32_t nowMs = millis();
  bool doLuxCheck = minuteChanged || ((int32_t)(nowMs - g_nextLuxCheckMs) >= 0);

  const bool needLux = autoBriEnabled || cfg.enableNightmode;

  if (doLuxCheck) {
    g_nextLuxCheckMs = nowMs + (uint32_t)cfg.luxCheckIntervalSec * 1000UL;

    if (needLux) {
      if (!g_bhOk) g_bhOk = bh1750Init();

      if (g_bhOk) {
        float lux;
        if (bh1750ReadLux(lux)) {
          g_luxLast = lux;

          bool nightModeNow = isNightmodeActive(tmInfo);
          nightToggled = (nightModeNow != lastNightShown);
          lastNightShown = nightModeNow;

          if (autoBriEnabled) {
            uint8_t cand = luxToBrightness(lux, cfg.maxBrightness, cfg.luxMin, cfg.luxMax);
            if (cand < 1) cand = 1;
            if (cand > cfg.maxBrightness) cand = cfg.maxBrightness;

            // ---- NEW RULES ----
            // Down: always accept (even -1)
            if (cand < g_briTarget) {
              g_briTarget = cand;
            }
            // Up: only accept if it exceeds hysteresisDelta
            else if (cand > (uint8_t)min<int>(255, (int)g_briTarget + (int)cfg.hysteresisDelta)) {
              g_briTarget = cand;
            }
            // else: ignore small upward jitter
          } else {
            g_briTarget = cfg.maxBrightness;
          }
        } else {
          // read failed
          if (!autoBriEnabled) g_briTarget = cfg.maxBrightness;
        }
      } else {
        // sensor init failed
        if (!autoBriEnabled) g_briTarget = cfg.maxBrightness;
      }
    } else {
      g_luxLast = -1.0f;
      g_briTarget = cfg.maxBrightness;
    }

    if (g_briTarget < 1) g_briTarget = 1;
    if (g_briTarget > cfg.maxBrightness) g_briTarget = cfg.maxBrightness;
  }

  if (minuteChanged || nightToggled) {
    showClockNowRuntime(tmInfo, g_briTarget);
    lastShownMinute = tmInfo.tm_min;
    lastShownHour   = tmInfo.tm_hour;
  } else {
    if (g_briTarget != g_briNow) {
      fadeBrightnessOnly(g_briTarget, cfg.fadeMs, cfg.stepMs);
    }
  }

  if (needsNtpSync()) ensureTimeNow();

  if (!autoBriEnabled) sleepUntilNextMinute(tmInfo);
  else sleepUntilNextWakeAuto(tmInfo);
}
