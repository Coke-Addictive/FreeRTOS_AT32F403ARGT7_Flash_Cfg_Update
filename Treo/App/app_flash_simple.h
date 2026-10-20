/**
 * @file app_flash_simple.h
 * @brief 设备用户配置定义、Flash 存储接口及版本升级维护说明。
 * @author Treo
 * @version V1.0.0
 * @date 2026-10-20
 *
 * @par 当前版本
 * V1.0.0，版本编码为 10000；载荷容量为 512 字节，有效字段为 70 字节，预留区为 442 字节。
 * 完整记录为 524 字节；V2 的 1024 字节容量宏仅作预留，当前仍使用 V1 容量。
 * 固定记录头：version 偏移 0，payload_valid_size 偏移 4，crc32 偏移 8，payload 偏移 12。
 * CRC32 仅覆盖对应主版本的完整 payload，包括预留区，不包含记录头。
 * 当前处于未发布调试阶段，本次新增定位方式开关保持 V1.0.0，不做旧布局迁移；以下版本升级规则用于发布后维护。
 *
 * @par 版本编号规则
 * 主版本：载荷容量不足，需要扩容时递增，次版本和修订号归零，例如 V1.0.0 -> V2.0.0。
 * 次版本：在当前载荷容量内追加业务字段时递增，修订号归零，例如 V1.0.0 -> V1.1.0。
 * 修订号：修改变量名称、注释等，不改变存储布局和字段含义，例如 V1.0.0 -> V1.0.1。
 * 编码规则：主版本 * 10000 + 次版本 * 100 + 修订号；次版本和修订号均限定为 0～99。
 * 每次变更同步维护文件头版本、版本宏、相关布局说明和修订记录。
 *
 * @par 发布后固定的约定
 * 已发布字段的类型、长度、偏移、顺序、单位和含义保持不变，仅在 Offset 之前追加新字段。
 * 记录头布局、CRC32 算法及覆盖规则保持不变；已发布主版本的载荷容量宏永久保留原值。
 *
 * @par 同一主版本内追加字段
 * 1. 在 Device_CFG_Params_t 的旧有效字段末尾、Offset 之前增加成员。
 * 2. 根据实际布局缩减 DEVICE_CONFIG_RESERVED_SIZE，计入新增字段所需的对齐填充。
 * 3. 确认 sizeof(Device_CFG_Params_t) 仍等于 DEVICE_CONFIG_PAYLOAD_SIZE_BYTES。
 * 4. 在 DEFAULT_DEVICE_CFG 中补充新增字段默认值，并按需同步参数校验和打印函数。
 * 5. 递增次版本、清零修订号；DEVICE_CONFIG_VALID_SIZE_BYTES 继续通过 offsetof 自动计算。
 *
 * @par 扩容到新主版本
 * 1. 新增或启用目标主版本的 DEVICE_CONFIG_Vn_PAYLOAD_SIZE_BYTES，保留全部历史容量宏。
 * 2. 将 DEVICE_CONFIG_PAYLOAD_SIZE_BYTES 切换到新容量，重新计算预留区并补充新增字段默认值。
 * 3. 在 App_Device_Config_Record_Is_Valid() 中增加目标主版本对应的容量分支。
 * 4. 递增主版本，次版本和修订号归零；按历史容量校验旧记录，再由 Load() 完成升级和写回。
 * 5. 核对完整记录不超过 Flash 配置分区，载荷及记录长度满足 4 字节对齐，同时复核 RAM 和任务栈。
 *
 * @par 升级验证
 * 下载新固件时保留旧配置分区，不执行整片擦除或恢复默认操作。
 * 首次启动应打印版本及有效长度的升级前后值，旧字段保留，新字段获得默认值。
 * 再次复位应直接加载成功，不再重复升级；若回填整份默认配置，则不视为升级测试通过。
 *
 * @par TODO：多任务更新原子性（暂未解决）
 * Get() 和 Save() 分别使用互斥锁，但“读取配置 -> 修改本地副本 -> 保存整份配置”并非原子操作。
 * 两个任务读取同一份旧配置后分别修改不同字段，后保存的旧副本可能覆盖前一个任务已经保存的修改。
 * 例如：任务 A 打开软开关并保存，任务 B 使用此前读取的副本修改静音时间并保存，可能将软开关恢复为旧值。
 * 此时两次 Save() 均可能返回成功，CRC32 校验也正常，因此不能通过保存返回值或 CRC32 发现修改丢失。
 * 解决前应由单一任务负责修改配置；若多个任务修改，调用方需统一串行化完整的读取、修改、保存过程。
 * 后续再确定原子更新接口及锁的管理方式，并验证两个任务修改不同字段时均能保留修改。
 * 本项仅记录待办，当前 Get()、Save() 接口及内部锁逻辑保持不变。
 *
 * @par 修订记录
 * 2026-10-20  V1.0.0  Treo：调试阶段新增北斗、Wi-Fi、基站定位开关，同步默认值、打印及预留区，版本号不变。
 */

#ifndef APP_FLASH_SIMPLE_H_
#define APP_FLASH_SIMPLE_H_




#include "stdint.h"
#include "stddef.h"



#define DEVICE_CONFIG_VER_MAJOR                         1U                                                      // 主版本
#define DEVICE_CONFIG_VER_MINOR                         0U                                                      // 次版本
#define DEVICE_CONFIG_VER_PATCH                         0U                                                      // 修订号

#define DEVICE_CONFIG_V1_PAYLOAD_SIZE_BYTES             512U                                     // 主版本 1 的载荷容量，单位字节
#define DEVICE_CONFIG_V2_PAYLOAD_SIZE_BYTES             1024U                                    // 主版本 2 的载荷容量，单位字节---后续需要扩容启用这里
#define DEVICE_CONFIG_PAYLOAD_SIZE_BYTES                DEVICE_CONFIG_V1_PAYLOAD_SIZE_BYTES      // 当前版本载荷容量，单位字节
// #define DEVICE_CONFIG_PAYLOAD_SIZE_BYTES                DEVICE_CONFIG_V2_PAYLOAD_SIZE_BYTES      // 当前版本载荷容量，单位字节
#define DEVICE_CONFIG_RESERVED_SIZE                     442U                                                    // V1后续版本扩展预留空间，单位字节
// #define DEVICE_CONFIG_RESERVED_SIZE                     xxxU                                                    // V2后续版本扩展预留空间，单位字节
#define DEVICE_CONFIG_VALID_SIZE_BYTES                  ((uint32_t)offsetof(Device_CFG_Params_t, Offset))       // 当前版本有效配置长度，单位字节
#define DEVICE_CONFIG_VER_CODE                          ((DEVICE_CONFIG_VER_MAJOR * 10000U) + (DEVICE_CONFIG_VER_MINOR * 100U) + DEVICE_CONFIG_VER_PATCH)    // 配置格式版本编码



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
    uint8_t Device_Location_BDS_SW;                     // 系统定位-北斗开关：0 关闭，1 开启
    uint8_t Device_Location_WIFI_SW;                    // 系统定位-Wi-Fi 开关：0 关闭，1 开启
    uint8_t Device_Location_LBS_SW;                     // 系统定位-基站开关：0 关闭，1 开启
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
