#ifndef ICON_MANAGER_H
#define ICON_MANAGER_H

/**
 * @file icon_manager.h
 * @brief 图标状态管理模块
 * @details 负责管理系统中所有图标的状态，提供统一的接口给各模块更新图标状态
 */

// 图标类型定义
typedef enum {
    ICON_LIGHT,     // 灯光图标
    ICON_FAN,       // 风扇图标
    ICON_AUTO,      // 自动模式图标
    ICON_WIFI,      // WiFi图标
    ICON_WEATHER,   // 天气图标
    ICON_COUNT      // 图标总数
} IconType;

// 图标状态定义
typedef enum {
    ICON_STATE_OFF,     // 关闭/离线
    ICON_STATE_ON,      // 开启/在线
    ICON_STATE_ERROR    // 错误状态
} IconState;

/**
 * @brief 初始化图标管理器
 */
void iconManagerSetup();

/**
 * @brief 更新图标状态
 * @param type 图标类型
 * @param state 图标状态
 */
void updateIconState(IconType type, IconState state);

/**
 * @brief 获取图标状态
 * @param type 图标类型
 * @return 图标状态
 */
IconState getIconState(IconType type);

#endif // ICON_MANAGER_H
