// v96 pale young-grass day ground + EQ outline synced to text ink: coarse/stable tilt, screen-space world tilt, near-edge return window,
 // 3s stable-level UFO rescue, and revised sky/cloud palette.
// Based on v52; microphone/terrain analysis itself is unchanged.
// v52: WAVE runner is stationary in world space, so it scrolls left with terrain while waving.
// v50: rain X spacing reduced to about half; other rain behavior unchanged.
// v97: Choplifter-style sand ground; real local sunrise/sunset from Open-Meteo; saved offline solar schedule.
// v98: real moonrise/moonset, low on-screen celestial arcs, centered clock block, T toggles date/time/weather.
// v102: SUN rise/set moved upper-left, MOON rise/set upper-right; date/weekday same size but bold; day/night ground both Choplifter sand #E9B86A.
// v104: renamed SOUND TERRARIUM / inspired by retro arcade; Claude review hardening: rate-limited NTP/Wi-Fi retries, bounded I2S read, safer scheduled UFO window, reduced per-frame String churn.
// v108am: tide display layout = vertical HIGH/LOW, with TIDE UP/DN to the right of LOW.
// v108ak: add Open-Meteo Marine tide HIGH/LOW display (I), plus MSL pressure and precipitation probability (T).
// v108x: night stars keep the existing positions/count, add sparse blue/red/yellow accents, and five phase-staggered twinkling stars.\n// v108w: central line spacing relaxed by 1px per gap: DATE y=35, TIME y=47, WEATHER y=72.\n// v108v: WEATHER now uses the same FreeSansBold9pt 7/9 scale as DATE/weekday.\n// v108u: DATE/weekday 7pt, TIME 16pt, WEATHER 6pt; compacted spacing; LOCATION moved to bottom-left.\n// v108t: compact central clock stack after font reduction; TIME y=47, WEATHER y=73.\n// v108m: Cloud brightness now follows the same continuous solar-elevation model as the sky.
// v108l: T/I display scopes made fully independent; per-frame LOCATION String allocation removed.
// v108k: review fixes: Sun/Moon ephemeris handled independently; Wi-Fi setup handlers are registered only once.
//        T and I remain independent: T default ON, I default OFF.
// v108j: Moon uses the proven Sun-face design and is visible from the exact moonrise edge.
// v107: fixes ArduinoJson extraction of sunrise/sunset/moonrise/moonset; keeps daily fetch + hourly retry design.
// v99: compact date/time, bold weather, and visible Sun/Moon rise-set times in the T-toggle information block.
// v49: weather/cloud_cover density; sky-following cloud tone; down-right rain.
// v48: rain/snow now fall from screen top to terrain zone.
// v47: night stars remain full; opaque dark foreground clouds mask them naturally.
// v46: night clouds are dark for CLOUDY/SNOW as well as rain/thunder.
// v45: moon face readability restored; cloudy nights no longer show a full star field.
// v44: astronomical lunar phase moon; antique human-face artwork retained.
// v35: cool-to-warm terrain, large off-screen celestial ellipse, yellow classical moon, J jump / W wave / U UFO, corrected DOWN key, clean status text, hidden Wi-Fi password.
// v33: stores up to five Wi-Fi networks and auto-connects to the strongest saved AP in range.
// v32b: saved AP also shows live RSSI when currently visible; otherwise OUT OF RANGE.
// v32a: weather fetch diagnostics + weekday appended to centered date.
// v32: explicit Open-Meteo current weather fetch with visible HTTP/JSON diagnostics.
// v31: Wi-Fi setup uses scan/select list instead of typing SSID manually.
// v30b: recover setup AP after failed STA; preserve saved SSID/password and show AP state.
// v30a: historical diagnostic build displayed saved Wi-Fi credentials (removed in v34).
// v30: one-time Wi-Fi setup; preserves saved SSID/password, reuses blank password, tests immediately.
// v29: persistent opaque Wi-Fi/NTP status box; no guessing whether connection succeeded.
// v28e: fixes false NTP success by clearing stale 2026-01-01 system time before SNTP.
// v28b: Wi-Fi setup guidance moved to bottom so it never overlaps date/time/weather.
// v28a: non-blocking first-run Wi-Fi setup; Sound Runner display keeps running.
// v27: blank WIFI_SSID now attempts ESP32 saved Wi-Fi credentials via WiFi.begin().
// v26b: robust NTP sync (15s), tiny weather label, no WEATHER placeholder.
// v25b: date, time, and weather text are all true-centered at screen X=120.
// v25a: clouds have a subtle light-gray 1px edge for pale-sky visibility.
// v25: date/time/weather centered; visible weather label added.
// v24: celestial bodies follow a left-horizon -> noon/midnight apex -> right-horizon arc.
/*
  SOUND TERRARIUM - inspired by retro arcade
  ------------------------------------------------
  Target: M5Stack Cardputer / Cardputer ADV
  Board:  M5Cardputer

  What this first device build includes:
    - built-in microphone -> spectral-centroid-like terrain control
    - stacked 8-color "Showa graphic EQ" terrain
    - original Sound Runner sprite family (RUN / JUMP / CLIMB / FALL / WAVE)
    - short bump -> JUMP
    - sustained steep rise -> CLIMB
    - steep drop -> FALL
    - CLIMB: approach ~5 px, then vertical climb while drifting left
      at the same speed as the terrain
    - clock with NTP when Wi-Fi is available
    - classical face Sun / Moon
    - worldwide weather via Open-Meteo (latitude/longitude)
    - offline fallback: app continues without Wi-Fi, NTP, or weather
    - song tempo controls terrain/runner scroll speed
    - silence falls back smoothly to a 60 BPM idle run
    - J jumps; W waves; U summons the UFO; T toggles clock/ephemeris overlay
    - spectrum analysis follows HTML: 80-1800 Hz
    - quiet baseline stays in cyan/blue area
    - adaptive centroid window expands Cardputer mic motion to HTML-like amplitude
    - runner motion is 75% of v1.6 while terrain scroll speed is unchanged

  Required:
    M5Stack board manager >= 3.2.2
    Board: M5Cardputer
    M5GFX >= 0.2.10

  v1.5 deliberately DOES NOT include M5Cardputer/M5Unified.
  The LCD is initialized directly with M5GFX so M5Unified never owns I2S0.

  IMPORTANT SETUP:
    Set WIFI_SSID / WIFI_PASS.
    Set LATITUDE / LONGITUDE and TZ_INFO for the place where the clock is used.
    Open-Meteo itself is worldwide; these are only the user's location settings.
*/

#include <Arduino.h>
#include <M5Cardputer.h>
#include <M5Unified.h>
#include <utility/imu/BMI270_Class.hpp>
#include <esp_heap_caps.h>
#include <Wire.h>
#include "driver/i2s_std.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

// ---------------- User settings ----------------
static const char* WIFI_SSID = "";         // blank = try saved Wi-Fi credentials from NVS first
static const char* WIFI_PASS = "";         // e.g. "password"

// Default example: Tokyo. Change these three values for another place.
static constexpr float DEFAULT_LATITUDE  = 35.6762f;
static constexpr float DEFAULT_LONGITUDE = 139.6503f;
static float locationLatitude = DEFAULT_LATITUDE;
static float locationLongitude = DEFAULT_LONGITUDE;
static String locationName = "Tokyo, Japan";
static const char* TZ_INFO = "JST-9";      // bootstrap only; Open-Meteo supplies the saved/current UTC offset

// ---------------- Display / world ----------------
static constexpr int W = 240;
static constexpr int H = 135;
static constexpr int TERRAIN_BOTTOM = 134;
static constexpr int TERRAIN_HIGH   = 70;
static constexpr int TERRAIN_LOW    = 123;
static constexpr int RUNNER_TARGET_X = 72;
// Bruce 1.16.1 Mic Spectrum uses 48 kHz on Cardputer / ADV.
static constexpr int SAMPLE_RATE = 48000;
// 512 samples = about 10.7 ms at 48 kHz: short enough for the animation loop,
// long enough for the Sound Runner spectral-centroid analysis.
static constexpr size_t AUDIO_N = 512;

// Cardputer ADV audio hardware used by Bruce 1.16.1.
static constexpr uint8_t ES8311_ADDR = 0x18;
static constexpr int ADV_I2C_SDA = 8;
static constexpr int ADV_I2C_SCL = 9;
static constexpr gpio_num_t ADV_MIC_BCLK = GPIO_NUM_41;
static constexpr gpio_num_t ADV_MIC_WS   = GPIO_NUM_43;
static constexpr gpio_num_t ADV_MIC_DIN  = GPIO_NUM_46;

// Cardputer ADV keyboard controller shares the same internal I2C bus.
static constexpr uint8_t TCA8418_ADDR = 0x34;
static constexpr uint8_t TCA_REG_CFG = 0x01;
static constexpr uint8_t TCA_REG_INT_STAT = 0x02;
static constexpr uint8_t TCA_REG_KEY_LCK_EC = 0x03;
static constexpr uint8_t TCA_REG_KEY_EVENT_A = 0x04;
static constexpr uint8_t TCA_REG_KP_GPIO_1 = 0x1D;
static constexpr uint8_t TCA_REG_KP_GPIO_2 = 0x1E;
static constexpr uint8_t TCA_REG_KP_GPIO_3 = 0x1F;

// Cardputer ADV BMI270 on the same internal I2C bus.
static constexpr uint8_t BMI270_ADDR = 0x69;
static constexpr uint8_t BMI270_ACC_X_LSB = 0x0C;
static constexpr float WORLD_TILT_LIMIT_DEG = 10.0f;
static constexpr float WORLD_TILT_INPUT_FULL_DEG = 10.0f; // device tilt that reaches full displayed +/-10 deg and starts runner slide
static constexpr float TILT_ENGAGE_DEG = 3.0f;       // intentional left/right gesture starts here
static constexpr float TILT_RELEASE_DEG = 1.5f;      // hysteresis: release only after returning well inside
static constexpr float PLAY_PITCH_LIMIT_DEG = 42.0f; // laid down / strongly pitched = not a tilt command
static constexpr float TILT_FILTER_ALPHA = 0.10f;    // smooth continuous control, no angle quantization
static constexpr uint32_t RECOVERY_HOLD_MS = 5000;
static constexpr float UFO_RESCUE_HOLD_ENTER_DEG = 45.0f; // if still this tilted when rescue arrives, keep runner aboard
static constexpr float UFO_RESCUE_HOLD_RELEASE_DEG = 40.0f; // hysteresis: release only after tilt settles below this
static constexpr float VIRTUAL_MARGIN_X = 120.0f;
static bool imuReady=false;
static float imuTiltDeg=0.0f, imuTiltSmoothDeg=0.0f, imuTiltDisplayDeg=0.0f;
static float imuPitchDeg=0.0f;
static bool tiltGestureActive=false;
static bool imuPlayPose=true;
static float imuPrevTiltDeg=0.0f;
static uint32_t imuLastReadMs=0;
static bool runnerLost=false;
static int8_t runnerLostSide=0; // -1 = left edge, +1 = right edge
static int8_t runnerLostTiltSign=0; // raw IMU sign at the instant the runner left
static uint32_t runnerOffscreenSinceMs=0;
static uint32_t recoverySinceMs=0;

// ---------------- Display ----------------
// Use the installed M5Cardputer display object ONLY as the LCD driver.
// IMPORTANT: do NOT call M5Cardputer.begin(), M5.begin(), Mic.begin(),
// or Speaker.begin(). That leaves I2S0 untouched for the Bruce-style mic path.
M5Canvas canvas(&M5Cardputer.Display);

static uint8_t terrain[W];
static float terrainSmoothY = 116.0f;

// Tempo-driven world scrolling.
// 60 BPM idle ~= 60 px/sec. 120 BPM ~= 120 px/sec, etc.
// Fractional pixels are accumulated so speed is independent of frame rate.
static constexpr float IDLE_BPM = 60.0f;
static constexpr float MIN_TRACK_BPM = 60.0f;
static constexpr float MAX_TRACK_BPM = 180.0f;
static float estimatedBpm = IDLE_BPM;
static float targetBpm = IDLE_BPM;
static float worldSpeedPxPerSec = IDLE_BPM;
static float scrollAccumulator = 0.0f;
static uint32_t lastWorldMs = 0;

// v85: last EQ-derived terrain height actually committed to the scrolling history.
// Used only to interpolate when one loop advances more than one terrain pixel.
static float lastCommittedTerrainY = 116.0f;
static bool lastCommittedTerrainValid = false;

// v91: right-side 8-band EQ is now the visible terrain generator.
// Its 54 px zone is sampled into a smooth spatial road profile and that
// completed profile is released leftward as the world scrolls.
static constexpr int EQ_GEN_BAR_W = 5;
static constexpr int EQ_GEN_GAP = 2;
static constexpr int EQ_GEN_W = 8 * EQ_GEN_BAR_W + 7 * EQ_GEN_GAP; // 54 px
static constexpr int EQ_GEN_RIGHT = W - 3;
static constexpr int EQ_GEN_LEFT = EQ_GEN_RIGHT - EQ_GEN_W;
static constexpr float EQ_TERRAIN_VALLEY_Y = 116.0f;
static constexpr float EQ_TERRAIN_SCALE = 0.80f;
static float eqGeneratorProfile[EQ_GEN_W] = {0};
static bool eqGeneratorReady = false;

// ADV keyboard state.
static bool advKeyboardReady = false;
void startUfoShift();

// 8 fixed Y-axis colors, bottom cool -> top warm
static const uint16_t EQ_COLORS[8] = {
  0x114B, // #142B5F deep navy: lowest band
  0x12F5, // #165DAB blue
  0x1CFA, // #1E9ED6 cyan-blue
  0x25B4, // #25B7A4 turquoise
  0x55C9, // #52B84A green
  0xE6A9, // #E3D64A yellow
  0xF4C7, // #F29A38 orange
  0xE247, // #E44B3A red: highest band
};

// ---------------- Runner sprites ----------------
// w = white, o = orange, b = blue, . = transparent
static const char* RUN0[11] = {
"......b...",".....www..",".....www..","....www...",
"..wwowww..",".wwbww..ww","....ww....","....www...",
".wwwwbww..","......ww..","......ww.."
};
static const char* RUN1[11] = {
"......b...",".....www..",".....www..",".....ww...",
"....www...","...wwwww..","...wwwwbww",".....www..",
"....wwww..","...www....","....ww...."
};
static const char* RUN2[11] = {
"......b...",".....www..",".....www..",".....ww...",
"...wwwwbb.","..wwowwww.",".....ww...","....wwww..",
"...ww..ww.","..ww...ww.","..ww......"
};
static const char* JUMP0[11] = {
"......b...",".....www..",".....www..","....www...",
"wwwwowww..","...bww..ww","....ww....","....wwwwww",
".wwwwb....","..........",".........."
};
static const char* FALL0[11] = {
"ww..b..ww.","wwowwwoww.","wwowwwoww.",".wwwwwww..",
"...ww.....","...ww.....","...wwww...","...wwbww..",
"...wwbww..","...wwbww..","...ww....."
};
static const char* CLIMB0[11] = {
"....ww....","....ww...o","....wwwwww",".o..www...",
".wwwwww...","....www...","....www...","...wwbww..",
"...wwbwww.","...ww.....","..www....."
};
static const char* CLIMB1[11] = {
"....ww....","b...ww....","wwwwww....","...www..b.",
"...wwwwww.","...www....","...www....","..wwoww...",
".wwwoww...",".....ww...",".....www.."
};
static const char* WAVE0[11] = {
"....ww.w..","...wwwww..","...wwww...","....ww....",
"...wwww...","..wwww....","....ww....","....ww....",
"...wwww...","...w..w...","..ww..ww.."
};
static const char* WAVE1[11] = {
"....ww..w.","...wwww.w.","...wwww.w.","....ww.w..",
"...wwww...","..wwww....","....ww....","....ww....",
"...wwww...","...w..w...","..ww..ww.."
};
static const char* WAVE2[11] = {
"....ww....","...wwww...","...wwww...","....ww....",
"...wwwwwww","..wwww....","....ww....","....ww....",
"...wwww...","...w..w...","..ww..ww.."
};

// ---------------- Runner state ----------------
enum RunnerState { RS_RUN, RS_JUMP, RS_CLIMB, RS_FALL, RS_WAVE };

struct Runner {
  float x = RUNNER_TARGET_X;
  float y = 96;
  RunnerState state = RS_RUN;
  float vy = 0;
  uint16_t anim = 0;
  uint16_t stateT = 0;
  uint8_t climbPhase = 0; // 0 approach, 1 vertical
  float climbStartX = 0;
  float climbTargetY = 0;
  int lastWaveHour = -1;
} runner;

// ---------------- UFO shift-change event ----------------
enum UfoPhase {
  UFO_IDLE, UFO_ENTER_LEFT, UFO_HOVER, UFO_BEAM_GROW, UFO_BEAM_CONTACT,
  UFO_ABDUCT, UFO_EXIT_RIGHT, UFO_RETURN_RIGHT, UFO_RETURN_LEFT,
  UFO_DROP_BEAM, UFO_DROP, UFO_LEAVE_LEFT
};

struct UfoState {
  bool active=false;
  UfoPhase phase=UFO_IDLE;
  float x=-30, y=23;
  float targetX=0;
  uint16_t phaseT=0;
  float beam=0;
  float beamProgress=0;
  int beamGroundY=0;
  uint32_t lastStepMs=0;
  int lastScheduleDay=-1;
  int lastScheduleHour=-1;
  bool lostRunnerRescue=false;
  bool rescueTiltHold=false;
} ufo;

bool ufoOwnsRunner(){
  return ufo.active && (ufo.phase>=UFO_BEAM_CONTACT && ufo.phase<=UFO_DROP);
}

// ---------------- Environment ----------------
enum WeatherMode { WX_DEFAULT, WX_CLEAR, WX_CLOUDY, WX_RAIN, WX_SNOW, WX_THUNDER };
WeatherMode modeFromWMO(int code);  // explicit prototype: avoids Arduino auto-prototype being emitted before WeatherMode

struct WeatherState {
  WeatherMode mode = WX_DEFAULT;
  int cloud = 35;
  int code = -1;
  bool online = false;
  uint32_t updatedMs = 0;
  float temperatureC = NAN;
  int humidityPct = -1;
  float pressureMslHpa = NAN;
  int precipitationProbabilityPct = -1;
} weather;

struct TideState {
  bool valid = false;
  bool rising = false;
  time_t nextHighEpoch = 0;
  time_t nextLowEpoch = 0;
  uint32_t updatedMs = 0;
} tide;


Preferences prefs;
Preferences wifiPrefs;
WebServer wifiSetupServer(80);
bool wifiSetupMode=false;
uint32_t lastWiFiRetryMs=0;

enum NetStage { NET_NO_WIFI, NET_CONNECTING, NET_WIFI_OK, NET_NTP_OK, NET_NTP_FAIL, NET_SETUP };
NetStage netStage=NET_NO_WIFI;

// /save must return immediately. Connection testing and location/weather refresh
// are deferred to loop() so the HTTP handler itself never blocks for tens of seconds.
enum PendingSetupStage { PSET_IDLE, PSET_START, PSET_WAIT_WIFI, PSET_NTP, PSET_LOCATION, PSET_WEATHER, PSET_TIDE, PSET_EPHEMERIS, PSET_DONE };
static PendingSetupStage pendingSetupStage=PSET_IDLE;
static String pendingSetupSsid;
static String pendingSetupPass;
static String pendingSetupCity;
static uint32_t pendingSetupStartedMs=0;
static uint32_t pendingSetupResponseSentMs=0;
static bool pendingLocationChanged=false;

void connectWiFi(bool allowSetup=true);
static void startWiFiRetryNonBlocking();
static void serviceWiFiRetryNonBlocking();

const char* netStageText(){
  switch(netStage){
    case NET_CONNECTING: return "WiFi CONNECTING";
    case NET_WIFI_OK:    return "WiFi OK / NTP...";
    case NET_NTP_OK:     return "WiFi OK / TIME OK";
    case NET_NTP_FAIL:   return "WiFi OK / NTP FAIL";
    case NET_SETUP:      return "WiFi SETUP";
    default:             return "WiFi OFF";
  }
}

static const char* SETUP_AP_SSID="SOUND-TERRARIUM-SETUP";

bool wifiOK = false;
bool ntpOK = false;
static uint32_t lastNtpAttemptMs=0;
static constexpr uint32_t NTP_RETRY_MS=5UL*60UL*1000UL;
static constexpr uint32_t WIFI_RETRY_MS=2UL*60UL*1000UL;

// Runtime reconnect is deliberately non-blocking so offline use never freezes the scene.
enum WiFiRetryStage { WRT_IDLE, WRT_TRY_LAST, WRT_SCAN_WAIT, WRT_TRY_BEST };
static WiFiRetryStage wifiRetryStage=WRT_IDLE;
static uint32_t wifiRetryStageStartedMs=0;
static int wifiRetryBestSlot=-1;
static String wifiRetryBestSsid;
static String wifiRetryBestPass;

time_t fallbackEpoch = 1767268800; // safe neutral fallback; exact date is not important
uint32_t fallbackMillis0 = 0;
uint32_t lastWeatherAttempt = 0;
uint32_t lastEpochSaveMs = 0;

// Daily Sun/Moon ephemeris. A successful fetch is tagged with the local
// YYYYMMDD. If today's data is unavailable, retry once per hour.
static uint32_t lastEphemerisAttemptMs = 0;
static constexpr uint32_t EPHEMERIS_RETRY_MS = 60UL*60UL*1000UL;
static int ephemerisDateKey = 0;

bool weatherOK = false;
String weatherDiag = "WEATHER...";
int lastHttpCode = 0;

// v97 solar schedule. Open-Meteo returns local sunrise/sunset when timezone=auto.
// If no online value has ever been stored, fall back to 06:00 / 18:00.
static int sunriseMinutes = 6 * 60;
static int sunsetMinutes  = 18 * 60;
static int32_t localUtcOffsetSeconds = 0;
static bool solarScheduleValid = false;

// v98 lunar rise/set schedule. These are local clock minutes returned by Open-Meteo.
// The pair may cross midnight (moonriseMinutes > moonsetMinutes).
static int moonriseMinutes = 18 * 60;
static int moonsetMinutes  = 6 * 60;
static bool lunarScheduleValid = false;

// T toggles only the date/time/weather block. Networking, NTP, weather and
// celestial calculations continue normally while the text is hidden.
static bool showClockInfo = true;
static bool showAuxInfo = false; // I: ephemeris/location/tide/AP; hidden by default

static uint32_t lastTideAttemptMs = 0;
static constexpr uint32_t TIDE_REFRESH_MS = 6UL*60UL*60UL*1000UL;
static constexpr uint32_t TIDE_RETRY_MS = 30UL*60UL*1000UL;

// ---------------- Audio ----------------
// IMPORTANT: this deliberately bypasses M5Cardputer.Mic.
// It follows the working Bruce 1.16.1 Cardputer ADV path:
// ES8311 setup over Wire1 + ESP-IDF new I2S channel API.
static i2s_chan_handle_t micChan = nullptr;
static int16_t audioBuf[AUDIO_N];
static bool micReady = false;
static bool audioPrimed = false;
static int16_t rawPeak = 0;
static bool recordEverSucceeded = false;

float smoothCentroid = 420.0f; // HTML-range centroid baseline
float smoothRms = 0.0f;
static bool audioPresent = false;

// Adaptive display range for the microphone spectrum.
// The browser FFT produces a much wider centroid excursion than the small
// Goertzel bank on the Cardputer.  These trackers learn the actual centroid
// range of the current music and expand it into the same visual road height.
static float centroidFloorHz = 260.0f;
static float centroidCeilHz  = 900.0f;
static float terrainNormSmooth = 0.0f;
// Curvature-limited terrain motion: the slope itself has inertia, so peaks
// become rounded hills rather than /\ spikes.
static float terrainSlope = 0.0f;
// Hill-shape memory: once a summit is crossed, do not let the downhill side
// become steeper than the uphill side that created that summit.
static float hillUphillMaxSlope = 0.0f;
static bool hillWasClimbing = false;



// Beat / tempo detector.
// Uses low-frequency spectral energy and an adaptive floor.
// The detector only changes speed when periodic beats are credible;
// otherwise the runner gently returns to the 60 BPM idle speed.
static float beatEnergyAvg = 0.0f;
static uint32_t lastBeatMs = 0;
static uint32_t lastCredibleBeatMs = 0;
static float beatIntervals[8] = {0};
static int beatIntervalCount = 0;
static int beatIntervalPos = 0;

// Denser Goertzel bank over the SAME useful range as the HTML version.
// HTML computes spectral centroid from FFT bins between 80 and 1800 Hz.
// Cardputer keeps the no-extra-FFT-library approach, but samples that range
// much more densely than v1.6 so the centroid has comparable movement.
static const float ANALYSIS_HZ[] = {
   80,  95, 112, 132, 155, 182, 214, 251,
  295, 346, 406, 477, 560, 657, 771, 905,
 1062,1247,1464,1719,1800
};
static constexpr int NFREQ = sizeof(ANALYSIS_HZ)/sizeof(ANALYSIS_HZ[0]);

// Temporary audio diagnostic only. Eight log-ish bands are built from the
// existing 21 Goertzel sample frequencies. They do NOT affect terrain, beat,
// centroid, gain, or microphone capture.
static float audioDiagBands[8] = {0};
static float audioDiagScale = 0.0001f;
// v95: road generation uses these exact visible EQ bar-top coordinates.
// They are calculated once from the same display formula as the bars.
static int visibleEqTopY[8] = {122,122,122,122,122,122,122,122};
static bool visibleEqTopReady = false;

// Human/character motion is deliberately slower than the terrain.
// v1.6 made both advance together; 0.75 keeps the current road speed
// while restoring a calmer runner animation/motion.
static constexpr float RUNNER_MOTION_SCALE = 0.75f;
static float runnerMotionPhase = 0.0f;

// ---------------- Utility ----------------
static inline int clampi(int v,int a,int b){ return v<a?a:(v>b?b:v); }
static inline float clampf(float v,float a,float b){ return v<a?a:(v>b?b:v); }

int rawGroundAt(int x){
  x=clampi(x,0,W-1);
  return terrain[x];
}

int groundAt(int x){
  // World-space terrain stays untouched. IMU tilt is a screen-space transform only.
  // Therefore a hill that was tilted immediately returns to level when the device does;
  // the tilt itself is NOT stored in the scrolling terrain history.
  return rawGroundAt(x);
}

int displayGroundAt(int x){
  x=clampi(x,0,W-1);
  float dy=tanf(imuTiltDisplayDeg*PI/180.0f)*(x-W*0.5f);
  return clampi((int)roundf(rawGroundAt(x)+dy),TERRAIN_HIGH,TERRAIN_BOTTOM);
}

float worldAdvancePixels(){
  // updateRunner() is called once for every terrain pixel actually shifted.
  return 1.0f;
}

void drawSprite(const char* const rows[11], int x, int y){
  // First pass: 1-pixel black silhouette around every opaque sprite pixel.
  // This is the same visual principle as the HTML version and keeps the
  // character readable over white sky, clouds, and bright EQ terrain.
  for(int yy=0; yy<11; ++yy){
    for(int xx=0; xx<10; ++xx){
      char c = rows[yy][xx];
      if(c=='.' || c=='\0') continue;
      for(int oy=-1; oy<=1; ++oy){
        for(int ox=-1; ox<=1; ++ox){
          if(ox==0 && oy==0) continue;
          canvas.drawPixel(x+xx+ox,y+yy+oy,TFT_BLACK);
        }
      }
    }
  }

  // Second pass: actual sprite colors on top of the outline.
  for(int yy=0; yy<11; ++yy){
    for(int xx=0; xx<10; ++xx){
      char c = rows[yy][xx];
      if(c=='.' || c=='\0') continue;
      uint16_t col = TFT_WHITE;
      if(c=='o') col = 0xFA20;
      else if(c=='b') col = 0x1C9F;
      canvas.drawPixel(x+xx,y+yy,col);
    }
  }
}

// ---------------- Time ----------------
time_t safeEpoch(){
  time_t now = time(nullptr);
  // NTP epoch sanity: after 2024-01-01
  if(now > 1704067200){
    return now;
  }
  return fallbackEpoch + (millis()-fallbackMillis0)/1000;
}

void tryNTP(){
  if(!wifiOK) return;
  const uint32_t nowMs=millis();
  if(lastNtpAttemptMs!=0 && (uint32_t)(nowMs-lastNtpAttemptMs)<NTP_RETRY_MS) return;
  lastNtpAttemptMs=nowMs;

  // IMPORTANT:
  // Cardputer/ESP32 may already hold a bogus-but-plausible date such as 2026-01-01.
  // If we only check "year > 2025", that stale value is mistaken for successful NTP.
  // Clear the system clock first, then start SNTP and wait for a genuinely new timestamp.
  timeval tv = {};
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);

  ntpOK=false;
  configTzTime(TZ_INFO, "ntp.nict.jp", "pool.ntp.org", "time.google.com");
  // Some ESP32/Arduino combinations can leave localtime() effectively in UTC
  // even though SNTP itself succeeds. Apply the POSIX TZ explicitly as well.
  setenv("TZ", TZ_INFO, 1);
  tzset();

  const uint32_t started=millis();
  while(millis()-started < 8000){
    time_t now=time(nullptr);

    // Because we explicitly zeroed the clock immediately before SNTP,
    // a modern epoch here can only have arrived from SNTP.
    if(now > 1735689600){ // 2025-01-01 UTC
      ntpOK=true;
      netStage=NET_NTP_OK;
      fallbackEpoch=now;
      fallbackMillis0=millis();
      prefs.putLong64("epoch",(int64_t)now);
      return;
    }
    delay(250);
  }

  ntpOK=false;
  netStage=NET_NTP_FAIL;
}

void localNow(struct tm &tmNow){
  // Keep the clock epoch in UTC, then apply the location offset returned by
  // Open-Meteo. Before the first successful fetch, the saved/default offset is used.
  time_t e=safeEpoch() + localUtcOffsetSeconds;
  gmtime_r(&e,&tmNow);
}

static bool parseIsoLocalMinutes(const char* iso,int &minutesOut){
  if(!iso) return false;
  const char* t=strchr(iso,'T');
  if(!t || strlen(t)<6) return false;
  int hh=(t[1]-'0')*10+(t[2]-'0');
  int mm=(t[4]-'0')*10+(t[5]-'0');
  if(hh<0 || hh>23 || mm<0 || mm>59) return false;
  minutesOut=hh*60+mm;
  return true;
}

static int minuteOfDay(const tm &t){
  return t.tm_hour*60+t.tm_min;
}

static bool isSolarDayMinute(int m){
  if(sunriseMinutes<sunsetMinutes) return m>=sunriseMinutes && m<sunsetMinutes;
  // Defensive support for unusual schedules that cross midnight.
  return m>=sunriseMinutes || m<sunsetMinutes;
}

// ---------------- Weather ----------------
WeatherMode modeFromWMO(int code){
  if(code==0) return WX_CLEAR;
  if(code>=1 && code<=3) return WX_CLOUDY;
  if(code==45 || code==48) return WX_CLOUDY;
  if((code>=51 && code<=67) || (code>=80 && code<=82)) return WX_RAIN;
  if((code>=71 && code<=77) || code==85 || code==86) return WX_SNOW;
  if(code>=95 && code<=99) return WX_THUNDER;
  return WX_DEFAULT;
}

bool extractJsonNumber(const String &s,const char* key,float &out){
  String token=String("\"")+key+"\":";
  int p=s.indexOf(token);
  if(p<0) return false;
  p += token.length();
  int e=p;
  while(e<(int)s.length()){
    char c=s[e];
    if((c>='0'&&c<='9') || c=='-' || c=='.') e++;
    else break;
  }
  if(e<=p) return false;
  out=s.substring(p,e).toFloat();
  return true;
}

void loadSavedWeather(){
  weather.code = prefs.getInt("wxcode",-1);
  weather.cloud = prefs.getInt("cloud",35);
  weather.mode = (WeatherMode)prefs.getUChar("wxmode",(uint8_t)WX_DEFAULT);
  weather.online=false;
  weather.temperatureC = prefs.getFloat("tempc", NAN);
  weather.humidityPct = prefs.getInt("humid", -1);
  weather.pressureMslHpa = prefs.getFloat("press", NAN);
  weather.precipitationProbabilityPct = prefs.getInt("pop", -1);

  tide.valid = prefs.getBool("tidevalid", false);
  tide.rising = prefs.getBool("tiderise", false);
  tide.nextHighEpoch = (time_t)prefs.getLong64("tidehigh", 0);
  tide.nextLowEpoch = (time_t)prefs.getLong64("tidelow", 0);

  locationLatitude = prefs.getFloat("loclat", DEFAULT_LATITUDE);
  locationLongitude = prefs.getFloat("loclon", DEFAULT_LONGITUDE);
  locationName = prefs.getString("locname", "Tokyo, Japan");

  int savedRise=prefs.getInt("sunriseM",-1);
  int savedSet =prefs.getInt("sunsetM",-1);
  bool savedSolarValid=prefs.getBool("sunvalid",true); // true keeps compatibility with pre-v108k saves
  if(savedSolarValid && savedRise>=0 && savedRise<1440 && savedSet>=0 && savedSet<1440 && savedRise!=savedSet){
    sunriseMinutes=savedRise;
    sunsetMinutes=savedSet;
    solarScheduleValid=true;
  }

  int savedMoonRise=prefs.getInt("moonriseM",-1);
  int savedMoonSet =prefs.getInt("moonsetM",-1);
  bool savedLunarValid=prefs.getBool("moonvalid",true); // true keeps compatibility with pre-v108k saves
  if(savedLunarValid && savedMoonRise>=0 && savedMoonRise<1440 && savedMoonSet>=0 && savedMoonSet<1440 && savedMoonRise!=savedMoonSet){
    moonriseMinutes=savedMoonRise;
    moonsetMinutes=savedMoonSet;
    lunarScheduleValid=true;
  }
  if(prefs.isKey("utcoff")){
    localUtcOffsetSeconds=prefs.getInt("utcoff",0);
  }else{
    const bool factoryTokyo =
      fabsf(locationLatitude-DEFAULT_LATITUDE)<0.01f &&
      fabsf(locationLongitude-DEFAULT_LONGITUDE)<0.01f;
    localUtcOffsetSeconds=factoryTokyo ? 9*60*60 : 0;
  }
  ephemerisDateKey=prefs.getInt("ephdate",0);
}

static int localDateKey(){
  struct tm t;
  localNow(t);
  return (t.tm_year+1900)*10000 + (t.tm_mon+1)*100 + t.tm_mday;
}

bool fetchDailyEphemeris(){
  if(WiFi.status()!=WL_CONNECTED) return false;

  HTTPClient http;
  String url =
    String("http://api.open-meteo.com/v1/forecast?latitude=")
    + String(locationLatitude,4)
    + "&longitude=" + String(locationLongitude,4)
    + "&daily=sunrise,sunset,moonrise,moonset"
    + "&forecast_days=1"
    + "&timezone=auto";

  http.setConnectTimeout(8000);
  http.setTimeout(8000);
  if(!http.begin(url)) return false;

  int code=http.GET();
  if(code!=HTTP_CODE_OK){
    http.end();
    return false;
  }

  String body=http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err=deserializeJson(doc,body);
  if(err) return false;

  JsonArray sunriseArr = doc["daily"]["sunrise"].as<JsonArray>();
  JsonArray sunsetArr  = doc["daily"]["sunset"].as<JsonArray>();
  JsonArray moonriseArr= doc["daily"]["moonrise"].as<JsonArray>();
  JsonArray moonsetArr = doc["daily"]["moonset"].as<JsonArray>();

  // Sun and Moon are deliberately independent. Open-Meteo can legitimately
  // return a missing moonrise or moonset for a calendar day; that must not
  // discard an otherwise valid sunrise/sunset update.
  bool solarValid=false;
  bool lunarValid=false;
  int srM=0,ssM=0,mrM=0,msM=0;

  if(!sunriseArr.isNull() && !sunsetArr.isNull() &&
     sunriseArr.size()>0 && sunsetArr.size()>0){
    String sr=sunriseArr[0].as<String>();
    String ss=sunsetArr[0].as<String>();
    solarValid=parseIsoLocalMinutes(sr.c_str(),srM) &&
               parseIsoLocalMinutes(ss.c_str(),ssM) &&
               srM!=ssM;
  }

  if(!moonriseArr.isNull() && !moonsetArr.isNull() &&
     moonriseArr.size()>0 && moonsetArr.size()>0){
    String mr=moonriseArr[0].as<String>();
    String ms=moonsetArr[0].as<String>();
    lunarValid=parseIsoLocalMinutes(mr.c_str(),mrM) &&
               parseIsoLocalMinutes(ms.c_str(),msM) &&
               mrM!=msM;
  }

  if(!solarValid && !lunarValid) return false;

  if(!doc["utc_offset_seconds"].isNull()){
    localUtcOffsetSeconds=doc["utc_offset_seconds"].as<int32_t>();
    prefs.putInt("utcoff",localUtcOffsetSeconds);
  }

  if(solarValid){
    sunriseMinutes=srM;
    sunsetMinutes=ssM;
    solarScheduleValid=true;
    prefs.putInt("sunriseM",sunriseMinutes);
    prefs.putInt("sunsetM",sunsetMinutes);
  }else{
    solarScheduleValid=false;
  }

  if(lunarValid){
    moonriseMinutes=mrM;
    moonsetMinutes=msM;
    lunarScheduleValid=true;
    prefs.putInt("moonriseM",moonriseMinutes);
    prefs.putInt("moonsetM",moonsetMinutes);
  }else{
    // A missing Moon event is valid astronomical data for some dates/latitudes.
    // Keep the old persisted pair as fallback storage, but do not draw it as today.
    lunarScheduleValid=false;
  }

  ephemerisDateKey=localDateKey();
  prefs.putBool("sunvalid",solarScheduleValid);
  prefs.putBool("moonvalid",lunarScheduleValid);
  prefs.putInt("ephdate",ephemerisDateKey);

  if(solarValid){
    Serial.printf("[EPHEMERIS] %08d SUN %02d:%02d-%02d:%02d",
                  ephemerisDateKey,
                  sunriseMinutes/60,sunriseMinutes%60,
                  sunsetMinutes/60,sunsetMinutes%60);
  }else{
    Serial.printf("[EPHEMERIS] %08d SUN unavailable",ephemerisDateKey);
  }
  if(lunarValid){
    Serial.printf(" MOON %02d:%02d-%02d:%02d\n",
                  moonriseMinutes/60,moonriseMinutes%60,
                  moonsetMinutes/60,moonsetMinutes%60);
  }else{
    Serial.println(" MOON unavailable");
  }
  return true;
}

void serviceDailyEphemeris(){
  if(WiFi.status()!=WL_CONNECTED) return;

  int today=localDateKey();
  if(today<=0 || ephemerisDateKey==today) return;

  uint32_t nowMs=millis();
  if(lastEphemerisAttemptMs!=0 &&
     (uint32_t)(nowMs-lastEphemerisAttemptMs)<EPHEMERIS_RETRY_MS) return;

  lastEphemerisAttemptMs=nowMs;
  if(fetchDailyEphemeris()) lastEphemerisAttemptMs=0;
}

void fetchWeather(){
  const uint32_t nowMs=millis();

  if(WiFi.status()!=WL_CONNECTED){
    weatherOK=false;
    weatherDiag="NO WIFI";
    return;
  }

  if(weatherOK && weather.updatedMs!=0 &&
     (uint32_t)(nowMs-weather.updatedMs) < 30UL*60UL*1000UL) return;
  if(lastWeatherAttempt!=0 &&
     (uint32_t)(nowMs-lastWeatherAttempt) < 60UL*1000UL) return;
  lastWeatherAttempt=nowMs;

  weatherOK=false;
  weatherDiag="WEATHER...";
  lastHttpCode=0;

  HTTPClient http;
  String url =
    String("http://api.open-meteo.com/v1/forecast?latitude=")
    + String(locationLatitude,4)
    + "&longitude=" + String(locationLongitude,4)
    + "&current=weather_code,cloud_cover,temperature_2m,relative_humidity_2m,pressure_msl"
    + "&hourly=precipitation_probability&forecast_hours=1"
    + "&timezone=auto";

  http.setConnectTimeout(8000);
  http.setTimeout(8000);

  if(!http.begin(url)){
    weatherDiag="HTTP BEGIN FAIL";
    return;
  }

  int code=http.GET();
  lastHttpCode=code;

  if(code!=HTTP_CODE_OK){
    weatherDiag="HTTP " + String(code);
    http.end();
    return;
  }

  String body=http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err=deserializeJson(doc,body);
  if(err){
    weatherDiag="JSON FAIL";
    return;
  }

  JsonVariant current=doc["current"];
  if(current.isNull() || current["weather_code"].isNull()){
    weatherDiag="NO WEATHER CODE";
    return;
  }

  weather.code=current["weather_code"].as<int>();
  weather.cloud=current["cloud_cover"] | 0;
  weather.cloud=clampi(weather.cloud,0,100);
  if(!current["temperature_2m"].isNull()) weather.temperatureC=current["temperature_2m"].as<float>();
  if(!current["relative_humidity_2m"].isNull()) weather.humidityPct=clampi(current["relative_humidity_2m"].as<int>(),0,100);
  if(!current["pressure_msl"].isNull()) weather.pressureMslHpa=current["pressure_msl"].as<float>();
  JsonArray popArr=doc["hourly"]["precipitation_probability"].as<JsonArray>();
  if(!popArr.isNull() && popArr.size()>0 && !popArr[0].isNull())
    weather.precipitationProbabilityPct=clampi(popArr[0].as<int>(),0,100);
  weather.mode=modeFromWMO(weather.code);
  weather.online=true;
  weather.updatedMs=millis();

  // Keep the timezone offset fresh from the weather response as well.
  if(!doc["utc_offset_seconds"].isNull()){
    localUtcOffsetSeconds=doc["utc_offset_seconds"].as<int32_t>();
    prefs.putInt("utcoff",localUtcOffsetSeconds);
  }

  prefs.putInt("wxcode",weather.code);
  prefs.putInt("cloud",weather.cloud);
  prefs.putUChar("wxmode",(uint8_t)weather.mode);
  if(isfinite(weather.temperatureC)) prefs.putFloat("tempc",weather.temperatureC);
  if(weather.humidityPct>=0) prefs.putInt("humid",weather.humidityPct);
  if(isfinite(weather.pressureMslHpa)) prefs.putFloat("press",weather.pressureMslHpa);
  if(weather.precipitationProbabilityPct>=0) prefs.putInt("pop",weather.precipitationProbabilityPct);

  weatherOK=true;
  weatherDiag="WEATHER OK";
}

static bool fetchTides(){
  if(WiFi.status()!=WL_CONNECTED) return false;

  const uint32_t nowMs=millis();
  if(tide.valid && tide.updatedMs!=0 && (uint32_t)(nowMs-tide.updatedMs)<TIDE_REFRESH_MS) return true;
  if(lastTideAttemptMs!=0 && (uint32_t)(nowMs-lastTideAttemptMs)<TIDE_RETRY_MS) return tide.valid;
  lastTideAttemptMs=nowMs;

  HTTPClient http;
  String url =
    String("http://marine-api.open-meteo.com/v1/marine?latitude=")
    + String(locationLatitude,4)
    + "&longitude=" + String(locationLongitude,4)
    + "&hourly=sea_level_height_msl"
    + "&past_hours=2&forecast_hours=36"
    + "&timeformat=unixtime&timezone=GMT&cell_selection=sea";

  http.setConnectTimeout(8000);
  http.setTimeout(8000);
  if(!http.begin(url)) return false;
  int code=http.GET();
  if(code!=HTTP_CODE_OK){ http.end(); return false; }
  String body=http.getString();
  http.end();

  JsonDocument doc;
  if(deserializeJson(doc,body)) return false;
  JsonArray times=doc["hourly"]["time"].as<JsonArray>();
  JsonArray levels=doc["hourly"]["sea_level_height_msl"].as<JsonArray>();
  const int n=(int)min(times.size(),levels.size());
  if(n<5) return false;

  const time_t now=safeEpoch();
  time_t nextHigh=0,nextLow=0;
  bool directionKnown=false;
  bool rising=false;

  // Determine the local trend around the present time.
  for(int i=0;i<n-1;i++){
    if(times[i].isNull() || times[i+1].isNull() || levels[i].isNull() || levels[i+1].isNull()) continue;
    time_t t0=(time_t)times[i].as<int64_t>();
    time_t t1=(time_t)times[i+1].as<int64_t>();
    if(now>=t0 && now<=t1){
      rising=levels[i+1].as<float>() >= levels[i].as<float>();
      directionKnown=true;
      break;
    }
  }

  // Find extrema. A 3-point parabolic correction improves the time estimate
  // beyond the raw one-hour grid without inventing any additional tide model.
  for(int i=1;i<n-1;i++){
    if(times[i-1].isNull() || times[i].isNull() || times[i+1].isNull() ||
       levels[i-1].isNull() || levels[i].isNull() || levels[i+1].isNull()) continue;
    const float y0=levels[i-1].as<float>();
    const float y1=levels[i].as<float>();
    const float y2=levels[i+1].as<float>();
    const bool isHigh=(y1>y0 && y1>=y2);
    const bool isLow =(y1<y0 && y1<=y2);
    if(!isHigh && !isLow) continue;

    float frac=0.0f;
    const float den=y0-2.0f*y1+y2;
    if(fabsf(den)>1e-6f){
      frac=0.5f*(y0-y2)/den;
      frac=clampf(frac,-1.0f,1.0f);
    }
    time_t te=(time_t)times[i].as<int64_t>() + (time_t)lroundf(frac*3600.0f);
    if(te<=now) continue;
    if(isHigh && nextHigh==0) nextHigh=te;
    if(isLow  && nextLow==0)  nextLow=te;
    if(nextHigh!=0 && nextLow!=0) break;
  }

  if(nextHigh==0 || nextLow==0) return false;
  tide.valid=true;
  tide.rising=directionKnown ? rising : (nextHigh<nextLow);
  tide.nextHighEpoch=nextHigh;
  tide.nextLowEpoch=nextLow;
  tide.updatedMs=millis();
  lastTideAttemptMs=0;

  prefs.putBool("tidevalid",true);
  prefs.putBool("tiderise",tide.rising);
  prefs.putLong64("tidehigh",(int64_t)tide.nextHighEpoch);
  prefs.putLong64("tidelow",(int64_t)tide.nextLowEpoch);
  return true;
}

static void serviceTides(){
  if(WiFi.status()!=WL_CONNECTED) return;
  fetchTides();
}

// ---------------- Bruce 1.16.1 ADV microphone path ----------------
static bool es8311Write(uint8_t reg, uint8_t value){
  Wire1.beginTransmission(ES8311_ADDR);
  Wire1.write(reg);
  Wire1.write(value);
  return Wire1.endTransmission() == 0;
}

static bool setupBruceADVCodecMic(){
  // EXACT enabled_bulk_data from Bruce 1.16.1:
  // boards/m5stack-cardputer/interface.cpp::_setup_codec_mic().
  bool ok = true;
  ok &= es8311Write(0x00, 0x80);
  ok &= es8311Write(0x01, 0xBA);
  ok &= es8311Write(0x02, 0x18);
  ok &= es8311Write(0x0D, 0x01);
  ok &= es8311Write(0x0E, 0x02);
  // Bruce uses 0x10 here (analog PGA 0 dB). On this ADV the PCM path is
  // alive but extremely insensitive (rubbing the mic is detectable, voice is not),
  // so raise the ES8311 capture gain while keeping the same Bruce signal path.
  //
  // REG14: LINSEL=Mic1 differential + PGAGAIN=6 => +18 dB analog PGA.
  ok &= es8311Write(0x14, 0x16);
  // REG16: ADC_SCALE=2 => +12 dB ADC scale.
  // Total added capture gain versus Bruce baseline: about +30 dB.
  ok &= es8311Write(0x16, 0x02);
  // Keep ADC digital volume at Bruce's 0 dB value to avoid unnecessary clipping.
  ok &= es8311Write(0x17, 0xBF);
  ok &= es8311Write(0x1C, 0x6A);
  delay(20);
  return ok;
}

static bool initBruceADVMicrophone(){
  // No M5Unified exists in this build, so I2S0 is untouched when we get here.
  // Bruce initializes the ADV codec on the internal I2C bus (GPIO 8/9).
  Wire1.end();
  Wire1.begin(ADV_I2C_SDA, ADV_I2C_SCL);
  delay(20);

  if(!setupBruceADVCodecMic()) return false;

  i2s_chan_config_t chanCfg =
      I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  chanCfg.dma_desc_num = 8;
  chanCfg.dma_frame_num = 124;

  esp_err_t err = i2s_new_channel(&chanCfg, nullptr, &micChan);
  if(err != ESP_OK) return false;

  i2s_std_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));

  cfg.clk_cfg.clk_src = I2S_CLK_SRC_PLL_160M;
  cfg.clk_cfg.sample_rate_hz = SAMPLE_RATE;
  cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;

  cfg.slot_cfg.data_bit_width = I2S_DATA_BIT_WIDTH_16BIT;
  cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_16BIT;
  cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_MONO;
  cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
  cfg.slot_cfg.ws_width = 16;
  cfg.slot_cfg.bit_shift = true;
  cfg.slot_cfg.left_align = true;
  cfg.slot_cfg.big_endian = false;
  cfg.slot_cfg.bit_order_lsb = false;

  cfg.gpio_cfg.mclk = I2S_GPIO_UNUSED;
  cfg.gpio_cfg.bclk = ADV_MIC_BCLK;
  cfg.gpio_cfg.ws   = ADV_MIC_WS;
  cfg.gpio_cfg.dout = I2S_GPIO_UNUSED;
  cfg.gpio_cfg.din  = ADV_MIC_DIN;
  cfg.gpio_cfg.invert_flags.mclk_inv = false;
  cfg.gpio_cfg.invert_flags.bclk_inv = false;
  cfg.gpio_cfg.invert_flags.ws_inv = false;

  err = i2s_channel_init_std_mode(micChan, &cfg);
  if(err != ESP_OK){
    i2s_del_channel(micChan);
    micChan = nullptr;
    return false;
  }

  err = i2s_channel_enable(micChan);
  if(err != ESP_OK){
    i2s_del_channel(micChan);
    micChan = nullptr;
    return false;
  }

  memset(audioBuf, 0, sizeof(audioBuf));
  return true;
}

// ---------------- ADV cursor keys ----------------
static bool tcaWrite(uint8_t reg, uint8_t value){
  Wire1.beginTransmission(TCA8418_ADDR);
  Wire1.write(reg);
  Wire1.write(value);
  return Wire1.endTransmission() == 0;
}

static int tcaRead(uint8_t reg){
  Wire1.beginTransmission(TCA8418_ADDR);
  Wire1.write(reg);
  if(Wire1.endTransmission(false) != 0) return -1;
  if(Wire1.requestFrom((int)TCA8418_ADDR, 1) != 1) return -1;
  return Wire1.read();
}

static bool initAdvCursorKeys(){
  // Same 7 x 8 matrix used by the Cardputer ADV firmware.
  Wire1.beginTransmission(TCA8418_ADDR);
  if(Wire1.endTransmission() != 0) return false;

  bool ok = true;
  ok &= tcaWrite(TCA_REG_KP_GPIO_1, 0x7F); // 7 rows
  ok &= tcaWrite(TCA_REG_KP_GPIO_2, 0xFF); // 8 columns
  ok &= tcaWrite(TCA_REG_KP_GPIO_3, 0x00);

  // Drain stale FIFO events.
  for(int i=0;i<16;i++){
    int n=tcaRead(TCA_REG_KEY_LCK_EC);
    if(n<0 || (n&0x0F)==0) break;
    (void)tcaRead(TCA_REG_KEY_EVENT_A);
  }

  // Enable key-event interrupt generation in the controller.
  int cfg=tcaRead(TCA_REG_CFG);
  if(cfg>=0) ok &= tcaWrite(TCA_REG_CFG, (uint8_t)(cfg | 0x01));
  return ok;
}

static void mapTcaRawToPhysical(uint8_t keyvalue,uint8_t &row,uint8_t &col){
  const uint8_t u=keyvalue%10; // 1..8
  const uint8_t t=keyvalue/10; // 0..6
  if(u>=1 && u<=8 && t<=6){
    const uint8_t u0=u-1;
    row=u0&0x03;
    col=(t<<1)|(u0>>2);
  }else{
    row=0xFF;
    col=0xFF;
  }
}

static void pollAdvCursorKeys(){
  if(!advKeyboardReady) return;

  // Poll the FIFO directly. This avoids M5Cardputer.begin(), which would
  // re-own board peripherals while the current microphone path is working.
  int available=tcaRead(TCA_REG_KEY_LCK_EC);
  if(available<0) return;
  int count=available&0x0F;

  while(count-->0){
    int ev=tcaRead(TCA_REG_KEY_EVENT_A);
    if(ev<=0) break;

    // Cardputer ADV firmware: bit 7 = pressed, lower 7 bits = raw key value.
    bool pressed=(ev&0x80)!=0;
    uint8_t raw=(uint8_t)(ev&0x7F);
    if(!pressed) continue;

    uint8_t row,col;
    mapTcaRawToPhysical(raw,row,col);

    // While the setup portal is open, ANY physical key is an EXIT command.
    // Consume the key here so J/W/T/U/S do not also trigger their normal actions.
    if(wifiSetupMode){
      Serial.println("[KEY] Wi-Fi SETUP -> EXIT");
      wifiSetupMode=false;
      wifiSetupServer.stop();
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      wifiOK=false;
      netStage=NET_NO_WIFI;
      wifiRetryStage=WRT_IDLE;
      lastWiFiRetryMs=0; // let loop start a non-blocking retry immediately
      continue;
    }

    // Physical keys on Cardputer ADV (zero-based physical row/column):
    // ';' / UP marking = row 2, col 11
    // '.' / DOWN marking = row 3, col 11
    // QWERTY row is 1-based here: W=2, T=5, U=7, I=8. J=row2,col8; S=row2,col3
    if(row==2 && col==8){
      if(!ufo.active && runner.state==RS_RUN){
        runner.state=RS_JUMP;
        runner.stateT=0;
        runner.y=groundAt((int)runner.x+5)-11;
      }
    }else if(row==1 && col==2){
      // Manual WAVE: reuse exactly the same RS_WAVE animation as the scheduled wave.
      if(!ufo.active && runner.state==RS_RUN){
        runner.state=RS_WAVE;
        runner.stateT=0;
        runner.y=groundAt((int)runner.x+5)-11;
      }
    }else if(row==1 && col==8){
      // I = Information: auxiliary screen labels, hidden by default.
      showAuxInfo=!showAuxInfo;
      Serial.printf("[KEY] I -> INFO %s\n",showAuxInfo?"ON":"OFF");
    }else if(row==1 && col==5){
      // T: show/hide the date + clock + weather text as one block.
      showClockInfo=!showClockInfo;
    }else if(row==1 && col==7){
      startUfoShift();
    }else if(row==2 && col==3){
      Serial.println("[KEY] S -> Wi-Fi SETUP");
      // S = Setup. Enter the Wi-Fi setup portal even while already connected.
      // Existing saved credentials are preserved; the portal can add/update APs.
      if(!wifiSetupMode){
        netStage=NET_SETUP;
        startWiFiSetupPortalNonBlocking();
      }
    }
  }

  // Clear key-event interrupt status after draining the FIFO.
  (void)tcaWrite(TCA_REG_INT_STAT,0x01);
}

// ---------------- Audio analysis ----------------
float goertzelPower(const int16_t* data,int n,float freq,float sampleRate){
  float k = 0.5f + (n*freq/sampleRate);
  float w = 2.0f*M_PI*k/n;
  float coeff = 2.0f*cosf(w);
  float q0=0,q1=0,q2=0;
  for(int i=0;i<n;i++){
    float x = data[i] / 32768.0f;
    // Hann window reduces harsh bin-to-bin hopping
    float win = 0.5f - 0.5f*cosf(2.0f*M_PI*i/(n-1));
    q0 = coeff*q1 - q2 + x*win;
    q2=q1; q1=q0;
  }
  return q1*q1 + q2*q2 - coeff*q1*q2;
}

void registerBeat(uint32_t nowMs){
  if(lastBeatMs!=0){
    uint32_t dt=nowMs-lastBeatMs;
    // Reject impossible/duplicate hits. 333..1000 ms = 60..180 BPM.
    // For faster subdivisions, fold them down into the target range.
    if(dt>=250 && dt<=1200){
      float interval=(float)dt;
      float bpm=60000.0f/interval;
      while(bpm>MAX_TRACK_BPM){ interval*=2.0f; bpm=60000.0f/interval; }
      while(bpm<MIN_TRACK_BPM && interval>260.0f){ interval*=0.5f; bpm=60000.0f/interval; }

      if(bpm>=MIN_TRACK_BPM && bpm<=MAX_TRACK_BPM){
        beatIntervals[beatIntervalPos]=interval;
        beatIntervalPos=(beatIntervalPos+1)%8;
        if(beatIntervalCount<8) beatIntervalCount++;

        // Robust average: ignore one shortest and one longest interval when possible.
        float sum=0.0f, mn=1e9f, mx=0.0f;
        for(int i=0;i<beatIntervalCount;i++){
          float v=beatIntervals[i];
          sum+=v;
          if(v<mn) mn=v;
          if(v>mx) mx=v;
        }
        int used=beatIntervalCount;
        if(beatIntervalCount>=5){
          sum-=mn; sum-=mx; used-=2;
        }

        float avgInterval=sum/fmaxf(1.0f,(float)used);
        float newBpm=60000.0f/avgInterval;
        newBpm=clampf(newBpm,MIN_TRACK_BPM,MAX_TRACK_BPM);

        // Do not jump instantly when the detector finds a new beat.
        estimatedBpm += (newBpm-estimatedBpm)*0.22f;
        targetBpm=estimatedBpm;
        lastCredibleBeatMs=nowMs;
      }
    }
  }
  lastBeatMs=nowMs;
}

void analyzeAudio(){
  if(!micReady || micChan==nullptr) return;

  size_t bytesRead = 0;
  esp_err_t err = i2s_channel_read(
      micChan,
      audioBuf,
      sizeof(audioBuf),
      &bytesRead,
      pdMS_TO_TICKS(100));

  if(err != ESP_OK || bytesRead < sizeof(int16_t) * 64) return;

  recordEverSucceeded = true;
  audioPrimed = true;
  const size_t n = bytesRead / sizeof(int16_t);

  double sumSq = 0.0;
  int32_t peak = 0;
  for(size_t i=0;i<n;i++){
    int32_t av = abs((int)audioBuf[i]);
    if(av > peak) peak = av;
    float v = audioBuf[i] / 32768.0f;
    sumSq += v*v;
  }
  rawPeak = (int16_t)min(peak, (int32_t)32767);
  float rms = sqrtf(sumSq / (float)n);
  smoothRms += (rms - smoothRms) * 0.18f;

  // Same Sound Runner concept as the HTML prototype:
  // spectral distribution -> centroid -> terrain height.
  double weighted = 0.0;
  double mass = 0.0;
  float lowBeatEnergy = 0.0f;
  float diagAccum[8] = {0};
  int diagCount[8] = {0};

  for(int i=0;i<NFREQ;i++){
    float p = goertzelPower(audioBuf, (int)n, ANALYSIS_HZ[i], SAMPLE_RATE);
    float m = sqrtf(fmaxf(p, 0.0f));
    weighted += ANALYSIS_HZ[i] * m;
    mass += m;

    // The frequency list is approximately logarithmic, so grouping successive
    // entries gives eight useful low->high diagnostic bands.
    int db=(i*8)/NFREQ;
    if(db<0) db=0;
    if(db>7) db=7;
    diagAccum[db]+=m;
    diagCount[db]++;

    // Kick/bass emphasis for tempo detection; independent of terrain mapping.
    if(ANALYSIS_HZ[i] <= 330.0f) lowBeatEnergy += m;
  }

  float diagFrameMax=0.0f;
  for(int b=0;b<8;b++){
    float v=(diagCount[b]>0)?(diagAccum[b]/(float)diagCount[b]):0.0f;
    audioDiagBands[b] += (v-audioDiagBands[b])*0.28f;
    if(audioDiagBands[b]>diagFrameMax) diagFrameMax=audioDiagBands[b];
  }
  // Fast attack, slow decay. This keeps the bars comparable between music and
  // chassis movement instead of renormalizing every single frame.
  audioDiagScale=fmaxf(diagFrameMax,audioDiagScale*0.992f);
  if(audioDiagScale<0.0001f) audioDiagScale=0.0001f;

  if(rawPeak > 8 && mass > 0.0001){
    float centroid = (float)(weighted / mass);
    smoothCentroid += (centroid - smoothCentroid) * 0.10f;
  }

  // Adaptive onset detector. Fast enough to catch beats, slow enough that
  // sustained bass does not continuously trigger.
  if(beatEnergyAvg<=0.0f) beatEnergyAvg=lowBeatEnergy;
  beatEnergyAvg += (lowBeatEnergy-beatEnergyAvg)*0.025f;

  const uint32_t nowMs=millis();
  const bool audioActive = (smoothRms > 0.00018f) || (rawPeak > 12);
  audioPresent = audioActive;
  const bool beatHit =
      audioActive &&
      lowBeatEnergy > (beatEnergyAvg*1.55f + 0.00005f) &&
      (lastBeatMs==0 || nowMs-lastBeatMs>=250);

  if(beatHit) registerBeat(nowMs);

  // If rhythmic audio disappears, do not stop: return gently to 60 BPM.
  if(!audioActive || (lastCredibleBeatMs!=0 && nowMs-lastCredibleBeatMs>3500)){
    targetBpm=IDLE_BPM;
    beatIntervalCount=0;
    beatIntervalPos=0;
  }

  targetBpm=clampf(targetBpm,IDLE_BPM,MAX_TRACK_BPM);
  worldSpeedPxPerSec += (targetBpm-worldSpeedPxPerSec)*0.025f;
}

int centroidToGround(float hz){
  // Kept as an absolute fallback/reference mapper.
  const float lo=80.0f, hi=1800.0f;
  hz=clampf(hz,lo,hi);
  float n=logf(hz/lo)/logf(hi/lo);
  return (int)roundf(TERRAIN_LOW - n*(TERRAIN_LOW-TERRAIN_HIGH));
}


// ---------------- Terrain ----------------
float adaptiveTerrainLevel(){
  const float c = clampf(smoothCentroid,80.0f,1800.0f);

  // Fast expansion when a new low/high is actually heard.
  // Very slow relaxation in the opposite direction lets the window follow
  // a new song without collapsing the visible amplitude every moment.
  if(c < centroidFloorHz) centroidFloorHz += (c-centroidFloorHz)*0.10f;
  else                    centroidFloorHz += (c-centroidFloorHz)*0.00035f;

  if(c > centroidCeilHz)  centroidCeilHz  += (c-centroidCeilHz)*0.10f;
  else                    centroidCeilHz  += (c-centroidCeilHz)*0.00055f;

  centroidFloorHz=clampf(centroidFloorHz,80.0f,1500.0f);
  centroidCeilHz =clampf(centroidCeilHz,180.0f,1800.0f);

  // Do not allow the adaptive window to become so wide that movement disappears.
  // 220 Hz is deliberately much narrower than the full 80-1800 Hz analysis band:
  // this is what converts the Cardputer's smaller centroid excursion into the
  // browser-like visual excursion the user actually sees.
  float span=centroidCeilHz-centroidFloorHz;
  const float MIN_SPAN_HZ=220.0f;
  if(span < MIN_SPAN_HZ) span=MIN_SPAN_HZ;

  float n=(c-centroidFloorHz)/span;
  n=clampf(n,0.0f,1.0f);

  // Make mid-level changes visible instead of spending most frames near zero.
  n=powf(n,0.78f);

  // Smooth only enough to avoid one-frame spikes; retain visible musical motion.
  // HTML-like temporal smoothing: broaden musical hills and valleys instead of
  // turning short spectral changes into needle-like peaks.
  // HTML-like symmetric temporal smoothing.
  // The previous adaptive signal could fall away too quickly after a peak,
  // making the downhill side look like a cliff.  Keep the same normalized
  // target/range, but let both ascent and descent follow it with the same
  // gentle inertia before the existing 0.14 spatial terrain interpolation.
  const float HTML_SIGNAL_ALPHA = 0.075f;
  terrainNormSmooth += (n-terrainNormSmooth)*HTML_SIGNAL_ALPHA;
  return clampf(terrainNormSmooth,0.0f,1.0f);
}

static void updateVisibleEqTopY(){
  // This is now the single source of truth for BOTH the visible EQ and road.
  // If the displayed bar does not move, the road receives no different height.
  const int horizonY = H - 13;
  const int maxH = 27;
  for(int b=0;b<8;b++){
    float n=(audioDiagScale>0.0f)?audioDiagBands[b]/audioDiagScale:0.0f;
    n=clampf(n,0.0f,1.0f);
    int h=6+(int)roundf(n*(maxH-6));
    visibleEqTopY[b]=horizonY-h;
  }
  visibleEqTopReady=true;
}

static float eqBandTerrainY(int b){
  b=clampi(b,0,7);
  float n=(audioDiagScale>0.0f)?audioDiagBands[b]/audioDiagScale:0.0f;
  n=clampf(n,0.0f,1.0f);
  return EQ_TERRAIN_VALLEY_Y -
         n*(EQ_TERRAIN_VALLEY_Y-(float)TERRAIN_HIGH)*EQ_TERRAIN_SCALE;
}

static float catmullRom1D(float p0,float p1,float p2,float p3,float t){
  float t2=t*t;
  float t3=t2*t;
  return 0.5f*((2.0f*p1) +
               (-p0+p2)*t +
               (2.0f*p0-5.0f*p1+4.0f*p2-p3)*t2 +
               (-p0+3.0f*p1-3.0f*p2+p3)*t3);
}

static void updateEqGeneratorProfile(){
  // v93: each EQ BAR CENTER is one terrain control point.
  // Bar width is no longer copied into the road. A Catmull-Rom curve passes
  // through the eight centers, removing the stair-step/block-width character.
  float cx[8];
  float cy[8];
  for(int b=0;b<8;b++){
    cx[b]=(float)(b*(EQ_GEN_BAR_W+EQ_GEN_GAP) + EQ_GEN_BAR_W/2);
    cy[b]=eqBandTerrainY(b);
  }

  for(int lx=0;lx<EQ_GEN_W;lx++){
    float x=(float)lx;

    if(x<=cx[0]){
      eqGeneratorProfile[lx]=cy[0];
      continue;
    }
    if(x>=cx[7]){
      eqGeneratorProfile[lx]=cy[7];
      continue;
    }

    int seg=0;
    while(seg<6 && x>cx[seg+1]) seg++;

    float span=cx[seg+1]-cx[seg];
    float t=(span>0.0f)?(x-cx[seg])/span:0.0f;
    t=clampf(t,0.0f,1.0f);

    float p0=cy[(seg>0)?seg-1:seg];
    float p1=cy[seg];
    float p2=cy[seg+1];
    float p3=cy[(seg+2<8)?seg+2:seg+1];

    float y=catmullRom1D(p0,p1,p2,p3,t);
    eqGeneratorProfile[lx]=clampf(y,(float)TERRAIN_HIGH,EQ_TERRAIN_VALLEY_Y);
  }
  eqGeneratorReady=true;
}

float terrainTargetFromEqMax(){
  // v84 CLEAN PATH:
  // The terrain height is taken DIRECTLY from the tallest of the same eight
  // smoothed/normalized bars that are visible in the colorful Audio City.
  //
  // No centroid mapping.
  // No learned frequency range.
  // No pow() response curve.
  // No audioPresent gate.
  // No extra terrain smoothing.
  // No terrain gain multiplier.
  float maxBand=0.0f;
  for(int b=0;b<8;b++){
    if(audioDiagBands[b]>maxBand) maxBand=audioDiagBands[b];
  }

  float n=(audioDiagScale>0.0f)?(maxBand/audioDiagScale):0.0f;
  n=clampf(n,0.0f,1.0f);

  // Fixed visual mapping only: silent/short EQ -> valley, tallest EQ -> high hill.
  // This is not an audio modifier; it only converts the 0..1 visible bar height
  // to LCD Y coordinates.
  const float valleyY=116.0f;
  const float summitY=(float)TERRAIN_HIGH;
  const float TERRAIN_AMPLITUDE_SCALE=0.80f; // v86: reduce only visible terrain excursion by 20%
  return valleyY - n*(valleyY-summitY)*TERRAIN_AMPLITUDE_SCALE;
}

void updateRunner(bool motionTick); // forward declaration

void shiftTerrainOnePixel(float targetY){
  for(int x=0;x<W-1;x++) terrain[x]=terrain[x+1];

  // v85: targetY is already the interpolated EQ-derived height for THIS pixel.
  // Do not add smoothing or slope shaping here.
  terrain[W-1]=(uint8_t)clampi((int)roundf(targetY),TERRAIN_HIGH,TERRAIN_LOW);
}



void updateWorld(){
  uint32_t now=millis();
  if(lastWorldMs==0){
    lastWorldMs=now;
    return;
  }

  uint32_t dt=now-lastWorldMs;
  lastWorldMs=now;
  if(dt>80) dt=80;

  // Freeze the eight visible bar tops for this frame before terrain processing.
  updateVisibleEqTopY();

  scrollAccumulator += worldSpeedPxPerSec*((float)dt/1000.0f);
  int pixelsToShift=(int)floorf(scrollAccumulator);
  if(pixelsToShift>6) pixelsToShift=6;
  if(pixelsToShift<=0) return;

  // v94 model:
  // The road is born at the RIGHTMOST red EQ station, then moves right->left.
  // As it passes orange, yellow, green ... navy, each station may PUSH the
  // existing road upward. A station never erases a hill already made by a
  // previous station. This makes all eight bands contribute to the final road.
  //
  // Each station influences a small neighborhood with a cosine-shaped blend,
  // so the road joins the station center by a curve rather than a flat bar-width
  // step.
  const int stationCenter[8]={
    EQ_GEN_LEFT + 0*(EQ_GEN_BAR_W+EQ_GEN_GAP) + EQ_GEN_BAR_W/2,
    EQ_GEN_LEFT + 1*(EQ_GEN_BAR_W+EQ_GEN_GAP) + EQ_GEN_BAR_W/2,
    EQ_GEN_LEFT + 2*(EQ_GEN_BAR_W+EQ_GEN_GAP) + EQ_GEN_BAR_W/2,
    EQ_GEN_LEFT + 3*(EQ_GEN_BAR_W+EQ_GEN_GAP) + EQ_GEN_BAR_W/2,
    EQ_GEN_LEFT + 4*(EQ_GEN_BAR_W+EQ_GEN_GAP) + EQ_GEN_BAR_W/2,
    EQ_GEN_LEFT + 5*(EQ_GEN_BAR_W+EQ_GEN_GAP) + EQ_GEN_BAR_W/2,
    EQ_GEN_LEFT + 6*(EQ_GEN_BAR_W+EQ_GEN_GAP) + EQ_GEN_BAR_W/2,
    EQ_GEN_LEFT + 7*(EQ_GEN_BAR_W+EQ_GEN_GAP) + EQ_GEN_BAR_W/2
  };
  const int influenceRadius=5;

  for(int k=0;k<pixelsToShift;k++){
    scrollAccumulator-=1.0f;

    // Existing road keeps its shape and moves one pixel left.
    for(int x=0;x<W-1;x++) terrain[x]=terrain[x+1];

    // New road is born at the red/rightmost station height.
    float redY=visibleEqTopReady?(float)visibleEqTopY[7]:EQ_TERRAIN_VALLEY_Y;
    terrain[W-1]=(uint8_t)clampi((int)roundf(redY),TERRAIN_HIGH,TERRAIN_LOW);

    // Process stations from right (red) toward left (navy), matching travel.
    for(int b=7;b>=0;b--){
      int cx=stationCenter[b];
      float targetY=visibleEqTopReady?(float)visibleEqTopY[b]:EQ_TERRAIN_VALLEY_Y;

      // "Push upward only": smaller screen Y means a higher road.
      // At the exact bar center the road reaches the band's current height
      // if that is higher than its existing shape. Around it, blend the push
      // smoothly back into the already-created road.
      for(int dx=-influenceRadius;dx<=influenceRadius;dx++){
        int x=cx+dx;
        if(x<0 || x>=W) continue;

        float oldY=(float)terrain[x];
        if(targetY>=oldY) continue; // this station does not push downward

        float d=fabsf((float)dx)/(float)influenceRadius;
        float w=0.5f*(1.0f+cosf(d*PI)); // 1 at center -> 0 at edges
        float pushedY=oldY+(targetY-oldY)*w;
        terrain[x]=(uint8_t)clampi((int)roundf(pushedY),
                                   TERRAIN_HIGH,TERRAIN_LOW);
      }
    }

    runnerMotionPhase += RUNNER_MOTION_SCALE;
    bool motionTick=false;
    if(runnerMotionPhase>=1.0f){
      runnerMotionPhase-=1.0f;
      motionTick=true;
    }
    updateRunner(motionTick);
  }
}





// ---------------- ADV IMU / physical world ----------------
static bool bmiReadAccelRaw(int16_t &ax,int16_t &ay,int16_t &az){
  Wire1.beginTransmission(BMI270_ADDR);
  Wire1.write(BMI270_ACC_X_LSB);
  if(Wire1.endTransmission(false)!=0) return false;
  if(Wire1.requestFrom((int)BMI270_ADDR,6)!=6) return false;
  uint8_t b[6];
  for(int i=0;i<6;i++) b[i]=Wire1.read();
  ax=(int16_t)(((uint16_t)b[1]<<8)|b[0]);
  ay=(int16_t)(((uint16_t)b[3]<<8)|b[2]);
  az=(int16_t)(((uint16_t)b[5]<<8)|b[4]);
  return true;
}

static bool initAdvImuOnce(){
  // M5Unified uploads the BMI270 Bosch config, but we deliberately do NOT call
  // M5.begin()/M5Cardputer.begin(), so the proven Bruce I2S0 microphone path is untouched.
  M5Cardputer.In_I2C.begin((i2c_port_t)I2C_NUM_1,ADV_I2C_SDA,ADV_I2C_SCL);
  m5::BMI270_Class bmi(BMI270_ADDR,400000,&M5Cardputer.In_I2C);
  bool ok=(bool)bmi.begin(&M5Cardputer.In_I2C);

  // Return the shared GPIO8/9 bus to the same Wire1 owner used by Sound Runner.
  Wire1.end();
  Wire1.begin(ADV_I2C_SDA,ADV_I2C_SCL);
  delay(5);
  return ok;
}

static void startLostRunnerRescue(){
  if(ufo.active) return;

  // Rescue from the same side where the runner disappeared.
  // Left loss  -> UFO enters from the left.
  // Right loss -> UFO enters from the right (legacy behavior).
  const int8_t lostSide=runnerLostSide;

  ufo.active=true;
  ufo.lostRunnerRescue=true;
  ufo.rescueTiltHold=false;
  ufo.phase=(lostSide<0)?UFO_RETURN_LEFT:UFO_RETURN_RIGHT; // runner is already gone
  ufo.x=(lostSide<0)?-30.0f:(float)W+30.0f;
  ufo.y=23;
  ufo.targetX=RUNNER_TARGET_X+5;
  ufo.phaseT=0;
  ufo.beam=0;
  ufo.beamProgress=0;
  ufo.lastStepMs=millis();

  // Keep the missing runner off-screen while the UFO approaches.
  runner.x=(lostSide<0)?-35.0f:(float)W+35.0f;
  runner.y=-20;

  runnerLost=false; // UFO now owns the recovery sequence
  runnerLostSide=0;
  runnerLostTiltSign=0;
}

static void updateAdvImu(){
  if(!imuReady) return;
  uint32_t now=millis();
  if(now-imuLastReadMs<20) return;
  imuLastReadMs=now;

  int16_t ax,ay,az;
  if(!bmiReadAccelRaw(ax,ay,az)) return;

  // Roll is the left/right control axis. Pitch is used only to reject poses such
  // as laying the Cardputer down or strongly tipping it toward/away from the user.
  float rollAbs=atan2f((float)ax,(float)az)*180.0f/PI;
  float pitch=atan2f((float)ay,sqrtf((float)ax*ax+(float)az*az))*180.0f/PI;
  // Use the absolute left/right angle from the gravity vector.
  // Do not redefine the startup pose as zero: if the unit is physically tilted,
  // that tilt must remain visible to the controller.
  imuTiltDeg=rollAbs;
  imuPitchDeg=pitch;

  // Small sensor noise is handled by the engage/release deadband below.
  imuPlayPose=(fabsf(imuPitchDeg)<=PLAY_PITCH_LIMIT_DEG);

  bool justEngaged=false;
  if(!imuPlayPose){
    tiltGestureActive=false;
  }else if(!tiltGestureActive){
    if(fabsf(imuTiltDeg)>=TILT_ENGAGE_DEG){
      tiltGestureActive=true;
      justEngaged=true;
    }
  }else{
    if(fabsf(imuTiltDeg)<=TILT_RELEASE_DEG) tiltGestureActive=false;
  }

  float target=0.0f;
  if(imuPlayPose){
    float sign=(imuTiltDeg>=0.0f)?1.0f:-1.0f;
    float mag=fabsf(imuTiltDeg);
    // ANGLE decides what the game does. Speed is NOT an activation condition.
    // 0..3 deg: neutral; 3..10 deg: ground 0..10 deg; >=10 deg: ground stays at 10 deg.
    float span=fmaxf(1.0f, WORLD_TILT_INPUT_FULL_DEG-TILT_ENGAGE_DEG);
    float n=clampf((mag-TILT_ENGAGE_DEG)/span,0.0f,1.0f);
    target=sign*(n*WORLD_TILT_LIMIT_DEG);
  }

  // Tilt SPEED affects only how quickly the DISPLAY catches the target angle.
  // It never decides whether ground tilt or runner motion is active.
  float tiltSpeed=fabsf(imuTiltDeg-imuPrevTiltDeg);
  imuPrevTiltDeg=imuTiltDeg;

  float alpha=TILT_FILTER_ALPHA;          // normal smooth tracking
  if(justEngaged) alpha=0.42f;            // immediate visible start
  if(tiltSpeed>=7.0f) alpha=fmaxf(alpha,0.48f);  // sharp large gesture
  else if(tiltSpeed>=3.0f) alpha=fmaxf(alpha,0.26f);

  imuTiltSmoothDeg+=(target-imuTiltSmoothDeg)*alpha;

  // Near neutral, ease cleanly back to level. This depends only on angle.
  if(fabsf(imuTiltDeg)<=TILT_ENGAGE_DEG){
    if(fabsf(imuTiltSmoothDeg)<0.15f) imuTiltSmoothDeg=0.0f;
  }

  imuTiltDisplayDeg=clampf(imuTiltSmoothDeg,-WORLD_TILT_LIMIT_DEG,WORLD_TILT_LIMIT_DEG);

}

static bool updateTiltPhysics(bool motionTick){
  if(!imuReady || ufo.active) return runnerLost;

  // Laying the unit down is not a game command. Let the world ease level and
  // suspend tilt physics until the unit returns to a playable pose.
  if(!imuPlayPose){
    return runnerLost;
  }

  float a=fabsf(imuTiltSmoothDeg);
  float dir=(imuTiltSmoothDeg>0)?1.0f:-1.0f;
  float rawTilt=fabsf(imuTiltDeg);

  if(runnerLost) return true; // per-loop rescue handler owns off-screen motion


  // Display tilt is intentionally capped at +/-10 degrees, so it can never be
  // used to decide when the runner should start sliding.  Use the RAW device
  // tilt instead.  Around 14 degrees the ground has reached its full visual
  // angle; any additional physical tilt becomes lateral gravity on the runner.
  if(rawTilt<=WORLD_TILT_INPUT_FULL_DEG) return false;

  float excess=rawTilt-WORLD_TILT_INPUT_FULL_DEG;
  // 10 deg: gentle slide; 15 deg: clear slide; 20+ deg: progressively faster.
  float slide=0.18f+fminf(2.4f,excess*0.11f);
  if(motionTick) runner.anim++;

  // Keep the movement direction tied to the physical tilt direction.
  // We use raw IMU sign here so the runner starts moving even while the
  // displayed ground remains visually capped at +/-10 degrees.
  float rawDir=(imuTiltDeg>0)?1.0f:-1.0f;
  // Verified on the user's Cardputer ADV: physical LEFT corresponds to the
  // opposite raw roll sign from the assumption used in v73.
  // Invert raw roll here so physical LEFT -> screen LEFT, RIGHT -> RIGHT.
  runner.x-=rawDir*slide;
  runner.y=groundAt((int)roundf(clampf(runner.x,0.0f,(float)(W-1)))+5)-11;

  if(runner.x<-8 || runner.x>W+8){
    runnerLost=true;
    runnerLostSide=(runner.x<0.0f)?-1:1;
    runnerLostTiltSign=(imuTiltDeg>=0.0f)?1:-1;

    // Once the runner has just disappeared, keep it immediately outside the
    // visible edge.  Continuing to tilt cannot send it farther into a hidden
    // virtual space, so a reversal can always bring it back promptly.
    runner.x=(runnerLostSide<0)?-8.0f:(float)W+8.0f;

    runnerOffscreenSinceMs=millis();
    recoverySinceMs=0;
  }
  return true;
}

// ---------------- Runner mechanics ----------------
void startClimb(){
  runner.state=RS_CLIMB;
  runner.stateT=0;
  runner.climbPhase=0;
  runner.climbStartX=runner.x;
  runner.climbTargetY=groundAt((int)runner.x+14)-11;
}

static void updateOffscreenRunnerRescue(){
  if(!runnerLost || ufo.active) return;

  uint32_t now=millis();

  // The design is simple and deterministic:
  //   - for the first 5 seconds, the user may recover the runner by easing or reversing tilt;
  //   - once 5 seconds have elapsed, UFO rescue starts unconditionally.
  // The automatic rescue must never depend on IMU neutral calibration, pitch,
  // tiltGestureActive, or a second hold timer.
  if((uint32_t)(now-runnerOffscreenSinceMs)>=RECOVERY_HOLD_MS){
    recoverySinceMs=0;
    startLostRunnerRescue();
    return;
  }

  // Self-recovery during the grace period requires a working IMU.
  if(!imuReady) return;

  float tiltMag=fabsf(imuTiltDeg);
  int8_t currentTiltSign=(imuTiltDeg>=0.0f)?1:-1;

  // Once the runner is already off-screen, recovery must NOT be blocked by
  // imuPlayPose/pitch. Putting the unit back toward its normal attitude should
  // always be allowed to recover the runner during the grace period.
  bool releasedSlide = tiltMag < WORLD_TILT_INPUT_FULL_DEG;
  bool oppositeTilt =
    (tiltMag>=TILT_ENGAGE_DEG) &&
    runnerLostTiltSign!=0 &&
    currentTiltSign==-runnerLostTiltSign;

  if(releasedSlide || oppositeTilt){
    int8_t side=runnerLostSide;

    // Make the re-entry visible in this very frame.
    runnerLost=false;
    runnerLostSide=0;
    runnerLostTiltSign=0;
    recoverySinceMs=0;
    runner.x=(side<0)?1.0f:(float)(W-11);
    runner.y=groundAt((int)runner.x+5)-11;
    runner.state=RS_RUN;
    runner.stateT=0;
    return;
  }

  // Still in the slide zone: pin immediately outside the same edge.
  runner.x=(runnerLostSide<0)?-8.0f:(float)W+8.0f;
}

void updateRunner(bool motionTick){
  if(ufoOwnsRunner()) return;

  // IMPORTANT: updateTiltPhysics() must run even while runnerLost is true.
  // v60 returned here before the off-screen rescue branch could execute,
  // so reversing the Cardputer could never bring the runner back.
  if(updateTiltPhysics(motionTick)) return;
  if(runnerLost) return;

  if(motionTick){
    runner.anim++;
    runner.stateT++;
  }

  int gx=groundAt((int)runner.x+5);
  int y0=gx;
  int y1=groundAt((int)runner.x+9);
  int y2=groundAt((int)runner.x+14);
  int y3=groundAt((int)runner.x+20);

  if(runner.state==RS_RUN){
    // Ease back toward the usual screen position after a climb/fall.
    if(runner.x<RUNNER_TARGET_X) runner.x=fminf((float)RUNNER_TARGET_X,runner.x+0.35f*RUNNER_MOTION_SCALE);
    if(runner.x>RUNNER_TARGET_X) runner.x=fmaxf((float)RUNNER_TARGET_X,runner.x-0.20f*RUNNER_MOTION_SCALE);
    runner.y=gx-11;

    float rise1=y0-y1;
    float rise2=y0-y2;
    float return3=y3-y0;
    bool smallBump = rise1>2.0f && rise2>3.0f &&
                     fabsf(return3)<3.0f && (y3-y2)>2.0f;

    if(smallBump){
      runner.state=RS_JUMP;
      runner.stateT=0;
      return;
    }

    float step=y1-y0;
    if(step>5.0f){
      runner.state=RS_FALL;
      runner.vy=0.2f;
      runner.stateT=0;
      return;
    }
    // v23b: climbing disabled. RUN pose follows even near-vertical spikes.

    struct tm t;
    localNow(t);
    if(t.tm_min==0 && t.tm_hour!=runner.lastWaveHour){
      runner.lastWaveHour=t.tm_hour;
      runner.state=RS_WAVE;
      runner.stateT=0;
    }
  }
  else if(runner.state==RS_JUMP){
    const float duration=22.0f;
    float u=fminf(1.0f,runner.stateT/duration);
    float lift=sinf(M_PI*u)*7.0f;
    runner.y=groundAt((int)runner.x+5)-11-lift;
    if(runner.stateT>=duration){
      runner.state=RS_RUN;
      runner.stateT=0;
    }
  }
  else if(runner.state==RS_FALL){
    runner.vy=fminf(1.8f,runner.vy+0.10f*RUNNER_MOTION_SCALE);
    runner.y+=runner.vy*RUNNER_MOTION_SCALE;
    int g=groundAt((int)runner.x+5);
    if(runner.y+11>=g){
      runner.y=g-11;
      runner.vy=0;
      runner.state=RS_RUN;
      runner.stateT=0;
    }
  }
  else if(runner.state==RS_CLIMB){
    if(runner.climbPhase==0){
      // Short approach only; never allow the runner to drift off-screen.
      runner.x += 0.18f*RUNNER_MOTION_SCALE;
      runner.x=clampf(runner.x,55.0f,90.0f);
      if(runner.x-runner.climbStartX>=4.0f){
        runner.climbPhase=1;
        runner.stateT=0;
      }
    } else {
      // True screen-space climb: X is held fixed while Y rises.
      runner.x=clampf(runner.x,55.0f,90.0f);
      float dy=runner.climbTargetY-runner.y;
      if(fabsf(dy)<=0.7f || runner.stateT>60){
        runner.y=runner.climbTargetY;
        runner.state=RS_RUN;
        runner.stateT=0;
      } else {
        runner.y += copysignf(fminf(0.55f*RUNNER_MOTION_SCALE,fabsf(dy)),dy);
      }
    }
  }
  else if(runner.state==RS_WAVE){
    // WAVE means the runner has stopped in world space.
    // The terrain continues scrolling left, so the stationary runner must
    // drift left by exactly the same one-pixel world shift.
    runner.x -= 1.0f;
    runner.y=groundAt((int)runner.x+5)-11;
    if(runner.stateT>42){
      runner.state=RS_RUN;
      runner.stateT=0;
    }
  }
}

// ---------------- UFO mechanics ----------------
void startUfoShift(){
  if(ufo.active) return;
  ufo.active=true;
  ufo.lostRunnerRescue=false;
  ufo.rescueTiltHold=false;
  ufo.phase=UFO_ENTER_LEFT;
  ufo.x=-30;
  ufo.y=23;
  ufo.targetX=runner.x+5;
  ufo.phaseT=0;
  ufo.beam=0;
  ufo.beamProgress=0;
  ufo.beamGroundY=groundAt((int)runner.x+5);
  ufo.lastStepMs=millis();
  // Runner deliberately keeps running while the UFO approaches.
}

void checkScheduledUfo(){
  struct tm t;
  localNow(t);
  if(!(t.tm_hour==1 || t.tm_hour==9 || t.tm_hour==17)) return;
  // Trigger during the whole boundary minute. The day/hour guard below makes
  // it fire only once, while avoiding missed shifts after a short network stall.
  if(t.tm_min!=0) return;
  if(ufo.lastScheduleDay==t.tm_yday && ufo.lastScheduleHour==t.tm_hour) return;
  ufo.lastScheduleDay=t.tm_yday;
  ufo.lastScheduleHour=t.tm_hour;
  startUfoShift();
}

void updateUfo(){
  checkScheduledUfo();
  if(!ufo.active) return;

  uint32_t now=millis();
  if(now-ufo.lastStepMs<16) return; // HTML choreography is approximately 60 fps
  ufo.lastStepMs=now;
  ufo.phaseT++;

  if(ufo.phase==UFO_ENTER_LEFT){
    ufo.targetX=runner.x+5;
    ufo.x+=1.6f;
    if(ufo.x>=ufo.targetX){
      ufo.x=ufo.targetX;
      ufo.phase=UFO_HOVER;
      ufo.phaseT=0;
    }
  }else if(ufo.phase==UFO_HOVER){
    ufo.x=runner.x+5;
    if(ufo.phaseT>10){
      ufo.phase=UFO_BEAM_GROW;
      ufo.phaseT=0;
      ufo.beam=0.15f;
      ufo.beamProgress=0;
      ufo.beamGroundY=groundAt((int)runner.x+5);
    }
  }else if(ufo.phase==UFO_BEAM_GROW){
    ufo.x=runner.x+5;
    ufo.beam=fminf(1.0f,ufo.beam+0.05f);
    ufo.beamProgress=fminf(1.0f,ufo.beamProgress+0.022f);
    ufo.beamGroundY=groundAt((int)runner.x+5);
    if(ufo.beamProgress>=1.0f){
      ufo.phase=UFO_BEAM_CONTACT;
      ufo.phaseT=0;
      runner.y=ufo.beamGroundY-11;
    }
  }else if(ufo.phase==UFO_BEAM_CONTACT){
    ufo.beam=1;
    ufo.beamProgress=1;
    ufo.beamGroundY=groundAt((int)runner.x+5);
    runner.x=ufo.x-5;
    runner.y=ufo.beamGroundY-11;
    if(ufo.phaseT>12){
      ufo.phase=UFO_ABDUCT;
      ufo.phaseT=0;
    }
  }else if(ufo.phase==UFO_ABDUCT){
    ufo.beam=1;
    ufo.beamProgress=1;
    runner.x=ufo.x-5;
    runner.y-=0.9f;
    if(runner.y<ufo.y+7){
      ufo.phase=UFO_EXIT_RIGHT;
      ufo.phaseT=0;
      ufo.beam=0;
      ufo.beamProgress=0;
    }
  }else if(ufo.phase==UFO_EXIT_RIGHT){
    ufo.x+=2.0f;
    runner.x=ufo.x-5;
    runner.y=ufo.y+5;
    if(ufo.x>W+30){
      ufo.phase=UFO_RETURN_RIGHT;
      ufo.x=W+30;
      ufo.phaseT=0;
      runner.x=W+35;
      runner.y=-20;
    }
  }else if(ufo.phase==UFO_RETURN_RIGHT){
    ufo.x-=1.8f;
    ufo.targetX=RUNNER_TARGET_X+5;
    if(ufo.x<=ufo.targetX){
      ufo.x=ufo.targetX;

      // Automatic lost-runner rescue always arrives after 5 s, but if the
      // Cardputer is still held at an extreme left/right angle, do not drop
      // the runner straight back into the same off-screen slide.
      if(ufo.lostRunnerRescue && imuReady){
        if(!ufo.rescueTiltHold && fabsf(imuTiltDeg)>=UFO_RESCUE_HOLD_ENTER_DEG){
          ufo.rescueTiltHold=true;
        }
        if(ufo.rescueTiltHold && fabsf(imuTiltDeg)>UFO_RESCUE_HOLD_RELEASE_DEG){
          runner.x=ufo.x-5;
          runner.y=ufo.y+5;
          return;
        }
        ufo.rescueTiltHold=false;
      }

      ufo.phase=UFO_DROP_BEAM;
      ufo.phaseT=0;
      ufo.beam=0.15f;
      ufo.beamProgress=0;
    }
  }else if(ufo.phase==UFO_RETURN_LEFT){
    ufo.x+=1.8f;
    ufo.targetX=RUNNER_TARGET_X+5;
    if(ufo.x>=ufo.targetX){
      ufo.x=ufo.targetX;

      if(ufo.lostRunnerRescue && imuReady){
        if(!ufo.rescueTiltHold && fabsf(imuTiltDeg)>=UFO_RESCUE_HOLD_ENTER_DEG){
          ufo.rescueTiltHold=true;
        }
        if(ufo.rescueTiltHold && fabsf(imuTiltDeg)>UFO_RESCUE_HOLD_RELEASE_DEG){
          runner.x=ufo.x-5;
          runner.y=ufo.y+5;
          return;
        }
        ufo.rescueTiltHold=false;
      }

      ufo.phase=UFO_DROP_BEAM;
      ufo.phaseT=0;
      ufo.beam=0.15f;
      ufo.beamProgress=0;
    }
  }else if(ufo.phase==UFO_DROP_BEAM){
    ufo.beam=fminf(1.0f,ufo.beam+0.05f);
    ufo.beamProgress=fminf(1.0f,ufo.beamProgress+0.022f);
    ufo.beamGroundY=groundAt((int)roundf(ufo.x));
    if(ufo.beamProgress>=1.0f){
      ufo.phase=UFO_DROP;
      ufo.phaseT=0;
      runner.x=ufo.x-5;
      runner.y=ufo.y+7;
    }
  }else if(ufo.phase==UFO_DROP){
    ufo.beam=1;
    ufo.beamProgress=1;
    runner.x=ufo.x-5;
    float landing=groundAt((int)runner.x+5)-11;
    runner.y=fminf(landing,runner.y+0.9f);
    ufo.beamGroundY=(int)roundf(landing+11);
    if(runner.y>=landing){
      runner.y=landing;
      ufo.phase=UFO_LEAVE_LEFT;
      ufo.phaseT=0;
      ufo.beam=0;
      ufo.beamProgress=0;
      runner.state=RS_RUN;
    }
  }else if(ufo.phase==UFO_LEAVE_LEFT){
    ufo.x-=2.0f;
    runner.x=fminf((float)RUNNER_TARGET_X,runner.x+0.30f);
    runner.y=groundAt((int)runner.x+5)-11;
    if(ufo.x<-30){
      ufo.active=false;
      ufo.phase=UFO_IDLE;
      ufo.phaseT=0;
      ufo.lostRunnerRescue=false;
      ufo.rescueTiltHold=false;
      runner.x=RUNNER_TARGET_X;
      runner.state=RS_RUN;
    }
  }
}

void drawUfo(){
  if(!ufo.active) return;
  int x=(int)roundf(ufo.x), y=(int)roundf(ufo.y);

  // Green tractor beam grows downward from the saucer to the actual terrain.
  if(ufo.beam>0 && ufo.beamProgress>0){
    int sy=y+4;
    int fullGround=max(sy+1,ufo.beamGroundY);
    int ey=sy+(int)roundf((fullGround-sy)*ufo.beamProgress);
    int half=4+(int)roundf(9*ufo.beamProgress);
    uint16_t beam=0xBFE7;   // pale green: keep the runner visible through the beam
    uint16_t edge=0x7FEA;   // brighter green edge
    canvas.fillTriangle(x-5,sy,x+5,sy,x+half,ey,beam);
    canvas.fillTriangle(x-5,sy,x+half,ey,x-half,ey,beam);
    canvas.drawLine(x-5,sy,x-half,ey,edge);
    canvas.drawLine(x+5,sy,x+half,ey,edge);
    if(ufo.beamProgress>=0.999f) canvas.drawLine(x-half,ey,x+half,ey,0xEFEA);
  }

  // Compact 70s/80s arcade saucer.
  canvas.fillEllipse(x,y,13,4,0xD69A);
  canvas.drawEllipse(x,y,13,4,TFT_BLACK);
  canvas.fillEllipse(x,y-3,6,4,0x7EDE);
  canvas.drawEllipse(x,y-3,6,4,TFT_BLACK);
  canvas.fillRect(x-10,y-1,20,3,0xEF7D);
  canvas.drawRect(x-10,y-1,20,3,TFT_BLACK);
  uint16_t lamp=((millis()/180)&1)?0xFFE0:0xFA28;
  canvas.fillRect(x-8,y+3,2,2,lamp);
  canvas.fillRect(x-1,y+3,2,2,lamp);
  canvas.fillRect(x+6,y+3,2,2,lamp);
}

// ---------------- Drawing ----------------
uint16_t mix565(uint16_t a,uint16_t b,float t){
  t=clampf(t,0.0f,1.0f);
  int ar=(a>>11)&31, ag=(a>>5)&63, ab=a&31;
  int br=(b>>11)&31, bg=(b>>5)&63, bb=b&31;
  int r=(int)roundf(ar+(br-ar)*t);
  int g=(int)roundf(ag+(bg-ag)*t);
  int bl=(int)roundf(ab+(bb-ab)*t);
  return (r<<11)|(g<<5)|bl;
}

static float intervalPhaseFloat(float nowMin,float rise,float set){
  if(rise<set){
    if(nowMin<rise || nowMin>=set) return -1.0f;
    return (nowMin-rise)/fmaxf(1.0f,set-rise);
  }
  if(rise>set){
    if(!(nowMin>=rise || nowMin<set)) return -1.0f;
    float len=(1440.0f-rise)+set;
    float since=(nowMin>=rise)?(nowMin-rise):((1440.0f-rise)+nowMin);
    return since/fmaxf(1.0f,len);
  }
  return -1.0f;
}

static float solarElevationProxy(float m){
  const float rise=(float)sunriseMinutes;
  const float set =(float)sunsetMinutes;

  float dayPhase=intervalPhaseFloat(m,rise,set);
  if(dayPhase>=0.0f) return sinf(PI*dayPhase);

  float nightPhase=intervalPhaseFloat(m,set,rise);
  if(nightPhase>=0.0f) return -sinf(PI*nightPhase);
  return -1.0f;
}

static float daylightStrength(float m){
  float e=solarElevationProxy(m);
  if(e>=0.0f) return 0.68f+0.32f*powf(e,0.55f);

  // No fixed 60-minute twilight: below-horizon light comes from the modeled
  // solar elevation, so its duration scales with the selected place/date.
  float nearHorizon=1.0f-fminf(1.0f,(-e)/0.34f);
  return fmaxf(0.0f,0.68f*nearHorizon);
}

static float moonVisibilityFromSun(float m){
  float d=daylightStrength(m);
  return clampf(1.0f-0.78f*d,0.22f,1.0f);
}

uint16_t clearSkyForHour(float h){
  // Twilight is driven by the modeled solar elevation from the actual
  // sunrise/sunset schedule. There is no fixed +/-60 minute window.
  const uint16_t NIGHT=0x0000;
  const uint16_t HORIZON=0xFFDC; // warm pale horizon
  const uint16_t DAY=0xBFFF;     // clean pale sky blue

  float m=h*60.0f;
  float e=solarElevationProxy(m);
  float d=daylightStrength(m);

  if(e>=0.0f){
    float high=powf(clampf(e,0.0f,1.0f),0.7f);
    return mix565(HORIZON,DAY,high);
  }
  return mix565(NIGHT,HORIZON,clampf(d/0.68f,0.0f,1.0f));
}

uint16_t skyColorForTime(const tm &t){
  float h=t.tm_hour+t.tm_min/60.0f+t.tm_sec/3600.0f;
  uint16_t sky=clearSkyForHour(h);
  if((weather.mode==WX_RAIN || weather.mode==WX_THUNDER) && isSolarDayMinute(minuteOfDay(t))){
    sky=mix565(sky,0x8410,0.28f);
  }
  return sky;
}

// Pick clock/date/weather ink from the ACTUAL interpolated sky brightness.
// This makes the text transition naturally during 04-06 dawn and 16-18 dusk,
// instead of switching at a fixed clock time.
static uint16_t readableInkForSky(uint16_t c){
  int r5=(c>>11)&0x1F;
  int g6=(c>>5)&0x3F;
  int b5=c&0x1F;
  int r=(r5*255+15)/31;
  int g=(g6*255+31)/63;
  int b=(b5*255+15)/31;
  // Perceptual luminance approximation (Rec.601 integer weights).
  int lum=(299*r + 587*g + 114*b)/1000;
  return (lum>=128)?TFT_BLACK:TFT_WHITE;
}

void localNow(struct tm &tmNow);

static uint16_t cloudColorForSky(){
  tm t; localNow(t);
  float m=(t.tm_hour+t.tm_min/60.0f+t.tm_sec/3600.0f)*60.0f;

  // Keep cloud brightness synchronized with the same continuous solar-elevation
  // model used by clearSkyForHour(). No separate fixed +/-60 minute twilight.
  float dayMix=clampf(daylightStrength(m),0.0f,1.0f);

  uint16_t dayCloud=0xFFFF;
  if((weather.mode==WX_RAIN || weather.mode==WX_THUNDER) && isSolarDayMinute((int)m))
    dayCloud=0x9CF3; // daytime rain: neutral grey
  return mix565(TFT_BLACK,dayCloud,dayMix);
}
void drawCloud(int x,int y){
  uint16_t body=cloudColorForSky();
  uint8_t r=((body>>11)&0x1F)*255/31, g=((body>>5)&0x3F)*255/63, b=(body&0x1F)*255/31;
  uint16_t edge=canvas.color565(clampi(r+18,0,255),clampi(g+18,0,255),clampi(b+18,0,255));
  canvas.fillCircle(x+1,y+1,4,edge); canvas.fillCircle(x+6,y-1,5,edge);
  canvas.fillCircle(x+11,y+1,4,edge); canvas.fillRect(x+1,y+1,11,5,edge);
  canvas.fillCircle(x,y,4,body); canvas.fillCircle(x+5,y-2,5,body);
  canvas.fillCircle(x+10,y,4,body); canvas.fillRect(x,y,11,5,body);
}

void drawClassicSun(int cx,int cy){
  // Refined at the actual Cardputer runtime size (18px diameter).
  uint16_t ink=0x4204;
  uint16_t face=0xF6AD;

  for(int i=0;i<24;i++){
    float a=i*(2.0f*M_PI/24.0f);
    int r1=9, r2=(i%2==0)?16:13;
    canvas.drawLine(cx+(int)roundf(cosf(a)*r1), cy+(int)roundf(sinf(a)*r1),
                    cx+(int)roundf(cosf(a)*r2), cy+(int)roundf(sinf(a)*r2), ink);
  }

  canvas.fillCircle(cx,cy,9,face);
  canvas.drawCircle(cx,cy,9,ink);

  // Brows and eyes.
  canvas.drawLine(cx-6,cy-4,cx-4,cy-5,ink); canvas.drawLine(cx-4,cy-5,cx-2,cy-4,ink);
  canvas.drawLine(cx+2,cy-4,cx+4,cy-5,ink); canvas.drawLine(cx+4,cy-5,cx+6,cy-4,ink);
  canvas.drawLine(cx-6,cy-2,cx-4,cy-3,ink); canvas.drawLine(cx-4,cy-3,cx-2,cy-2,ink); canvas.drawPixel(cx-4,cy-2,ink);
  canvas.drawLine(cx+2,cy-2,cx+4,cy-3,ink); canvas.drawLine(cx+4,cy-3,cx+6,cy-2,ink); canvas.drawPixel(cx+4,cy-2,ink);

  // Nose.
  canvas.drawLine(cx,cy-1,cx-1,cy+2,ink);
  canvas.drawLine(cx-1,cy+2,cx,cy+3,ink);
  canvas.drawLine(cx,cy+3,cx+2,cy+2,ink);

  // Philtrum.
  canvas.drawPixel(cx,cy+4,ink);

  // Small upper lip and separate lower lip.
  canvas.drawLine(cx-2,cy+5,cx,cy+4,ink);
  canvas.drawLine(cx,cy+4,cx+2,cy+5,ink);
  canvas.drawLine(cx-2,cy+6,cx,cy+7,ink);
  canvas.drawLine(cx,cy+7,cx+2,cy+6,ink);

  // Chin.
  canvas.drawLine(cx-1,cy+8,cx+1,cy+8,ink);
}

// Astronomical lunar phase from UTC epoch.
// Returns 0.0 at new moon, 0.25 first quarter, 0.5 full moon,
// 0.75 last quarter. Reference new moon: 2000-01-06 18:14 UTC.
float lunarPhase01(){
  const double SYNODIC_DAYS = 29.530588853;
  const double REF_NEW_MOON_JD = 2451550.25972;
  const double UNIX_JD = 2440587.5;

  double jd = UNIX_JD + (double)safeEpoch() / 86400.0;
  double p = fmod((jd - REF_NEW_MOON_JD) / SYNODIC_DAYS, 1.0);
  if(p < 0.0) p += 1.0;
  return (float)p;
}

void drawClassicMoon(int cx,int cy,uint16_t sky,float visibility=1.0f){
  // Same face proportions as the Sun. On bright daytime sky the entire Moon
  // is blended toward the sky color, reproducing the low-contrast daytime Moon.
  visibility=clampf(visibility,0.0f,1.0f);
  const uint16_t baseFace=0xF689;
  const uint16_t baseInk =0x4204;
  const uint16_t baseDark=0x3186;
  const uint16_t face=mix565(sky,baseFace,visibility);
  const uint16_t ink =mix565(sky,baseInk,visibility);
  const uint16_t darkMoon=mix565(sky,baseDark,visibility);
  const int R=10;
  float phase=lunarPhase01();

  canvas.fillCircle(cx,cy,R,darkMoon);
  for(int yy=-R; yy<=R; ++yy){
    for(int xx=-R; xx<=R; ++xx){
      if(xx*xx+yy*yy>R*R) continue;
      float z=sqrtf(fmaxf(0.0f,(float)(R*R-xx*xx-yy*yy)));
      float sx=-sinf(2.0f*PI*phase), sz=-cosf(2.0f*PI*phase);
      if((float)xx*sx+z*sz>0.0f) canvas.drawPixel(cx+xx,cy+yy,face);
    }
  }
  canvas.drawCircle(cx,cy,R,ink);

  canvas.drawLine(cx-6,cy-4,cx-4,cy-5,ink); canvas.drawLine(cx-4,cy-5,cx-2,cy-4,ink);
  canvas.drawLine(cx+2,cy-4,cx+4,cy-5,ink); canvas.drawLine(cx+4,cy-5,cx+6,cy-4,ink);
  canvas.drawLine(cx-6,cy-2,cx-4,cy-3,ink); canvas.drawLine(cx-4,cy-3,cx-2,cy-2,ink); canvas.drawPixel(cx-4,cy-2,ink);
  canvas.drawLine(cx+2,cy-2,cx+4,cy-3,ink); canvas.drawLine(cx+4,cy-3,cx+6,cy-2,ink); canvas.drawPixel(cx+4,cy-2,ink);

  canvas.drawLine(cx,cy-1,cx-1,cy+2,ink);
  canvas.drawLine(cx-1,cy+2,cx,cy+3,ink);
  canvas.drawLine(cx,cy+3,cx+2,cy+2,ink);
  canvas.drawPixel(cx,cy+4,ink);

  canvas.drawLine(cx-2,cy+5,cx,cy+4,ink);
  canvas.drawLine(cx,cy+4,cx+2,cy+5,ink);
  canvas.drawLine(cx-2,cy+6,cx,cy+7,ink);
  canvas.drawLine(cx,cy+7,cx+2,cy+6,ink);
  canvas.drawLine(cx-1,cy+8,cx+1,cy+8,ink);
}

void moonXYForPhase(float phase,int &cx,int &cy){
  // Moonrise means the lunar disk is already visible at the left edge;
  // moonset means it reaches the right edge. Preserve the low arc.
  phase=clampf(phase,0.0f,1.0f);
  const float R=10.0f;
  cx=(int)roundf(R + ((float)W-2.0f*R)*phase);
  const float horizonY=62.0f;
  const float ry=42.0f;
  cy=(int)roundf(horizonY-ry*sinf(PI*phase));
}

void celestialXYForPhase(float phase, int &cx, int &cy){
  // Sunrise: the Sun disk is already visible at the left edge.
  // Sunset: the disk reaches the right edge. Keep the existing low arc.
  phase=clampf(phase,0.0f,1.0f);
  const float R=9.0f;
  cx=(int)roundf(R + ((float)W-2.0f*R)*phase);
  const float horizonY=62.0f;
  const float ry=42.0f;
  cy=(int)roundf(horizonY-ry*sinf(PI*phase));
}

bool sunVisibleAndPhase(const tm &t,float &phase){
  float m=t.tm_hour*60.0f+t.tm_min+t.tm_sec/60.0f;
  const float rise=(float)sunriseMinutes;
  const float set =(float)sunsetMinutes;
  if(rise<set && m>=rise && m<set){
    phase=(m-rise)/fmaxf(1.0f,set-rise);
    return true;
  }
  // Defensive support for unusual solar schedules that cross midnight.
  if(rise>set && (m>=rise || m<set)){
    float len=(1440.0f-rise)+set;
    float since=(m>=rise)?(m-rise):((1440.0f-rise)+m);
    phase=since/fmaxf(1.0f,len);
    return true;
  }
  return false;
}

bool moonVisibleAndPhase(const tm &t,float &phase){
  float m=t.tm_hour*60.0f+t.tm_min+t.tm_sec/60.0f;
  const float rise=(float)moonriseMinutes;
  const float set =(float)moonsetMinutes;
  if(rise<set){
    if(m<rise || m>=set) return false;
    phase=(m-rise)/fmaxf(1.0f,set-rise);
    return true;
  }
  if(rise>set){
    if(!(m>=rise || m<set)) return false;
    float len=(1440.0f-rise)+set;
    float since=(m>=rise)?(m-rise):((1440.0f-rise)+m);
    phase=since/fmaxf(1.0f,len);
    return true;
  }
  return false;
}

const char* weatherLabel(){
  switch(weather.mode){
    case WX_CLEAR:   return "SUNNY";
    case WX_CLOUDY:  return "CLOUDY";
    case WX_RAIN:    return "RAIN";
    case WX_SNOW:    return "SNOW";
    case WX_THUNDER: return "THUNDER";
    default:         return "--";
  }
}

void drawWeatherAndClock(){
  struct tm t;
  localNow(t);
  bool day=isSolarDayMinute(minuteOfDay(t));
  uint16_t sky=skyColorForTime(t);
  canvas.fillScreen(sky);

  // v98: Sun and Moon have independent real rise/set schedules. A daytime Moon
  // is therefore possible. CLOUDY does not delete either body; later cloud
  // drawing can naturally pass in front of them. Preserve the existing
  // rain/thunder policy for now.
  bool hide=(weather.mode==WX_RAIN || weather.mode==WX_THUNDER);
  if(!hide){
    float p; int cx,cy;
    if(solarScheduleValid && sunVisibleAndPhase(t,p)){
      celestialXYForPhase(p,cx,cy);
      drawClassicSun(cx,cy);
    }
    if(lunarScheduleValid && moonVisibleAndPhase(t,p)){
      moonXYForPhase(p,cx,cy);
      float m=t.tm_hour*60.0f+t.tm_min+t.tm_sec/60.0f;
      drawClassicMoon(cx,cy,sky,moonVisibilityFromSun(m));
    }
  }

  // Night sky: stars instead of decorative clouds.
  // Rain/thunder and heavy overcast hide the stars.
  bool night=!day;
  // v47: stars always exist in the night sky.
  // Weather no longer deletes/reduces stars; foreground clouds physically hide them.
  if(night){
    static const uint8_t sx[] = {8,21,37,52,69,84,101,119,137,154,173,191,207,224,234,29,77,146,216};
    static const uint8_t sy[] = {11,28,18,44,9,33,22,48,13,37,25,7,42,20,52,58,55,61,57};

    // Retro-game star palette: white remains dominant, with sparse blue/red/yellow accents.
    static const uint16_t sc[] = {
      TFT_WHITE, TFT_WHITE, 0x5DFF, TFT_WHITE, TFT_YELLOW,
      TFT_WHITE, TFT_WHITE, TFT_RED, TFT_WHITE, TFT_WHITE,
      0x5DFF, TFT_WHITE, TFT_YELLOW, TFT_WHITE, TFT_WHITE,
      TFT_RED, TFT_WHITE, 0x5DFF, TFT_WHITE
    };

    // Only a few stars twinkle. Fixed phase offsets keep them from blinking in unison.
    static const bool twinkle[] = {
      false,true,false,false,false,
      true,false,false,true,false,
      false,false,true,false,false,
      false,false,true,false
    };

    const int N=sizeof(sx)/sizeof(sx[0]);
    const uint32_t tick=millis()/420UL;
    for(int i=0;i<N;i++){
      if(twinkle[i]){
        uint8_t phase=(uint8_t)((tick+i*3)%7);
        if(phase==0 || phase==1) continue;  // brief retro-style off beat
      }
      canvas.drawPixel(sx[i],sy[i],sc[i]);
    }
  }

  // Weather sets the basic density; Open-Meteo cloud_cover fine-tunes it.
  int baseClouds =
      weather.mode==WX_CLEAR ? 0 :
      weather.mode==WX_CLOUDY ? 3 :
      weather.mode==WX_RAIN ? 5 :
      weather.mode==WX_SNOW ? 4 :
      weather.mode==WX_THUNDER ? 6 : 1;
  int cloudAdj = weather.cloud>=85 ? 2 : weather.cloud>=65 ? 1 : weather.cloud<=20 ? -1 : 0;
  int cloudCount=clampi(baseClouds+cloudAdj,0,8);
  if(weather.mode==WX_CLEAR && weather.cloud>=25) cloudCount=1;
  for(int i=0;i<cloudCount;i++){
    uint32_t period=W+80;
    int x=(int)((W+30+i*72)-((millis()/220+i*13)%period));
    int y=63+(i%2)*10;
    drawCloud(x,y);
  }

  if(weather.mode==WX_RAIN || weather.mode==WX_THUNDER){
    uint16_t rain=0x5C12;
    // v48: precipitation falls from the very top of the LCD down to the terrain zone.
    const int precipBottom=TERRAIN_LOW;
    for(int i=0;i<28;i++){
      int x=(i*8 + millis()/18)%W;
      int y=(i*29 + millis()/11)%(precipBottom+10)-8;
      canvas.drawLine(x,y,x+2,y+5,rain);
    }
  } else if(weather.mode==WX_SNOW){
    const int precipBottom=TERRAIN_LOW;
    for(int i=0;i<24;i++){
      int x=(i*19 + millis()/80)%W;
      int y=(i*31 + millis()/40)%(precipBottom+10)-8;
      canvas.drawPixel(x,y,TFT_WHITE);
    }
  }

}


static uint16_t lerpRgb565(uint8_t r0,uint8_t g0,uint8_t b0,
                           uint8_t r1,uint8_t g1,uint8_t b1,float t){
  t=clampf(t,0.0f,1.0f);
  uint8_t r=(uint8_t)lroundf(r0+(r1-r0)*t);
  uint8_t g=(uint8_t)lroundf(g0+(g1-g0)*t);
  uint8_t b=(uint8_t)lroundf(b0+(b1-b0)*t);
  return canvas.color565(r,g,b);
}

static void clockRetroPalette(const struct tm &t,
                              uint16_t &dateColor,
                              uint16_t &timeColor,
                              uint16_t &weatherColor){
  float d=daylightStrength(minuteOfDay(t)); // 0=night, 1=bright day

  // Original 1980s-game palette plan, continuously adapted for sky brightness.
  // d=0 (night): DATE cyan / TIME bright yellow / WEATHER bright magenta
  // d=1 (day):   DATE deep blue / TIME black / WEATHER deep magenta
  dateColor   = lerpRgb565(102,255,255,   0, 64,160,d);
  timeColor   = lerpRgb565(255,255, 64,   0,  0,  0,d);
  weatherColor= lerpRgb565(255, 79,216, 144,  0, 88,d);
}

void drawClockInfoOverlay(){
  // T and I are deliberately independent:
  // T = normal clock/weather text (visible by default)
  // I = maintenance/auxiliary information (hidden by default)
  if(!showClockInfo && !showAuxInfo) return;

  struct tm t;
  localNow(t);
  uint16_t sky=skyColorForTime(t);
  uint16_t ink=readableInkForSky(sky);
  const int clockX=W/2;
  canvas.setTextColor(ink);

  if(showAuxInfo){
    char sunb[28],moonb[28];
    auto fmtHM=[](int mins, char *dst, size_t n){
      mins=((mins%1440)+1440)%1440;
      snprintf(dst,n,"%02d:%02d",mins/60,mins%60);
    };
    char sr[8],ss[8],mr[8],ms[8];
    if(solarScheduleValid){
      fmtHM(sunriseMinutes,sr,sizeof(sr));
      fmtHM(sunsetMinutes,ss,sizeof(ss));
      snprintf(sunb,sizeof(sunb),"SUN  R%s S%s",sr,ss);
    }else{
      snprintf(sunb,sizeof(sunb),"SUN  --:-- --:--");
    }
    if(lunarScheduleValid){
      fmtHM(moonriseMinutes,mr,sizeof(mr));
      fmtHM(moonsetMinutes,ms,sizeof(ms));
      snprintf(moonb,sizeof(moonb),"MOON R%s S%s",mr,ms);
    }else{
      snprintf(moonb,sizeof(moonb),"MOON --:-- --:--");
    }

    canvas.setFont(&fonts::Font0);
    canvas.setTextSize(1);
    canvas.setTextDatum(top_left);
    canvas.drawString(sunb,2,2);
    canvas.drawString(moonb,2,12);

    // Avoid a temporary Arduino String allocation on every rendered frame.
    char locLabel[33];
    snprintf(locLabel,sizeof(locLabel),"%.32s",locationName.c_str());
    canvas.setTextDatum(bottom_left);
    if(tide.valid){
      auto fmtTideTime=[](time_t epoch, char *dst, size_t n){
        time_t localEpoch=epoch + localUtcOffsetSeconds;
        struct tm tmv;
        gmtime_r(&localEpoch,&tmv);
        snprintf(dst,n,"%02d:%02d",tmv.tm_hour,tmv.tm_min);
      };
      char hi[8],lo[8],highb[20],lowb[20],trendb[12];
      fmtTideTime(tide.nextHighEpoch,hi,sizeof(hi));
      fmtTideTime(tide.nextLowEpoch,lo,sizeof(lo));
      snprintf(highb,sizeof(highb),"HIGH %s",hi);
      snprintf(lowb,sizeof(lowb),"LOW  %s",lo);
      snprintf(trendb,sizeof(trendb),"TIDE %s",tide.rising?"UP":"DN");
      canvas.drawString(highb,3,H-22);
      canvas.drawString(lowb,3,H-12);
      canvas.drawString(trendb,72,H-12);
    }else{
      canvas.drawString("HIGH --:--",3,H-22);
      canvas.drawString("LOW  --:--",3,H-12);
      canvas.drawString("TIDE --",72,H-12);
    }
    canvas.drawString(locLabel,3,H-2);

    // Detailed weather information belongs to I (auxiliary information).
    char auxPress[20],auxPop[16];
    if(isfinite(weather.pressureMslHpa)) snprintf(auxPress,sizeof(auxPress),"PRES %.0fhPa",weather.pressureMslHpa);
    else snprintf(auxPress,sizeof(auxPress),"PRES ----hPa");
    if(weather.precipitationProbabilityPct>=0) snprintf(auxPop,sizeof(auxPop),"RAIN %d%%",weather.precipitationProbabilityPct);
    else snprintf(auxPop,sizeof(auxPop),"RAIN --%%");
    canvas.setTextDatum(top_left);
    canvas.drawString(auxPress,174,22);
    canvas.drawString(auxPop,174,32);
  }

  if(showClockInfo){
    char tb[16],db[24],tempb[20],humb[16],pressb[20],popb[16];
    static const char* WDAYS[7]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
    strftime(tb,sizeof(tb),"%H:%M",&t);
    snprintf(db,sizeof(db),"%04d.%02d.%02d %s",
             t.tm_year+1900,t.tm_mon+1,t.tm_mday,
             (t.tm_wday>=0 && t.tm_wday<7)?WDAYS[t.tm_wday]:"---");
    if(isfinite(weather.temperatureC)) snprintf(tempb,sizeof(tempb),"TEMP %.1fC",weather.temperatureC);
    else snprintf(tempb,sizeof(tempb),"TEMP --.-C");
    if(weather.humidityPct>=0) snprintf(humb,sizeof(humb),"HUM  %d%%",weather.humidityPct);
    else snprintf(humb,sizeof(humb),"HUM  --%%");
    if(isfinite(weather.pressureMslHpa)) snprintf(pressb,sizeof(pressb),"PRES %.0fhPa",weather.pressureMslHpa);
    else snprintf(pressb,sizeof(pressb),"PRES ----hPa");
    if(weather.precipitationProbabilityPct>=0) snprintf(popb,sizeof(popb),"RAIN %d%%",weather.precipitationProbabilityPct);
    else snprintf(popb,sizeof(popb),"RAIN --%%");

    // v108: right = current atmosphere.
    canvas.setFont(&fonts::Font0);
    canvas.setTextSize(1);
    canvas.setTextDatum(top_left);
    canvas.drawString(tempb,174,2);
    canvas.drawString(humb,174,12);

    // Central information block.
    // M5GFX ships FreeSansBold at 9/12/18/24pt, so use its float text scaling
    // to obtain the requested effective sizes without changing font family:
    // DATE/weekday 9pt*(7/9)=7pt, TIME 18pt*(8/9)=16pt,
    // WEATHER 9pt*(7/9)=7pt.
    uint16_t dateColor,timeColor,weatherColor;
    clockRetroPalette(t,dateColor,timeColor,weatherColor);

    canvas.setTextDatum(top_center);
    canvas.setFont(&fonts::FreeSansBold9pt7b);
    canvas.setTextSize(7.0f/9.0f);
    canvas.setTextColor(dateColor);
    canvas.drawString(db,clockX,39);

    canvas.setFont(&fonts::FreeSansBold18pt7b);
    canvas.setTextSize(8.0f/9.0f);
    canvas.setTextColor(timeColor);
    canvas.drawString(tb,clockX,53);

    canvas.setFont(&fonts::FreeSansBold9pt7b);
    canvas.setTextSize(7.0f/9.0f);
    canvas.setTextColor(weatherColor);
    const char* wx=weatherLabel();
    canvas.drawString(wx,clockX,80);

    canvas.setTextColor(ink);

    canvas.setTextSize(1);
  }

  canvas.setTextDatum(top_left);
}

void drawTerrain(){
  // v92: terrain is the exact inverse of the current sky-text ink.
  // Bright sky -> black text + white terrain.
  // Dark sky   -> white text + black terrain.
  struct tm st;
  localNow(st);
  uint16_t ink=readableInkForSky(skyColorForTime(st));
  // Day: Choplifter-style sand #E9B86A. Night: saturated primary blue #0000FF.
  // Only presentation changes; the sound-generated terrain profile is unchanged.
  bool day=isSolarDayMinute(minuteOfDay(st));
  uint16_t ground=day ? canvas.color565(233,184,106) : canvas.color565(0,0,255);

  for(int x=0;x<W;x++){
    int top=displayGroundAt(x);
    if(top<=TERRAIN_BOTTOM){
      canvas.drawFastVLine(x,top,TERRAIN_BOTTOM-top+1,ground);
    }
  }

  // Road edge uses the same ink color as the clock/HUD, i.e. the opposite
  // of the ground, so the 1 px surface remains visible in both phases.
  for(int x=0;x<W-1;x++){
    canvas.drawLine(x,displayGroundAt(x),x+1,displayGroundAt(x+1),ink);
  }
}

void drawRunner(){
  if(runnerLost) return;
  const char* const* sp=RUN0;
  if(ufoOwnsRunner()){
    // Raised-arms/capture pose while the beam owns the runner.
    sp=FALL0;
  } else if(runner.state==RS_RUN){
    int f=(runner.anim/4)%3;
    sp=(f==0)?RUN0:(f==1)?RUN1:RUN2;
  } else if(runner.state==RS_JUMP) sp=JUMP0;
  else if(runner.state==RS_FALL) sp=FALL0;
  else if(runner.state==RS_CLIMB) sp=((runner.anim/6)%2)?CLIMB0:CLIMB1;
  else if(runner.state==RS_WAVE){
    int f=(runner.anim/7)%3;
    sp=(f==0)?WAVE0:(f==1)?WAVE1:WAVE2;
  }
  int drawY=(int)roundf(runner.y);
  if(!ufoOwnsRunner()){
    int gx=clampi((int)roundf(runner.x)+5,0,W-1);
    drawY += displayGroundAt(gx)-rawGroundAt(gx);
  }
  drawSprite(sp,(int)roundf(runner.x),drawY);
}



static void drawAudioDiagnostic(){
  // 8-band Audio City is now the visible right-side terrain generator.
  // LOW -> HIGH runs left to right. Audio calculation is unchanged from v80b/v81.
  //
  // v89 layout:
  // - keep the approved colorful 8-band Audio City exactly the same size
  // - move it into the dark ground at bottom-right
  // - align its right edge with the AP status area
  // - keep it immediately above the AP text, away from the sun/moon orbit
  const int barW = EQ_GEN_BAR_W;
  const int gap = EQ_GEN_GAP;
  const int cityLeft = EQ_GEN_LEFT;
  const int horizonY = H - 13;            // leaves the bottom AP row clear

  // Use exactly the same frozen coordinates that updateWorld() used.
  if(!visibleEqTopReady) updateVisibleEqTopY();

  for(int b=0;b<8;b++){
    int x=cityLeft+b*(barW+gap);
    int y=visibleEqTopY[b];
    int h=max(2,horizonY-y);

    // Keep the original Sound Runner EQ palette at full saturation:
    // deep navy -> blue -> cyan -> turquoise -> green -> yellow -> orange -> red.
    uint16_t body=EQ_COLORS[b];
    canvas.fillRect(x,y,barW,h,body);
    // v94: 1 px white outline keeps every EQ color distinct from green ground.
    struct tm st;
    localNow(st);
    uint16_t eqOutlineInk=readableInkForSky(skyColorForTime(st));
    canvas.drawRect(x-1,y-1,barW+2,h+2,eqOutlineInk);

    // Tiny contrasting windows so the bars still read as distant buildings.
    // Alternate window brightness based on the building's own luminance.
    int r5=(body>>11)&0x1F;
    int g6=(body>>5)&0x3F;
    int b5=body&0x1F;
    int lum=(299*(r5*255/31)+587*(g6*255/63)+114*(b5*255/31))/1000;
    uint16_t winC=(lum>=135)?0x4208:TFT_WHITE;

    for(int wy=y+4; wy<horizonY-2; wy+=6){
      int wx=x+2;
      if(((wy+b)/2)&1) canvas.drawPixel(wx,wy,winC);
    }
  }
}

static void drawWiFiSetupScreen(){
  canvas.fillScreen(TFT_BLACK);
  canvas.setTextDatum(top_center);

  canvas.setTextColor(TFT_WHITE);
  canvas.setFont(&fonts::FreeSansBold12pt7b);
  canvas.drawString("SOUND TERRARIUM",W/2,10);

  canvas.setFont(&fonts::Font0);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_LIGHTGREY);
  canvas.drawString("inspired by retro arcade",W/2,36);

  canvas.setTextColor(TFT_CYAN);
  canvas.setTextSize(2);
  canvas.drawString("Wi-Fi SETUP",W/2,52);

  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE);
  canvas.drawString("Connect phone / PC to:",W/2,80);
  canvas.setTextColor(TFT_YELLOW);
  canvas.drawString(SETUP_AP_SSID,W/2,94);

  canvas.setTextColor(TFT_WHITE);
  canvas.drawString("Then open:",W/2,109);
  canvas.setTextColor(TFT_GREEN);
  canvas.drawString("http://192.168.4.1",W/2,118);

  canvas.setTextColor(TFT_LIGHTGREY);
  canvas.setTextSize(1);
  canvas.drawString("PRESS ANY KEY TO EXIT",W/2,128);

  canvas.setTextDatum(top_left);
  canvas.pushSprite(0,0);
}

void drawFrame(){
  if(wifiSetupMode){
    drawWiFiSetupScreen();
    return;
  }

  // Compose the complete frame off-screen.
  drawWeatherAndClock();
  drawTerrain();
  drawAudioDiagnostic(); // v91: visible EQ terrain generator, bars meet the road
  drawUfo();
  drawRunner();

  // Final presentation pass. drawClockInfoOverlay() calls localNow() and
  // skyColorForTime() again, so a LOCATION change also refreshes date/weekday/time
  // and selects the correct black/white text ink for that city's current sky.
  drawClockInfoOverlay();

  // One transfer only: the LCD never sees the intermediate erase/draw stages.

// IMU angle diagnostics removed from the scene overlay.

drawNetworkStatusBox();
  canvas.pushSprite(0,0);
}

// ---------------- Setup ----------------
static constexpr int WIFI_SLOT_COUNT=5;

bool readWiFiSlot(int idx, String &ssidOut, String &passOut){
  if(idx<0 || idx>=WIFI_SLOT_COUNT) return false;
  char ks[8],kp[8];
  snprintf(ks,sizeof(ks),"ssid%d",idx);
  snprintf(kp,sizeof(kp),"pass%d",idx);
  wifiPrefs.begin("srwifi",true);
  ssidOut=wifiPrefs.getString(ks,"");
  passOut=wifiPrefs.getString(kp,"");
  wifiPrefs.end();
  return ssidOut.length()>0;
}

void writeWiFiSlot(int idx,const String &ssid,const String &pass){
  if(idx<0 || idx>=WIFI_SLOT_COUNT) return;
  char ks[8],kp[8];
  snprintf(ks,sizeof(ks),"ssid%d",idx);
  snprintf(kp,sizeof(kp),"pass%d",idx);
  wifiPrefs.begin("srwifi",false);
  wifiPrefs.putString(ks,ssid);
  wifiPrefs.putString(kp,pass);
  wifiPrefs.putInt("lastwifi",idx);
  wifiPrefs.end();
}

int getLastWiFiSlot(){
  wifiPrefs.begin("srwifi",true);
  int v=wifiPrefs.getInt("lastwifi",0);
  wifiPrefs.end();
  return v;
}

void setLastWiFiSlot(int idx){
  wifiPrefs.begin("srwifi",false);
  wifiPrefs.putInt("lastwifi",idx);
  wifiPrefs.end();
}

bool loadSoundRunnerWiFi(String &ssid,String &pass){
  // Backward compatibility: migrate old single-entry keys once into slot 0.
  String s0,p0;
  if(readWiFiSlot(0,s0,p0)){
    ssid=s0; pass=p0; return true;
  }

  wifiPrefs.begin("srwifi",true);
  String oldSsid=wifiPrefs.getString("ssid","");
  String oldPass=wifiPrefs.getString("pass","");
  wifiPrefs.end();

  if(oldSsid.length()){
    writeWiFiSlot(0,oldSsid,oldPass);
    ssid=oldSsid; pass=oldPass;
    return true;
  }
  return false;
}

void saveSoundRunnerWiFi(const String &ssid,const String &pass){
  // Update existing saved AP.
  for(int i=0;i<WIFI_SLOT_COUNT;i++){
    String ss,pp;
    if(readWiFiSlot(i,ss,pp) && ss==ssid){
      writeWiFiSlot(i,ssid,pass);
      return;
    }
  }

  // Add to first empty slot.
  for(int i=0;i<WIFI_SLOT_COUNT;i++){
    String ss,pp;
    if(!readWiFiSlot(i,ss,pp)){
      writeWiFiSlot(i,ssid,pass);
      return;
    }
  }

  // Five already saved: replace the least recently selected simple fallback slot 0.
  writeWiFiSlot(0,ssid,pass);
}

bool findSavedWiFi(const String &ssid,String &passOut,int &slotOut){
  for(int i=0;i<WIFI_SLOT_COUNT;i++){
    String ss,pp;
    if(readWiFiSlot(i,ss,pp) && ss==ssid){
      passOut=pp;
      slotOut=i;
      return true;
    }
  }
  return false;
}

static String urlEncode(const String &src){
  const char *hex="0123456789ABCDEF";
  String out;
  for(size_t i=0;i<src.length();i++){
    uint8_t c=(uint8_t)src[i];
    if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~') out+=(char)c;
    else if(c==' ') out+='%'+String("20");
    else { out+='%'; out+=hex[c>>4]; out+=hex[c&15]; }
  }
  return out;
}

bool setLocationFromCity(const String &city){
  if(city.length()==0 || WiFi.status()!=WL_CONNECTED) return false;
  HTTPClient http;
  String url="http://geocoding-api.open-meteo.com/v1/search?name="+urlEncode(city)+"&count=1&language=en&format=json";
  http.setConnectTimeout(8000); http.setTimeout(8000);
  if(!http.begin(url)) return false;
  int code=http.GET();
  if(code!=HTTP_CODE_OK){ http.end(); return false; }
  String body=http.getString(); http.end();
  JsonDocument doc;
  if(deserializeJson(doc,body)) return false;
  JsonArray results=doc["results"].as<JsonArray>();
  if(results.isNull() || results.size()==0) return false;
  JsonObject r=results[0].as<JsonObject>();
  if(r["latitude"].isNull() || r["longitude"].isNull()) return false;
  locationLatitude=r["latitude"].as<float>();
  locationLongitude=r["longitude"].as<float>();
  String name=r["name"] | city;
  String country=r["country"] | "";
  locationName=name + (country.length()?", "+country:"");
  prefs.putFloat("loclat",locationLatitude);
  prefs.putFloat("loclon",locationLongitude);
  prefs.putString("locname",locationName);

  // LOCATION is a hard context change. Do not leave values from the previous
  // city on screen while the new city's data is being acquired.
  ephemerisDateKey=0;
  lastEphemerisAttemptMs=0;
  solarScheduleValid=false;
  lunarScheduleValid=false;
  prefs.putBool("sunvalid",false);
  prefs.putBool("moonvalid",false);
  sunriseMinutes=6*60;
  sunsetMinutes=18*60;
  moonriseMinutes=18*60;
  moonsetMinutes=6*60;

  weather.updatedMs=0;
  weatherOK=false;
  lastWeatherAttempt=0;
  weather.code=-1;
  weather.mode=WX_DEFAULT;
  weather.cloud=35;
  weather.temperatureC=NAN;
  weather.humidityPct=-1;
  weather.pressureMslHpa=NAN;
  weather.precipitationProbabilityPct=-1;
  weather.online=false;
  weatherDiag="UPDATING...";

  tide.valid=false;
  tide.nextHighEpoch=0;
  tide.nextLowEpoch=0;
  tide.updatedMs=0;
  lastTideAttemptMs=0;
  prefs.putBool("tidevalid",false);

  return true;
}

void startWiFiSetupPortalNonBlocking(){
  static bool handlersRegistered=false;
  wifiSetupMode=true;

  // Stop only the active STA attempt; preserve saved credentials.
  WiFi.disconnect(false, false);
  delay(150);
  WiFi.mode(WIFI_OFF);
  delay(150);
  WiFi.mode(WIFI_AP_STA);
  delay(150);

  bool apStarted = WiFi.softAP(SETUP_AP_SSID);
  if(!apStarted){
    WiFi.mode(WIFI_OFF);
    delay(250);
    WiFi.mode(WIFI_AP);
    delay(150);
    apStarted = WiFi.softAP(SETUP_AP_SSID);
  }

  if(!handlersRegistered){
    wifiSetupServer.on("/",HTTP_GET,[](){
      String savedSsid,savedPass;
      int lastSlot=getLastWiFiSlot();
      String lastSavedSsid,lastSavedPass;
      if(readWiFiSlot(lastSlot,lastSavedSsid,lastSavedPass)){
        savedSsid=lastSavedSsid;
        savedPass=lastSavedPass;
      }

      // Scan nearby Wi-Fi networks and present them as a selectable list.
      int n=WiFi.scanNetworks(false,true,false,300);

      String h=
        "<!doctype html><meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>SOUND TERRARIUM - Wi-Fi SETUP</title>"
        "<style>body{font-family:sans-serif;max-width:520px;margin:24px auto;padding:0 14px}"
        "input,select,button{font-size:18px;width:100%;box-sizing:border-box;padding:10px;margin:5px 0 14px}"
        ".brand{margin-bottom:22px}.brand h1{font-size:25px;margin:0 0 2px}.tag{font-size:14px;font-style:italic;color:#666}"
        ".setup{font-size:20px;font-weight:700;margin-top:10px}.note{font-size:14px;color:#555}</style>"
        "<div class='brand'><h1>SOUND TERRARIUM</h1>"
        "<div class='tag'>inspired by retro arcade</div>"
        "<div class='setup'>Wi-Fi SETUP</div></div>";

      h += "<p><b>Location:</b> " + locationName + "</p>";

      h += "<p><b>Saved Wi-Fi:</b></p><ol>";
      for(int slot=0;slot<WIFI_SLOT_COUNT;slot++){
        String ss,pp;
        if(readWiFiSlot(slot,ss,pp)){
          h += "<li>" + ss + "</li>";
        }
      }
      h += "</ol>";

      h += "<form method='POST' action='/save'>"
           "Wi-Fi network<br><select name='s'>";

      bool savedSeen=false;
      for(int i=0;i<n;i++){
        String ss=WiFi.SSID(i);
        if(ss.length()==0) continue;
        String savedPassForSsid;
        int savedSlot=-1;
        bool isSaved=findSavedWiFi(ss,savedPassForSsid,savedSlot);
        bool selected=(ss==savedSsid);
        if(selected) savedSeen=true;
        h += "<option value='" + ss + "'" + String(selected ? " selected" : "") + ">"
             + ss
             + (isSaved ? " (SAVED)" : "")
             + " (" + String(WiFi.RSSI(i)) + " dBm)"
             + "</option>";
      }

      // Keep a saved SSID selectable even if it is temporarily not in scan results.
      if(savedSsid.length() && !savedSeen){
        h += "<option value='" + savedSsid + "' selected>"
             + savedSsid + " (SAVED) (OUT OF RANGE)"
             + "</option>";
      }

      h += "</select>"
           "Password<br><input name='p' type='password' maxlength='63' value='' "
           "placeholder='Leave blank to keep saved password' autocomplete='new-password'><br>"
           "Location (city)<br><input name='city' maxlength='80' value='' placeholder='e.g. New York, London, Tokyo'><br>"
           "<button type='submit'>SAVE & CONNECT</button>"
           "</form>"
           "<p class='note'>Select your Wi-Fi from the list. Saved passwords are never displayed. "
           "Leave Password blank when reconnecting to an already saved network.</p>";

      WiFi.scanDelete();
      wifiSetupServer.send(200,"text/html",h);
    });

    wifiSetupServer.on("/save",HTTP_POST,[](){
      String newSsid=wifiSetupServer.arg("s");
      String newPass=wifiSetupServer.arg("p");
      String newCity=wifiSetupServer.arg("city");
      newSsid.trim(); newCity.trim();

      String existingPass;
      int existingSlot=-1;
      if(newPass.length()==0 && findSavedWiFi(newSsid,existingPass,existingSlot)){
        newPass=existingPass;
      }

      if(newSsid.length()==0){
        wifiSetupServer.send(400,"text/plain","SSID is required");
        return;
      }

      saveSoundRunnerWiFi(newSsid,newPass);

      wifiSetupServer.send(200,"text/html",
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<h2>Saved.</h2><p>Testing connection now. Watch the Cardputer status box.</p>");

      // Defer all slow work until after this HTTP response has returned.
      pendingSetupSsid=newSsid;
      pendingSetupPass=newPass;
      pendingSetupCity=newCity;
      pendingSetupStage=PSET_START;
      pendingSetupStartedMs=0;
      pendingSetupResponseSentMs=millis();
      pendingLocationChanged=false;
    });

    handlersRegistered=true;
  }

  if(apStarted){
    wifiSetupServer.begin();
  }
}

static void startWiFiRetryNonBlocking(){
  if(wifiSetupMode || pendingSetupStage!=PSET_IDLE || wifiRetryStage!=WRT_IDLE) return;
  if(WiFi.status()==WL_CONNECTED) return;

  wifiOK=false;
  netStage=NET_CONNECTING;
  WiFi.mode(WIFI_STA);

  int lastSlot=getLastWiFiSlot();
  String ssid,pass;
  if(readWiFiSlot(lastSlot,ssid,pass)){
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid.c_str(),pass.c_str());
    wifiRetryStage=WRT_TRY_LAST;
    wifiRetryStageStartedMs=millis();
    return;
  }

  // No last AP available: go straight to an asynchronous scan.
  WiFi.scanDelete();
  int rc=WiFi.scanNetworks(true,true,false,300);
  (void)rc;
  wifiRetryStage=WRT_SCAN_WAIT;
  wifiRetryStageStartedMs=millis();
}

static void serviceWiFiRetryNonBlocking(){
  if(wifiRetryStage==WRT_IDLE) return;

  // Setup portal and /save own Wi-Fi while active.
  if(wifiSetupMode || pendingSetupStage!=PSET_IDLE){
    wifiRetryStage=WRT_IDLE;
    return;
  }

  if(WiFi.status()==WL_CONNECTED){
    wifiOK=true;
    netStage=NET_WIFI_OK;
    wifiRetryStage=WRT_IDLE;
    return;
  }

  const uint32_t now=millis();

  if(wifiRetryStage==WRT_TRY_LAST){
    if((uint32_t)(now-wifiRetryStageStartedMs)<12000UL) return;

    WiFi.disconnect(false,false);
    WiFi.scanDelete();
    int rc=WiFi.scanNetworks(true,true,false,300);
    (void)rc;
    wifiRetryStage=WRT_SCAN_WAIT;
    wifiRetryStageStartedMs=now;
    return;
  }

  if(wifiRetryStage==WRT_SCAN_WAIT){
    int n=WiFi.scanComplete();
    if(n==WIFI_SCAN_RUNNING) return;

    wifiRetryBestSlot=-1;
    wifiRetryBestSsid="";
    wifiRetryBestPass="";
    int bestRssi=-999;

    if(n>=0){
      for(int i=0;i<n;i++){
        String seen=WiFi.SSID(i);
        int rssi=WiFi.RSSI(i);
        for(int slot=0;slot<WIFI_SLOT_COUNT;slot++){
          String ss,pp;
          if(readWiFiSlot(slot,ss,pp) && ss==seen && rssi>bestRssi){
            bestRssi=rssi;
            wifiRetryBestSlot=slot;
            wifiRetryBestSsid=ss;
            wifiRetryBestPass=pp;
          }
        }
      }
    }
    WiFi.scanDelete();

    if(wifiRetryBestSlot<0){
      wifiRetryStage=WRT_IDLE;
      wifiOK=false;
      netStage=NET_NO_WIFI;
      return;
    }

    WiFi.setAutoReconnect(true);
    WiFi.begin(wifiRetryBestSsid.c_str(),wifiRetryBestPass.c_str());
    wifiRetryStage=WRT_TRY_BEST;
    wifiRetryStageStartedMs=now;
    return;
  }

  if(wifiRetryStage==WRT_TRY_BEST){
    if((uint32_t)(now-wifiRetryStageStartedMs)<15000UL) return;

    wifiRetryStage=WRT_IDLE;
    wifiOK=false;
    netStage=NET_NO_WIFI;
    return;
  }
}

void connectWiFi(bool allowSetup){
  wifiOK=false;
  netStage=NET_CONNECTING;

  WiFi.mode(WIFI_STA);
  delay(150);

  // IMPORTANT: migrate credentials saved by the older single-AP versions.
  // v38 had this migration helper but never called it from connectWiFi(),
  // so an AP saved by an older build could appear as 0/5 after reboot.
  String legacySsid,legacyPass;
  loadSoundRunnerWiFi(legacySsid,legacyPass);

  // First try the last saved AP directly. This avoids depending entirely on scan results.
  int lastSlot=getLastWiFiSlot();
  String lastSsid,lastPass;
  if(readWiFiSlot(lastSlot,lastSsid,lastPass)){
    WiFi.setAutoReconnect(true);
    WiFi.begin(lastSsid.c_str(),lastPass.c_str());
    uint32_t t0=millis();
    while(WiFi.status()!=WL_CONNECTED && millis()-t0<12000) delay(100);
    if(WiFi.status()==WL_CONNECTED){
      wifiOK=true;
      wifiSetupMode=false;
      netStage=NET_WIFI_OK;
      return;
    }
    WiFi.disconnect(false,false);
    delay(100);
  }

  // If that AP is unavailable, scan and try the strongest saved AP in range.
  int n=WiFi.scanNetworks(false,true,false,300);
  int bestSlot=-1;
  int bestRssi=-999;
  String bestSsid,bestPass;

  for(int i=0;i<n;i++){
    String seen=WiFi.SSID(i);
    int rssi=WiFi.RSSI(i);

    for(int slot=0;slot<WIFI_SLOT_COUNT;slot++){
      String ss,pp;
      if(readWiFiSlot(slot,ss,pp) && ss==seen){
        if(rssi>bestRssi){
          bestRssi=rssi;
          bestSlot=slot;
          bestSsid=ss;
          bestPass=pp;
        }
      }
    }
  }
  WiFi.scanDelete();

  if(bestSlot>=0){
    WiFi.setAutoReconnect(true);
    WiFi.begin(bestSsid.c_str(),bestPass.c_str());
    uint32_t t0=millis();
    while(WiFi.status()!=WL_CONNECTED && millis()-t0<15000) delay(100);

    wifiOK=(WiFi.status()==WL_CONNECTED);
    if(wifiOK){
      setLastWiFiSlot(bestSlot);
      wifiSetupMode=false;
      netStage=NET_WIFI_OK;
      return;
    }
  }

  if(allowSetup){
    netStage=NET_SETUP;
    startWiFiSetupPortalNonBlocking();
  }else{
    // Offline operation is valid. A failed reconnect must not trap the user
    // back in SETUP after they explicitly exited it.
    netStage=NET_NO_WIFI;
    wifiSetupMode=false;
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
  }
}

static void servicePendingWiFiSetup(){
  if(pendingSetupStage==PSET_IDLE) return;

  if(pendingSetupStage==PSET_START){
    // Preserve an explicit delivery margin after WebServer::send().
    // The browser is connected through the SoftAP we are about to tear down.
    if((uint32_t)(millis()-pendingSetupResponseSentMs)<300UL) return;

    wifiSetupMode=false;
    wifiSetupServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(pendingSetupSsid.c_str(),pendingSetupPass.c_str());
    wifiOK=false;
    netStage=NET_CONNECTING;
    pendingSetupStartedMs=millis();
    pendingSetupStage=PSET_WAIT_WIFI;
    return;
  }

  if(pendingSetupStage==PSET_WAIT_WIFI){
    if(WiFi.status()==WL_CONNECTED){
      wifiOK=true;
      int savedSlot=-1;
      String savedPassCheck;
      if(findSavedWiFi(pendingSetupSsid,savedPassCheck,savedSlot) && savedSlot>=0){
        setLastWiFiSlot(savedSlot);
      }
      netStage=NET_WIFI_OK;
      pendingSetupStage=PSET_NTP;
      return;
    }

    if((uint32_t)(millis()-pendingSetupStartedMs)>=20000UL){
      wifiOK=false;
      netStage=NET_NO_WIFI;
      pendingSetupStage=PSET_DONE;
    }
    return;
  }

  if(pendingSetupStage==PSET_NTP){
    // One network operation per loop pass. Individual library calls are still
    // synchronous, but the previous 24s monolithic refresh is eliminated.
    tryNTP();
    pendingSetupStage=PSET_LOCATION;
    return;
  }

  if(pendingSetupStage==PSET_LOCATION){
    pendingLocationChanged=false;
    if(pendingSetupCity.length()){
      pendingLocationChanged=setLocationFromCity(pendingSetupCity);
    }
    pendingSetupStage=PSET_WEATHER;
    return;
  }

  if(pendingSetupStage==PSET_WEATHER){
    fetchWeather();
    pendingSetupStage=PSET_TIDE;
    return;
  }

  if(pendingSetupStage==PSET_TIDE){
    fetchTides();
    pendingSetupStage=PSET_EPHEMERIS;
    return;
  }

  if(pendingSetupStage==PSET_EPHEMERIS){
    fetchDailyEphemeris();
    pendingSetupStage=PSET_DONE;
    return;
  }

  if(pendingSetupStage==PSET_DONE){
    if(pendingLocationChanged && wifiOK){
      struct tm refreshedLocal;
      localNow(refreshedLocal);
      Serial.printf("[LOCATION] %s  %04d-%02d-%02d %02d:%02d  TEMP %.1fC HUM %d%%\n",
                    locationName.c_str(),
                    refreshedLocal.tm_year+1900,refreshedLocal.tm_mon+1,refreshedLocal.tm_mday,
                    refreshedLocal.tm_hour,refreshedLocal.tm_min,
                    weather.temperatureC,weather.humidityPct);
    }

    pendingSetupStage=PSET_IDLE;
    pendingSetupSsid="";
    pendingSetupPass="";
    pendingSetupCity="";
    pendingSetupStartedMs=0;
    pendingSetupResponseSentMs=0;
    pendingLocationChanged=false;
    return;
  }
}

void setup(){
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[BOOT] SOUND TERRARIUM v108am tide vertical layout");
  delay(100);

  // Initialize only the LCD; do not globally initialize M5Unified/Cardputer.
  M5Cardputer.Display.begin();
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setBrightness(128);
  M5Cardputer.Display.fillScreen(TFT_BLACK);

  // Initialize BMI270 first without M5.begin()/M5Cardputer.begin().
  imuReady=initAdvImuOnce();

  // Bruce 1.16.1 ADV microphone path, with I2S0 untouched by M5Cardputer.begin().
  micReady = initBruceADVMicrophone();

  canvas.setColorDepth(16);
  if(!canvas.createSprite(W,H)){
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextColor(TFT_RED,TFT_BLACK);
    M5Cardputer.Display.drawString("Canvas alloc failed",4,4);
    while(true) delay(1000);
  }
  canvas.setTextWrap(false);

  prefs.begin("sndrunner",false);
  int64_t saved=prefs.getLong64("epoch",0);
  if(saved>1704067200) fallbackEpoch=(time_t)saved;
  fallbackMillis0=millis();
  loadSavedWeather();

  // Initialize only the ADV keyboard controller on the already-active Wire1 bus.
  // This does not call M5Cardputer.begin() and does not touch I2S0.
  advKeyboardReady=initAdvCursorKeys();

  for(int x=0;x<W;x++){
    terrain[x]=(uint8_t)(108 + 3*sinf(x*0.05f));
  }
  lastCommittedTerrainY=(float)terrain[W-1];
  lastCommittedTerrainValid=true;

  connectWiFi();
  if(wifiOK){
    tryNTP();
    fetchWeather();
    fetchTides();
    fetchDailyEphemeris();
  }

  runner.y=groundAt((int)runner.x+5)-11;
  lastWorldMs=millis();
}

// ---------------- Main loop ----------------

void drawNetworkStatusBox(){
  if(!showAuxInfo) return;
  canvas.setFont(&fonts::Font0);
  canvas.setTextSize(1);
  struct tm st;
  localNow(st);
  uint16_t groundInk=readableInkForSky(skyColorForTime(st));
  canvas.setTextColor(groundInk);
  canvas.setTextDatum(bottom_right);

  // Avoid allocating a temporary String on every rendered frame.
  static char line[48]="WiFi SETUP";
  static uint32_t lastRefreshMs=0;
  static wl_status_t lastStatus=WL_NO_SHIELD;
  wl_status_t status=WiFi.status();
  uint32_t nowMs=millis();
  if(status!=lastStatus || lastRefreshMs==0 || (uint32_t)(nowMs-lastRefreshMs)>=5000UL){
    lastStatus=status;
    lastRefreshMs=nowMs;
    if(status==WL_CONNECTED){
      String ssid=WiFi.SSID();
      snprintf(line,sizeof(line),"AP: %.32s",ssid.c_str());
    }else{
      snprintf(line,sizeof(line),"WiFi SETUP");
    }
  }
  canvas.drawString(line,W-3,H-2);
  canvas.setTextDatum(top_left);
}

void drawWeatherDiagnostic(){
  if(weatherOK && millis()>20000) return;
  canvas.setFont(&fonts::Font0);
  canvas.setTextSize(1);
  struct tm st;
  localNow(st);
  uint16_t statusInk=readableInkForSky(skyColorForTime(st));
  canvas.setTextColor(statusInk);
  canvas.setTextDatum(bottom_right);
  canvas.drawString(weatherDiag,W-3,H-2);
  canvas.setTextDatum(top_left);
}

void loop(){
  servicePendingWiFiSetup();
  serviceWiFiRetryNonBlocking();
  updateAdvImu();
  updateOffscreenRunnerRescue();
  if(wifiSetupMode) wifiSetupServer.handleClient();
  // Keep network services alive after reset or a temporary Wi-Fi drop.
  if(WiFi.status()==WL_CONNECTED){
    wifiOK=true;

    // If Wi-Fi came back but NTP has not succeeded in this boot, get fresh time now.
    if(!ntpOK && (lastNtpAttemptMs==0 || (uint32_t)(millis()-lastNtpAttemptMs)>=NTP_RETRY_MS)){
      tryNTP();
    }
  }else{
    wifiOK=false;

    // Do not leave the clock permanently on stale fallback time.
    // Retry saved Wi-Fi periodically when not actively using the setup portal.
    if(!wifiSetupMode && pendingSetupStage==PSET_IDLE &&
       wifiRetryStage==WRT_IDLE &&
       (lastWiFiRetryMs==0 || millis()-lastWiFiRetryMs>WIFI_RETRY_MS)){
      lastWiFiRetryMs=millis();
      startWiFiRetryNonBlocking();
    }
  }

  // Preserve the best known clock while running, including offline operation.
  // A 10-minute cadence limits flash writes while greatly reducing fallback staleness.
  if(lastEpochSaveMs==0 || millis()-lastEpochSaveMs>=10UL*60UL*1000UL){
    time_t keep=safeEpoch();
    if(keep>1704067200) prefs.putLong64("epoch",(int64_t)keep);
    lastEpochSaveMs=millis();
  }

  analyzeAudio();
  pollAdvCursorKeys();
  updateWorld();
  updateUfo();

  // Weather refresh is deliberately sparse.
  if(wifiOK) fetchWeather();
  serviceTides();

  // Sun/Moon rise-set is daily data. After local midnight, fetch once;
  // if unavailable, retry hourly until today's complete set succeeds.
  serviceDailyEphemeris();

  drawFrame();
  delay(8);
}
