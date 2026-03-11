/**
 * @file slave_comm.h
 * @brief 从机通信模块头文件
 * @details 该文件定义了主机与从机之间的通信协议和接口函数
 * @author ESP32 Developer
 * @date 2026-02-23
 */

#ifndef SLAVE_COMM_H
#define SLAVE_COMM_H

#include <WiFi.h>

/**
 * @section 设备ID定义
 * 用于标识不同类型的从机设备
 */

/**
 * @brief 灯光设备ID
 * @details 用于标识灯光控制从机设备
 */
#define DEVICE_LIGHT  "01"

/**
 * @brief 风扇设备ID
 * @details 用于标识风扇控制从机设备
 */
#define DEVICE_FAN    "02"

/**
 * @section 命令类型定义
 * 用于定义主机发送给从机的命令类型
 */

/**
 * @brief 电源控制命令
 * @details 用于控制设备的电源开关状态
 */
#define CMD_POWER     "01"  // Power control

/**
 * @brief 亮度/速度控制命令
 * @details 用于控制灯光亮度或风扇速度
 */
#define CMD_BRIGHTNESS "02" // Brightness/Speed control

/**
 * @section 状态类型定义
 * 用于定义从机发送给主机的状态类型
 */

/**
 * @brief 电源状态
 * @details 用于报告设备的电源开关状态
 */
#define STATUS_POWER  "01"  // Power status

/**
 * @brief 数值状态
 * @details 用于报告设备的亮度或速度值
 */
#define STATUS_VALUE  "02"  // Brightness/Speed value

/**
 * @section 函数声明
 * 从机通信模块的核心功能函数
 */

/**
 * @brief 初始化从机通信
 * @details 启动TCP服务器，准备接收从机连接
 */
void slaveCommSetup();

/**
 * @brief 从机通信主循环
 * @details 处理从机连接、数据接收和发送
 */
void slaveCommLoop();

/**
 * @brief 发送命令到从机
 * @param deviceId 设备ID，如DEVICE_LIGHT或DEVICE_FAN
 * @param commandType 命令类型，如CMD_POWER或CMD_BRIGHTNESS
 * @param parameter 命令参数，根据命令类型不同而不同
 */
void sendCommand(const char* deviceId, const char* commandType, const char* parameter);

/**
 * @brief 检查灯光从机是否连接
 * @return true表示灯光从机已连接，false表示未连接
 */
bool isLightConnected();

/**
 * @brief 检查风扇从机是否连接
 * @return true表示风扇从机已连接，false表示未连接
 */
bool isFanConnected();

/**
 * @brief 获取灯光从机的电源状态
 * @return true表示灯光开启，false表示灯光关闭
 */
bool isLightOn();

/**
 * @brief 获取风扇从机的电源状态
 * @return true表示风扇开启，false表示风扇关闭
 */
bool isFanOn();

#endif // SLAVE_COMM_H