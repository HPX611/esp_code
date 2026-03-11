#include "icon_manager.h"

// 图标状态数组
static IconState iconStates[ICON_COUNT] = {
    ICON_STATE_OFF,  // ICON_LIGHT
    ICON_STATE_OFF,  // ICON_FAN
    ICON_STATE_OFF,  // ICON_AUTO
    ICON_STATE_OFF,  // ICON_WIFI
    ICON_STATE_OFF   // ICON_WEATHER
};

void iconManagerSetup() {
    // 初始化所有图标状态为默认值
    for (int i = 0; i < ICON_COUNT; i++) {
        iconStates[i] = ICON_STATE_OFF;
    }
}

void updateIconState(IconType type, IconState state) {
    if (type >= 0 && type < ICON_COUNT) {
        iconStates[type] = state;
    }
}

IconState getIconState(IconType type) {
    if (type >= 0 && type < ICON_COUNT) {
        return iconStates[type];
    }
    return ICON_STATE_OFF;
}
