#ifndef APP_FLASH_SIMPLE_H_
#define APP_FLASH_SIMPLE_H_




#include "stdint.h"
#include "stddef.h"



#define DEVICE_CONFIG_VER_MAJOR                         1U                                                      // 主版本
#define DEVICE_CONFIG_VER_MINOR                         0U                                                      // 次版本
#define DEVICE_CONFIG_VER_PATCH                         0U                                                      // 修订号

#define DEVICE_CONFIG_V1_PAYLOAD_SIZE_BYTES             512U                                     // 主版本 1 的载荷容量，单位字节
// #define DEVICE_CONFIG_V2_PAYLOAD_SIZE_BYTES             1024U                                    // 主版本 2 的载荷容量，单位字节---后续需要扩容启用这里
#define DEVICE_CONFIG_PAYLOAD_SIZE_BYTES                DEVICE_CONFIG_V1_PAYLOAD_SIZE_BYTES      // 当前版本载荷容量，单位字节
#define DEVICE_CONFIG_RESERVED_SIZE                     445U                                                    // 后续版本扩展预留空间，单位字节
#define DEVICE_CONFIG_VALID_SIZE_BYTES                  ((uint32_t)offsetof(Device_CFG_Params_t, Offset))       // 当前版本有效配置长度，单位字节
#define DEVICE_CONFIG_VER_CODE                          ((DEVICE_CONFIG_VER_MAJOR * 10000U) + (DEVICE_CONFIG_VER_MINOR * 100U) + DEVICE_CONFIG_VER_PATCH)    // 配置格式版本编码
// Flash记录格式


/**
 * @brief 设备运行配置。
 * @note 结构体固定为 512 字节，按 4 字节对齐，后续增加字段时须相应缩减 Offset 预留区，不能调整已有字段顺序。
 */
typedef struct __attribute__((aligned(4))) {
    uint8_t Device_Business_Mode;                       // 设备业务模式

    uint8_t Device_Hard_SW;                             // 系统硬开关标志位
    uint8_t Device_Soft_SW;                             // 系统软开关标志位
    uint8_t Device_Silent_SW;                           // 系统静默开关标志位（什么功能都不起作用，只存活）

    uint8_t Device_Night_Mute_SW;                       // 夜间静音 - 开关
    uint8_t Device_Night_Mute_Start_Hour;               // 夜静开始时间-小时
    uint8_t Device_Night_Mute_Start_Minute;             // 夜静开始时间-分钟
    uint8_t Device_Night_Mute_End_Hour;                 // 夜静结束时间-小时
    uint8_t Device_Night_Mute_End_Minute;               // 夜静结束时间-分钟

    uint8_t Device_Fragment_Mute_SW;                    // 时间段静音 - 总开关
    uint8_t Device_Fragment_Mute_1_SW;                  // 时间段1静音 - 开关
    uint8_t Device_Fragment_Mute_1_Start_Hour;          // 时间段1开始时间-小时
    uint8_t Device_Fragment_Mute_1_Start_Minute;        // 时间段1开始时间-分钟
    uint8_t Device_Fragment_Mute_1_End_Hour;            // 时间段1结束时间-小时
    uint8_t Device_Fragment_Mute_1_End_Minute;          // 时间段1结束时间-分钟

    uint8_t Device_Fragment_Mute_2_SW;                  // 时间段2静音 - 开关
    uint8_t Device_Fragment_Mute_2_Start_Hour;          // 时间段2开始时间-小时
    uint8_t Device_Fragment_Mute_2_Start_Minute;        // 时间段2开始时间-分钟
    uint8_t Device_Fragment_Mute_2_End_Hour;            // 时间段2结束时间-小时
    uint8_t Device_Fragment_Mute_2_End_Minute;          // 时间段2结束时间-分钟

    uint8_t Device_Fragment_Mute_3_SW;                  // 时间段3静音 - 开关
    uint8_t Device_Fragment_Mute_3_Start_Hour;          // 时间段3开始时间-小时
    uint8_t Device_Fragment_Mute_3_Start_Minute;        // 时间段3开始时间-分钟
    uint8_t Device_Fragment_Mute_3_End_Hour;            // 时间段3结束时间-小时
    uint8_t Device_Fragment_Mute_3_End_Minute;          // 时间段3结束时间-分钟

    uint8_t Device_Ultrasonic_SW;                       // 超声波 - 开关
    uint8_t Device_Ultrasonic_Mode;                     // 超声波模式（正常/特殊）（对应阶梯/扫频）
    uint8_t Device_Ultrasonic_Start_Hz;                 // 超声波开始频率
    uint8_t Device_Ultrasonic_End_Hz;                   // 超声波结束频率
    uint8_t Device_Ultrasonic_Schedule_SW;              // 超声波定时 - 开关
    uint8_t Device_Ultrasonic_Schedule_Span;            // 超声波定时跨度，分钟

    uint8_t Device_Simulated_SW;                        // 模拟声 - 开关
    uint8_t Device_Simulated_Mode;                      // 模拟声模式(1.顺序  2.随机)
    uint8_t Device_Simulated_Select_Number_Max;         // 模拟声音频最多选取数量
    uint8_t Device_Simulated_Schedule_SW;               // 模拟声定时 - 开关
    uint8_t Device_Simulated_Schedule_Span;             // 模拟声定时跨度，分钟

    uint8_t Device_R_B_LED_SW;                          // 红蓝光 - 开关
    uint8_t Device_R_B_LED_Mode;                        // 红蓝光模式(正常/特殊)(对应交替闪烁/同频闪烁)
    uint8_t Device_R_B_LED_Frequency;                   // 红蓝光频率
    uint8_t Device_R_B_LED_Schedule_SW;                 // 红蓝光定时 - 开关
    uint8_t Device_R_B_LED_Schedule_Span;               // 红蓝光定时跨度，分钟

    uint8_t Device_Laser_SW;                            // 激光 - 开关
    uint8_t Device_Laser_Mode;                          // 激光模式（正常/特殊）（对应轮询/花样预设数组）
    uint8_t Device_Laser_Frequency;                     // 激光频率
    uint8_t Device_Laser_Schedule_SW;                   // 激光定时 - 开关
    uint8_t Device_Laser_Schedule_Span;                 // 激光定时跨度，分钟

    uint8_t Device_Soft_Schedule_SW;                    // 软定时 - 开关
    uint8_t Device_Soft_Schedule_Span;                  // 软定时跨度，分钟

    uint8_t Device_Hard_Schedule_Mode;                  // 硬定时
    uint8_t Device_Hard_Schedule_Span;                  // 硬定时跨度，分钟

    uint8_t Device_Logic_Volume;                        // 逻辑音量等级：0关、1低、2中、3高、4最大
    uint8_t Device_Real_Volume;                         // 真实物理音量值：

    uint8_t Device_Cooling_Mode;                        // 设备冷却模式(固定/递增)
    uint8_t Device_Cooling_Initial_Duration;            // 设备初始冷却时间，分钟
    uint8_t Device_Cooling_Increase_Time;               // 设备连续触发递增冷却时间，分钟

    uint8_t Device_Location_SW;                         // 系统定位开关
    uint8_t Device_Location_Result_Broadcast_SW;        // 系统定位结果播报开关
    uint8_t Device_Location_Report_SW;                  // 系统定位上报开关
    uint8_t Device_Location_Report_Broadcast_SW;        // 系统定位上报播报开关
    uint8_t Device_Location_Frequency_Hour;             // 系统定位频率，小时
    uint8_t Device_Location_Frequency_Minute;           // 系统定位频率，分钟

    uint8_t Device_R5a_Id[6];                           // 5位遥控器ID+结束符
    uint8_t Offset[DEVICE_CONFIG_RESERVED_SIZE];        // 后续版本扩展预留字段
} Device_CFG_Params_t;


// 用户配置 Flash 记录，包含记录头和固定容量载荷
typedef struct __attribute__((aligned(4))) {
    uint32_t version;                                                                   // 配置结构版本
    uint32_t payload_valid_size;                                                        // 当前版本实际有效配置长度，单位字节
    uint32_t crc32;                                                                     // CRC32校验码(先)
    Device_CFG_Params_t payload;                                                        // 配置数据(后)
} DeviceConfigRecord_t;




int32_t App_Flash_Init(void);



//----------------------------------------用户配置相关-----------------------------------------
int32_t App_Flash_DeviceConfig_Load(void);
int32_t App_Flash_DeviceConfig_Get(Device_CFG_Params_t *config);
int32_t App_Flash_DeviceConfig_Save(const Device_CFG_Params_t *config);
int32_t App_Flash_DeviceConfig_Reset(void);
void App_Flash_DeviceConfig_Prf(void);








#endif // APP_FLASH_SIMPLE_H_
