#pragma once

#include <Arduino.h>
#include <Preferences.h>

// Spacescope's persistent "spotter's logbook": lifetime tallies of the notable events the device
// has caught, plus an observing-day streak. Its own NVS namespace ("sp-log"), mirroring the radar's
// Logbook / EamLogbook. Fed by the same alert edges in SpaceManager::CheckAlerts, so transient
// notifications accumulate into a personal history that's the reason to keep the device on the shelf.
//
// A "day" is a UTC calendar day on which at least one event was recorded. The streak is the run of
// consecutive such days; it stays "alive" through today/yesterday and reads 0 once a day is missed.
class SpaceLogbook {
public:
    void Begin();                    // load counters from NVS (idempotent)

    // Record one caught event (increments the tally + advances the day streak). nowUtc must be a
    // real (NTP-synced) epoch; calls with an unsynced clock are ignored so the streak stays honest.
    void RecordIssPass(long nowUtc);
    void RecordLaunch(long nowUtc);
    void RecordAurora(long nowUtc);
    void RecordFlare(long nowUtc);
    void RecordAsteroid(long nowUtc);
    void NoteKp(float kp);           // track the highest Kp ever seen (no day effect)

    uint32_t IssPasses() const { return d.issPasses; }
    uint32_t Launches() const  { return d.launches; }
    uint32_t Auroras() const   { return d.auroras; }
    uint32_t Flares() const    { return d.flares; }
    uint32_t Asteroids() const { return d.asteroids; }
    uint32_t Nights() const    { return d.nights; }
    uint32_t BestStreak() const{ return d.bestStreak; }
    float    MaxKp() const     { return d.maxKp; }
    // The live streak: the stored run if it reaches today or yesterday, else 0 (broken).
    uint32_t CurrentStreak(long nowUtc) const;
    bool Any() const { return d.issPasses || d.launches || d.auroras || d.flares || d.asteroids; }

private:
    struct Data {
        uint32_t issPasses = 0, launches = 0, auroras = 0, flares = 0, asteroids = 0;
        uint32_t nights = 0, streak = 0, bestStreak = 0;
        uint32_t lastDay = 0;   // UTC days-since-epoch of the most recent recorded event
        float maxKp = 0;
    } d;
    Preferences prefs;
    void markDay(long nowUtc);  // roll the streak/nights for today
    void save();
};
