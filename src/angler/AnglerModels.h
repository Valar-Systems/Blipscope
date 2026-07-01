#pragma once

#include <Arduino.h>
#include <vector>
#include <utility>
#include <ArduinoJson.h>

// Typed models for the Angler edition's Stage-2 live feeds, plus the free-function parsers that build
// them off the wire and the request/result envelopes the background poller hands across the task
// boundary. Mirrors the Space/Seismic models' shape (feed-agnostic structs + parsers + a fetch
// envelope). Every source is keyless: NOAA CO-OPS (tides + water temp, US) and Open-Meteo (weather +
// barometer trend, and marine wave height + sea-surface temperature, worldwide).
//
// Units: NOAA is fetched in the user's chosen units (english=ft/degF or metric=m/degC), so tide
// heights + water temp are already display-ready. Open-Meteo weather is fetched with matching
// wind/temperature units; pressure_msl is always hPa (converted to inHg for display if imperial).
// Open-Meteo Marine is always metric (m / degC) and converted on the screen.
namespace angler {

// ------------------------------------------------------------------------------------ tides
struct TideEvent {
    time_t t = 0;      // UTC epoch of the extreme
    float height = 0;  // in station units (ft or m)
    bool high = false; // true = high tide, false = low
};
struct TideData {
    bool valid = false;
    std::vector<TideEvent> events;   // upcoming hi/lo, chronological
};

// ---------------------------------------------------------------------------------- weather
struct WeatherData {
    bool valid = false;
    float airTemp = 0;        // display units (degF / degC)
    float windSpeed = 0;      // display units (mph / km/h)
    float windGust = 0;
    int   windDir = 0;        // degrees FROM
    float pressureHpa = 0;    // sea-level pressure, hPa (current)
    int   cloud = 0;          // %
    int   weatherCode = -1;   // WMO weather code
    long  currentEpoch = 0;   // UTC epoch of the current reading
    // Recent sea-level pressure history (UTC epoch, hPa), oldest..newest, for the trend + sparkline.
    std::vector<std::pair<time_t, float>> pressHist;
};

// ----------------------------------------------------------------------------------- marine
struct MarineData {
    bool valid = false;
    bool haveWave = false;   float waveHeightM = 0;  // significant wave height, metres
    bool haveSst = false;    float seaTempC = 0;     // sea-surface temperature, degrees C
};

// -------------------------------------------------------------------------------- envelopes
enum class AnglerEndpoint : uint8_t { Tides, WaterTemp, Weather, Marine };

struct AnglerFetchRequest {
    AnglerEndpoint endpoint = AnglerEndpoint::Weather;
    String url;
    std::vector<std::pair<String, String>> params;
    std::vector<std::pair<String, String>> headers;
};

struct AnglerFetchResult {
    AnglerEndpoint endpoint = AnglerEndpoint::Weather;
    bool ok = false;
    TideData tide;
    WeatherData weather;
    MarineData marine;
    bool haveWaterTemp = false;
    float waterTemp = 0;      // display units (degF / degC)
};

// --------------------------------------------------------------------------------- parsers
// NOAA CO-OPS predictions (time_zone=gmt so `t` is UTC "YYYY-MM-DD HH:MM"). Reads root["predictions"];
// a NOAA {"error":..} or missing array yields valid=false. Cap bounds the retained event list.
bool ParseTides(JsonObjectConst root, TideData& out, size_t cap);
// NOAA water_temperature (date=latest). Reads root["data"][0]["v"]. Returns fetch-ok even when the
// station has no sensor (have=false); false only on an error payload.
bool ParseWaterTemp(JsonObjectConst root, float& tempOut, bool& have);
// Open-Meteo forecast: current{...} + hourly{time[],pressure_msl[]}. histCap bounds pressHist.
bool ParseWeather(JsonObjectConst root, WeatherData& out, size_t histCap);
// Open-Meteo Marine: current{wave_height, sea_surface_temperature} (falls back to hourly[0]).
bool ParseMarine(JsonObjectConst root, MarineData& out);

// TZ-independent "YYYY-MM-DD hh:mm[:ss]" / "YYYY-MM-DDThh:mm[:ss]" (UTC) -> unix epoch; 0 on failure.
long IsoToEpoch(const char* s);

} // namespace angler
