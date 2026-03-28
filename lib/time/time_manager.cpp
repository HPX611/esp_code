#include "time_manager.h"

int currentHour = 12;
int currentMinute = 0;
int currentSecond = 0;
int currentYear = 2024;
int currentMonth = 1;
int currentDay = 1;

bool networkConnected = true;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

unsigned long lastSyncTime = 0;
unsigned long lastSyncEpoch = 0;

Preferences timePrefs;
const char* TIME_PREF_NAMESPACE = "time-config";
const char* TIME_PREF_KEY = "epoch";

void applyTimeFromEpoch(unsigned long epoch) {
    struct tm* ptm = gmtime((time_t*)&epoch);
    if (ptm == NULL) return;
    
    currentHour = (ptm->tm_hour + 8) % 24;
    currentMinute = ptm->tm_min;
    currentSecond = ptm->tm_sec;
    currentYear = ptm->tm_year + 1900;
    currentMonth = ptm->tm_mon + 1;
    currentDay = ptm->tm_mday;
}

void syncNTPTime() {
    if (!timeClient.update()) return;
    
    unsigned long epoch = timeClient.getEpochTime();
    if (epoch == 0) return;
    
    lastSyncTime = millis();
    lastSyncEpoch = epoch;
    
    timePrefs.begin(TIME_PREF_NAMESPACE, false);
    timePrefs.putULong(TIME_PREF_KEY, epoch);
    timePrefs.end();
    
    applyTimeFromEpoch(epoch);
}

void timeSetup() {
    timeClient.begin();
    
    timePrefs.begin(TIME_PREF_NAMESPACE, false);
    unsigned long savedEpoch = timePrefs.getULong(TIME_PREF_KEY, 0);
    timePrefs.end();
    
    if (savedEpoch > 0) {
        lastSyncEpoch = savedEpoch;
        lastSyncTime = millis();
        applyTimeFromEpoch(savedEpoch);
    }
}

void updateTime() {
    static unsigned long lastNetworkCheck = 0;
    static unsigned long lastNTPSync = 0;
    
    if (millis() - lastNetworkCheck >= NETWORK_CHECK_INTERVAL) {
        lastNetworkCheck = millis();
        networkConnected = (WiFi.status() == WL_CONNECTED);
    }
    
    if (networkConnected && millis() - lastNTPSync >= NTP_UPDATE_INTERVAL) {
        lastNTPSync = millis();
        syncNTPTime();
    }
    
    if (lastSyncEpoch > 0) {
        unsigned long currentEpoch = lastSyncEpoch + (millis() - lastSyncTime) / 1000;
        applyTimeFromEpoch(currentEpoch);
    }
}

String getFormattedTime() {
    char timeString[6];
    sprintf(timeString, "%02d:%02d", currentHour, currentMinute);
    return String(timeString);
}
