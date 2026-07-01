#pragma once

#include <Arduino.h>
#include <vector>
#include <utility>

#include "ConfigurationWebServer.h"
#include "OpenSkyAuthTokenHandler.h"
#include "HttpRequestManager.h"
#include "LGFX.h"
#include "BandCanvas.h"
#include "AnglerTheme.h"
#include "Solunar.h"
#include "AnglerFeedClient.h"
#include "AnglerLogbook.h"

// FEATURE_ANGLER top-level controller -- the Angler edition: a desk fishing companion whose hero is
// an on-device SOLUNAR "best bite times" forecast. The sixth sibling to AircraftManager / EamManager
// / SpaceManager / SeismicManager / BirdingManager, selected at compile time in main.cpp (same
// Initialise / Update / Draw surface, driven by the loop task, same shared infra).
//
// UI is the HYBRID shell (like Birding): a dwell-timed auto-rotation that skips empty screens AND
// swipe-to-navigate, plus a tap-to-inspect detail-card overlay. No aircraft PPI sweep (main.cpp
// gates it out of this build); the bite forecast draws a static 24-hour ring.
//
// Stage 1 (this file) is fully on-device: solunar / sun / moon computed from src/astro, no network.
// Later stages add an AnglerFeedClient for keyless live data (NOAA CO-OPS tides + water temp,
// Open-Meteo weather/barometer, NDBC buoys) as purely additive screens + a HasData() case.
class AnglerManager
{
public:
    AnglerManager(ConfigurationWebServer& config, OpenSkyAuthTokenHandler& auth,
                  HttpRequestManager& httpManager, LGFX& tftGfx)
        : configServer(config), authHandler(auth), http(httpManager), tft(tftGfx),
          feed(httpManager)
    {
    }
    ~AnglerManager() = default;

    void Initialise();
    void Update();
    void Draw(BandCanvas& backbuffer, bool firstPass);

private:
    // Stage-1 screens. Order here is the default rotation order; the user enables/orders via the
    // "ang-screens" CSV. Clock is the always-available idle screen; Splash is the cold-start prompt
    // (shown until a location is set and the ephemeris is ready). Later stages add Tides, Barometer,
    // Wind, Water, CatchLog before Splash.
    enum class Screen : uint8_t { Bite, Tides, Barometer, Wind, Water, Moon, Sun, CatchLog,
                                  Splash, Clock, COUNT };

    ConfigurationWebServer& configServer;
    OpenSkyAuthTokenHandler& authHandler;   // reserved (parity with the other apps); unused here
    HttpRequestManager& http;
    LGFX& tft;
    AnglerFeedClient feed;                   // Stage-2 live feeds (tides / weather / marine)
    AnglerLogbook logbook;                   // persistent catch log

    // ---- config-derived state (set in Initialise) ----
    angler::Palette palette = angler::PaletteDefault();
    bool hasLatLon = false;
    double deviceLat = 0.0, deviceLon = 0.0;
    long tzOffsetSec = 0;                     // local offset for bite windows / rise-set / clock
    String tideStation;                       // NOAA CO-OPS station id (empty = no tide screens)
    bool imperial = true;                     // ft/degF/mph/inHg vs m/degC/(km/h)/hPa
    std::vector<Screen> enabledOrder;
    uint8_t configuredBrightness = 255;
    bool autoDim = true;

    // ---- solunar cache (recomputed ~1/min on the loop task; on-device, no network) ----
    angler::SolunarDay today;                 // local day containing now
    angler::SolunarDay tomorrow;              // next day, for the roll-over "next bite" search
    bool solunarValid = false;
    unsigned long lastSolunarCalcMs = 0;
    long solunarDayStart = 0;                 // detect a local-day roll-over between recomputes

    // ---- navigation / selection ----
    Screen current = Screen::Splash;
    unsigned long lastAdvanceMs = 0;
    unsigned long lastInteractionMs = 0;
    bool inDetail = false;
    int selectedPeriod = -1;                  // Bite screen: which period the detail card shows (-1 = day summary)

    // ---- brightness / night-dim ----
    uint8_t currentBrightness = 255;
    bool nightDim = false;
    unsigned long lastBrightnessCheck = 0;

    // ---- ntfy + chime alerts (shared ntfy-topic key + POST pattern) ----
    String ntfyTopic;
    bool alertBite = true;                    // push when a major feeding window opens
    bool alertBaro = false;                   // push when the barometer is falling fast
    bool alertTide = false;                   // push when a hi/lo tide is ~30 min away
    bool chimeOnAlert = true;                 // also chirp the speaker (HAS_AUDIO)
    // edge state so each alert fires once per event, seeded at boot so the backlog never fires
    time_t lastAlertedBiteCenter = 0;
    bool baroAlerted = false;                 // a fast-fall episode already alerted (reset when it eases)
    time_t lastAlertedTide = 0;               // tide event already alerted for
    bool alertSeeded = false;
    unsigned long lastNotifyMs = 0;

    // ---- touch / gestures ----
    bool wasTouched = false;
    int touchStartX = 0, touchStartY = 0;
    int touchLastX = 0, touchLastY = 0;

    // rotation
    std::vector<Screen> BuildRotation() const;
    bool HasData(Screen s) const;
    void AdvanceRotation(int dir);
    void AutoRotate();

    // input
    void HandleTouch();
    void HandleTap(int tx, int ty);
    void ExitDetail() { inDetail = false; selectedPeriod = -1; }

    // brightness / alerts
    void UpdateBrightness();
    float GlowFactor() const { return nightDim ? 0.5f : 1.0f; }
    void RecomputeSolunar(bool force);
    void SelfCheck();       // one-shot Serial cross-check of the solunar math vs the NYC reference
    void CheckAlerts();
    void SendNtfy(const String& title, const String& body, const String& tags, int priority);

    // screens (defined in AnglerScreens.cpp)
    void DrawBite(BandCanvas& c);
    void DrawTides(BandCanvas& c);
    void DrawBarometer(BandCanvas& c);
    void DrawWind(BandCanvas& c);
    void DrawWater(BandCanvas& c);
    void DrawMoon(BandCanvas& c);
    void DrawSun(BandCanvas& c);
    void DrawCatchLog(BandCanvas& c);
    void DrawSplash(BandCanvas& c);
    void DrawClock(BandCanvas& c);
    void DrawDetailCard(BandCanvas& c);
    void DrawScreenDots(BandCanvas& c, const std::vector<Screen>& rot) const;

    // solunar helpers (shared by screens + alerts)
    bool NextPeriod(time_t nowUtc, angler::Period& out, bool majorOnly) const;
    bool ActiveNow(time_t nowUtc, angler::Period& out) const;      // a period active right now
    std::pair<int, int> PeriodMarkerXY(const angler::Period& p) const;  // Bite ring position
    int PeriodHitTest(int tx, int ty) const;                       // nearest period marker; -1 if none
    // Shared 24-hour bite-ring geometry (midnight at top, clockwise), used by both the drawing and
    // the hit-test so a tapped marker always matches where it was painted.
    static std::pair<int, int> RingXY(long sodLocal, int radius);
    static int BiteRingRadius();
    long LocalSod(time_t utc) const { return (((long)utc + tzOffsetSec) % 86400 + 86400) % 86400; }

    // small helpers
    static std::vector<String> SplitList(const String& s, bool lower);
    void CenterText(BandCanvas& c, const String& s, int y, uint32_t color);
    String LocalHM(time_t utc) const;                              // UTC epoch -> local "HH:MM"
    static String HM(time_t utc, long tzSec);

    bool TimeReady() const { return time(nullptr) > 1600000000; }   // NTP-synced clock

    // Barometer trend derived from the weather feed's hourly sea-level pressure history.
    struct BaroTrend { bool valid = false; float rateHpaPerH = 0; float nowHpa = 0; float pastHpa = 0; };
    BaroTrend ComputeBaro() const;

    // unit-aware display helpers (imperial flag from config)
    const char* TempUnit()  const { return imperial ? "F" : "C"; }
    const char* WindUnit()  const { return imperial ? "mph" : "km/h"; }
    const char* WaveUnit()  const { return imperial ? "ft" : "m"; }
    const char* PressUnit() const { return imperial ? "inHg" : "hPa"; }
    float SeaTempDisp(float c)  const { return imperial ? c * 1.8f + 32.0f : c; }   // marine SST is degC
    float WaveDisp(float m)     const { return imperial ? m * 3.28084f : m; }        // marine wave is m
    float PressDisp(float hpa)  const { return imperial ? hpa * 0.02953f : hpa; }    // pressure is hPa
    static const char* CompassPoint(int deg);
};
