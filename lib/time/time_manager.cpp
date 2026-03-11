#include "time_manager.h"

int currentHour = 12;
int currentMinute = 0;
int currentSecond = 0;
int currentYear = 2024;
int currentMonth = 1;
int currentDay = 1;

bool networkConnected = true;
bool lastNetworkStatus = true;
unsigned long lastNetworkCheck = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

unsigned long lastSyncTime = 0;
unsigned long lastSyncEpoch = 0;
bool usingOfflineTime = false;

Preferences timePrefs;
const char* TIME_PREF_NAMESPACE = "time-config";
const char* TIME_PREF_KEY = "epoch";

void timeSetup() {
    timeClient.begin();
    
    timePrefs.begin(TIME_PREF_NAMESPACE, false);
    unsigned long savedEpoch = timePrefs.getULong(TIME_PREF_KEY, 0);
    timePrefs.end();
    
    if (savedEpoch > 0) {
        lastSyncEpoch = savedEpoch;
        lastSyncTime = millis();
        usingOfflineTime = true;
        
        struct tm *ptm = gmtime((time_t *)&lastSyncEpoch);
        if (ptm != NULL) {
            currentHour = (ptm->tm_hour + 8) % 24;
            currentMinute = ptm->tm_min;
            currentSecond = ptm->tm_sec;
            currentYear = ptm->tm_year + 1900;
            currentMonth = ptm->tm_mon + 1;
            currentDay = ptm->tm_mday;
        }
    }
}

void updateTime() {
    if (millis() - lastNetworkCheck >= NETWORK_CHECK_INTERVAL) {
        lastNetworkCheck = millis();
        bool previousNetworkStatus = networkConnected;
        networkConnected = WiFi.status() == WL_CONNECTED;
        
        if (previousNetworkStatus && !networkConnected) {
            usingOfflineTime = true;
        }
        
        if (!previousNetworkStatus && networkConnected) {
            usingOfflineTime = false;
            timeClient.update();
        }
    }
    
    static unsigned long lastTimeCalculation = 0;
    if (millis() - lastTimeCalculation >= 1000) {
        lastTimeCalculation = millis();
        
        unsigned long currentEpoch = 0;
        
        if (networkConnected) {
            static unsigned long lastNTPUpdate = 0;
            if (millis() - lastNTPUpdate >= NTP_UPDATE_INTERVAL) {
                lastNTPUpdate = millis();
                if (timeClient.update()) {
                    currentEpoch = timeClient.getEpochTime();
                    struct tm *ptm = gmtime((time_t *)&currentEpoch);
                    
                    if (ptm != NULL) {
                        currentHour = (ptm->tm_hour + 8) % 24;
                        currentMinute = ptm->tm_min;
                        currentSecond = ptm->tm_sec;
                        currentYear = ptm->tm_year + 1900;
                        currentMonth = ptm->tm_mon + 1;
                        currentDay = ptm->tm_mday;
                        
                        lastSyncTime = millis();
                        lastSyncEpoch = currentEpoch;
                        usingOfflineTime = false;
                        
                        timePrefs.begin(TIME_PREF_NAMESPACE, false);
                        timePrefs.putULong(TIME_PREF_KEY, currentEpoch);
                        timePrefs.end();
                    }
                }
            }
            
            if (lastSyncEpoch > 0) {
                currentEpoch = lastSyncEpoch + (millis() - lastSyncTime) / 1000;
            }
        } else {
            if (lastSyncEpoch > 0) {
                currentEpoch = lastSyncEpoch + (millis() - lastSyncTime) / 1000;
            } else {
                currentSecond++;
                if (currentSecond >= 60) {
                    currentSecond = 0;
                    currentMinute++;
                    if (currentMinute >= 60) {
                        currentMinute = 0;
                        currentHour++;
                        if (currentHour >= 24) {
                            currentHour = 0;
                            currentDay++;
                            if (currentDay > 31) {
                                currentDay = 1;
                                currentMonth++;
                                if (currentMonth > 12) {
                                    currentMonth = 1;
                                    currentYear++;
                                }
                            }
                        }
                    }
                }
            }
            usingOfflineTime = true;
        }
        
        if (currentEpoch > 0) {
            struct tm *ptm = gmtime((time_t *)&currentEpoch);
            if (ptm != NULL) {
                currentHour = (ptm->tm_hour + 8) % 24;
                currentMinute = ptm->tm_min;
                currentSecond = ptm->tm_sec;
                currentYear = ptm->tm_year + 1900;
                currentMonth = ptm->tm_mon + 1;
                currentDay = ptm->tm_mday;
            }
        }
    }
}
