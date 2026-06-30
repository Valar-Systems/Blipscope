#include "SpaceLogbook.h"

namespace { constexpr long DAY = 86400; }

void SpaceLogbook::Begin()
{
    prefs.begin("sp-log", false);
    d.issPasses  = prefs.getUInt("iss", 0);
    d.launches   = prefs.getUInt("lau", 0);
    d.auroras    = prefs.getUInt("aur", 0);
    d.flares     = prefs.getUInt("fla", 0);
    d.asteroids  = prefs.getUInt("ast", 0);
    d.nights     = prefs.getUInt("nights", 0);
    d.streak     = prefs.getUInt("streak", 0);
    d.bestStreak = prefs.getUInt("best", 0);
    d.lastDay    = prefs.getUInt("lastday", 0);
    d.maxKp      = prefs.getFloat("maxkp", 0);
}

void SpaceLogbook::save()
{
    prefs.putUInt("iss", d.issPasses);
    prefs.putUInt("lau", d.launches);
    prefs.putUInt("aur", d.auroras);
    prefs.putUInt("fla", d.flares);
    prefs.putUInt("ast", d.asteroids);
    prefs.putUInt("nights", d.nights);
    prefs.putUInt("streak", d.streak);
    prefs.putUInt("best", d.bestStreak);
    prefs.putUInt("lastday", d.lastDay);
    prefs.putFloat("maxkp", d.maxKp);
}

void SpaceLogbook::markDay(long nowUtc)
{
    const uint32_t day = (uint32_t)(nowUtc / DAY);
    if (day == d.lastDay) return;                  // already counted today
    d.streak = (day == d.lastDay + 1) ? d.streak + 1 : 1; // consecutive vs reset
    if (d.streak > d.bestStreak) d.bestStreak = d.streak;
    d.nights++;
    d.lastDay = day;
}

uint32_t SpaceLogbook::CurrentStreak(long nowUtc) const
{
    const uint32_t day = (uint32_t)(nowUtc / DAY);
    return (day >= d.lastDay && day - d.lastDay <= 1) ? d.streak : 0;
}

void SpaceLogbook::RecordIssPass(long nowUtc)  { if (nowUtc < 1600000000) return; d.issPasses++; markDay(nowUtc); save(); }
void SpaceLogbook::RecordLaunch(long nowUtc)   { if (nowUtc < 1600000000) return; d.launches++;  markDay(nowUtc); save(); }
void SpaceLogbook::RecordAurora(long nowUtc)   { if (nowUtc < 1600000000) return; d.auroras++;   markDay(nowUtc); save(); }
void SpaceLogbook::RecordFlare(long nowUtc)    { if (nowUtc < 1600000000) return; d.flares++;    markDay(nowUtc); save(); }
void SpaceLogbook::RecordAsteroid(long nowUtc) { if (nowUtc < 1600000000) return; d.asteroids++; markDay(nowUtc); save(); }

void SpaceLogbook::NoteKp(float kp)
{
    if (kp > d.maxKp) { d.maxKp = kp; prefs.putFloat("maxkp", d.maxKp); }
}
