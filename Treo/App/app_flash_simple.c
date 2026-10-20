#include "app_flash_simple.h"


#include "drv_crc32.h"
#include "drv_flash.h"



#include "string.h"



#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"


#include "log.h"



#define APP_FLASH_MUTEX_TIMEOUT_MS                      1000U                   // Flash互斥锁最长等待时间


// 公共状态
static bool s_app_flash_initialized = false;                                    // Flash服务初始化标志
static SemaphoreHandle_t s_app_flash_mutex = NULL;                              // Flash互斥量句柄


// 用户配置相关私有
static bool s_user_config_ready = false;                                        // 用户配置数据加载就绪标志
static DeviceConfigRecord_t s_device_config_record = {0};                       // 当前用户配置RAM缓存
static DeviceConfigRecord_t s_device_config_candidate = {0};                    // 本次操作的候选记录


// 默认配置
static const Device_CFG_Params_t DEFAULT_DEVICE_CFG = {
    .Device_Hard_SW =                       0U,                                 // 系统硬开关默认关闭
    .Device_Soft_SW =                       0U,                                 // 系统软开关默认关闭
    .Device_Silent_SW =                     0U,                                 // 系统静默默认关闭

    .Device_Night_Mute_SW =                 0U,                                 // 夜间静音默认关闭
    .Device_Night_Mute_Start_Hour =         18U,                                // 夜静开始时间 18 点
    .Device_Night_Mute_Start_Minute =       0U,                                 // 夜静开始时间 0 分
    .Device_Night_Mute_End_Hour =           9U,                                 // 夜静结束时间 9 点
    .Device_Night_Mute_End_Minute =         0U,                                 // 夜静结束时间 0 分

    .Device_Fragment_Mute_SW =              0U,                                 // 时间段静音总开关默认关闭
    .Device_Fragment_Mute_1_SW =            0U,                                 // 时间段 1 静音默认关闭
    .Device_Fragment_Mute_1_Start_Hour =    0U,                                 // 时间段 1 开始小时占位，待业务设定
    .Device_Fragment_Mute_1_Start_Minute =  0U,                                 // 时间段 1 开始分钟占位，待业务设定
    .Device_Fragment_Mute_1_End_Hour =      0U,                                 // 时间段 1 结束小时占位，待业务设定
    .Device_Fragment_Mute_1_End_Minute =    0U,                                 // 时间段 1 结束分钟占位，待业务设定

    .Device_Fragment_Mute_2_SW =            0U,                                 // 时间段 2 静音默认关闭
    .Device_Fragment_Mute_2_Start_Hour =    0U,                                 // 时间段 2 开始小时占位，待业务设定
    .Device_Fragment_Mute_2_Start_Minute =  0U,                                 // 时间段 2 开始分钟占位，待业务设定
    .Device_Fragment_Mute_2_End_Hour =      0U,                                 // 时间段 2 结束小时占位，待业务设定
    .Device_Fragment_Mute_2_End_Minute =    0U,                                 // 时间段 2 结束分钟占位，待业务设定

    .Device_Fragment_Mute_3_SW =            0U,                                 // 时间段 3 静音默认关闭
    .Device_Fragment_Mute_3_Start_Hour =    0U,                                 // 时间段 3 开始小时占位，待业务设定
    .Device_Fragment_Mute_3_Start_Minute =  0U,                                 // 时间段 3 开始分钟占位，待业务设定
    .Device_Fragment_Mute_3_End_Hour =      0U,                                 // 时间段 3 结束小时占位，待业务设定
    .Device_Fragment_Mute_3_End_Minute =    0U,                                 // 时间段 3 结束分钟占位，待业务设定

    .Device_Ultrasonic_SW =                 1U,                                 // 超声波默认开启
    .Device_Ultrasonic_Mode =               0U,                                 // 超声波模式占位，待业务定义取值
    .Device_Ultrasonic_Start_Hz =           0U,                                 // 超声波开始频率占位，HZ,待确认取值
    .Device_Ultrasonic_End_Hz =             0U,                                 // 超声波结束频率占位，HZ,待确认取值
    .Device_Ultrasonic_Schedule_SW =        0U,                                 // 超声波定时默认关闭
    .Device_Ultrasonic_Schedule_Span =      0U,                                 // 超声波定时跨度占位，单位分钟

    .Device_Simulated_SW =                  1U,                                 // 模拟声默认开启
    .Device_Simulated_Mode =                0U,                                 // 模拟声默认模式
    .Device_Simulated_Select_Number_Max =   0U,                                 // 模拟声音频选取数量占位，待业务设定
    .Device_Simulated_Schedule_SW =         0U,                                 // 模拟声定时默认关闭
    .Device_Simulated_Schedule_Span =       0U,                                 // 模拟声定时跨度占位，单位分钟

    .Device_R_B_LED_SW =                    1U,                                 // 红蓝光默认开启
    .Device_R_B_LED_Mode =                  0U,                                 // 红蓝光模式占位，待业务定义取值
    .Device_R_B_LED_Frequency =             0U,                                 // 红蓝光频率占位，HZ,待确认取值
    .Device_R_B_LED_Schedule_SW =           0U,                                 // 红蓝光定时默认关闭
    .Device_R_B_LED_Schedule_Span =         0U,                                 // 红蓝光定时跨度占位，单位分钟

    .Device_Laser_SW =                      1U,                                 // 激光-开
    .Device_Laser_Mode =                    0U,                                 // 激光模式占位，待业务定义取值
    .Device_Laser_Frequency =               0U,                                 // 激光频率占位，HZ,待确认取值
    .Device_Laser_Schedule_SW =             0U,                                 // 激光定时默认关闭
    .Device_Laser_Schedule_Span =           0U,                                 // 激光定时跨度占位，单位分钟

    .Device_Soft_Schedule_SW =              0U,                                 // 软定时默认关闭
    .Device_Soft_Schedule_Span =            0U,                                 // 软定时时长，分钟，待确认取值

    .Device_Hard_Schedule_Mode =            0U,                                 // 硬定时默认关闭
    .Device_Hard_Schedule_Span =            0U,                                 // 硬定时时长，分钟，待确认取值

    .Device_Logic_Volume =                  1U,                                 // 默认使用低音量档位
    .Device_Real_Volume =                   0U,                                 // 物理音量占位，待确定低音量档位映射

    .Device_Location_SW =                   1U,                                 // 系统定位-开
    .Device_Location_BDS_SW =               1U,                                 // 北斗 定位-开
    .Device_Location_WIFI_SW =              1U,                                 // Wi-Fi 定位-开
    .Device_Location_LBS_SW =               1U,                                 // 基站 定位-开
    .Device_Location_Result_Broadcast_SW =  1U,                                 // 系统定位结果播报-开
    .Device_Location_Report_SW =            1U,                                 // 系统定位上报-开
    .Device_Location_Report_Broadcast_SW =  1U,                                 // 系统定位上报播报-开
    .Device_Location_Frequency_Hour  =      24U,                                // 定位间隔时间，单位小时
    .Device_Location_Frequency_Minute =     0U,                                 // 定位间隔时间，单位分钟

    .Device_R5a_Id =                        {'0', '0', '0', '0', '0', '\0'},    // 默认遥控器 ID 为 00000
    .Offset =                               {0U}                                // 清零全部预留空间
};





//----------------------------------------------------------工具函数-----------------------------------------------------------------

/**
 * @brief 获取 Flash 服务互斥量。
 * @return 0 成功，-1 尚未初始化，-2 等待超时。
 */
static int32_t App_Flash_Lock(void) {
    if (s_app_flash_mutex == NULL) {
        return -1;
    }

    if (xSemaphoreTake(s_app_flash_mutex, pdMS_TO_TICKS(APP_FLASH_MUTEX_TIMEOUT_MS)) != pdTRUE) {
        LOG_ERROR("等待Flash互斥量超时");
        return -2;
    }

    return 0;
}




/**
 * @brief 释放Flash服务互斥量。
 */
static void App_Flash_Unlock(void) {
    if (s_app_flash_mutex != NULL) {
        (void)xSemaphoreGive(s_app_flash_mutex);
    }
}

/**
 * @brief 一次性计算指定缓冲区的 CRC32，每次计算前复位硬件累计状态。
 * @param data 待计算数据的只读缓冲区，地址不要求按 4 字节对齐，调用期间内容须保持不变。
 * @param data_size 数据长度，单位字节，须大于 0 且为 4 的倍数，不自动补齐。
 * @param out_crc32 CRC32 结果的输出地址，成功时写入，失败时保持原值。
 * @return true 表示计算成功，false 表示参数非法或底层操作失败。
 * @note 调用前须完成 CRC 外设初始化；整轮计算期间须独占 CRC 外设。
 */
static bool App_Flash_Crc32_Calculate(const void *data, size_t data_size, uint32_t *out_crc32) {
    if((data == NULL) || (out_crc32 == NULL) || (data_size == 0U)) {
        return false;
    }

    // 4字节对齐-弱检查
    if (((data_size % sizeof(uint32_t)) != 0U)) {
        return false;
    }

    if (Drv_Crc32_Reset() == false) {
        return false;
    }

    if (Drv_Crc32_Update(data, data_size) == false) {
        return false;
    }

    *out_crc32 = Drv_Crc32_Get();
    return true;
}



/**
 * @brief 校验用户配置记录的格式、版本、有效长度和 CRC32。
 * @param record 已读取到 RAM 的完整用户配置记录。
 * @return true 表示校验通过，false 表示参数为空或记录无效。
 * @note 按新增字段追加到预留区、旧字段布局和含义保持不变的规则兼容旧版本。
 * @note 调用期间记录内容须保持不变，整轮计算须独占 CRC 外设。
 */
static bool App_Device_Config_Record_Is_Valid(const DeviceConfigRecord_t *record) {
    uint32_t crc32;
    uint32_t major_version;                     // 原始记录的主版本号
    uint32_t payload_size;                      // 原始记录的载荷容量，单位字节


    if (record == NULL) {
        return false;
    }

    // 初筛（版本=0 / 大于当前版本）
    if ((record->version == 0U) || (record->version > DEVICE_CONFIG_VER_CODE)) {
        return false;
    }

    // 根据原始主版本选择容量，避免使用新容量校验旧记录。
    major_version = record->version / 10000U;
    if (major_version == 1U) {
        payload_size = DEVICE_CONFIG_V1_PAYLOAD_SIZE_BYTES;
    } else if (major_version == 2U) {
        payload_size = DEVICE_CONFIG_V2_PAYLOAD_SIZE_BYTES;
    } else {
        return false;
    }

    if (payload_size > sizeof(record->payload)) {
        return false;
    }    

    // 检查有效数据长度
    // 有效长度不能超过历史载荷容量，也不能超过当前可识别的字段范围。
    if ((record->payload_valid_size == 0U) || (record->payload_valid_size > payload_size) || (record->payload_valid_size > DEVICE_CONFIG_VALID_SIZE_BYTES)) {
        return false;
    }


    // 当前版本的有效记录长度 必须等于 当前版本全部有效字段。
    if ((record->version == DEVICE_CONFIG_VER_CODE) && (record->payload_valid_size != DEVICE_CONFIG_VALID_SIZE_BYTES)) {
        return false;
    }

    // 校验完整载荷，包括预留区
    if (App_Flash_Crc32_Calculate(&(record->payload), payload_size, &crc32) == false) {
        return false;
    }

    return crc32 == record->crc32;
}



/**
 * @brief 设备配置参数合法性检查
 * @param config 
 * @return 
 */
static bool Device_Config_Params_Is_Valid(const Device_CFG_Params_t *config) {
    //[TODO]:这里应该做合法性检查，并且！把不合法的值全部改成默认值。
    return true;
}



/**
 * @brief 将准备好的当前版本用户配置记录写入 Flash。
 * @param record RAM 中的完整候选记录，版本、有效长度和 CRC32 须已填写，调用期间内容须保持不变。
 * @return 0 表示写入并回读校验成功；-3 表示空指针；-4 表示记录大小超出分区或不满足写入对齐；
 *         -5 表示记录非当前版本或格式、CRC32 校验失败；-6 表示擦除失败；-7 表示写入流程失败。
 * @note 调用前须完成相关初始化，并由调用者持有 Flash 写事务锁、保证校验期间独占 CRC 外设。
 * @note 本函数不获取互斥锁，不修改输入记录，也不更新当前配置缓存和就绪状态。
 */
static int32_t App_Device_Config_Record_Write(const DeviceConfigRecord_t *record) {
    Drv_Flash_Result_t result;              //Flash操作结果

    // 合法性检查
    if (record == NULL) {
        return -3;
    }

    // 擦除前检查记录容量和写入长度，避免结构体扩展后越过配置分区。
    if ((sizeof(*record) > DRV_FLASH_USERCFG_SIZE) || ((sizeof(*record) % DRV_FLASH_WRITE_ALIGN) != 0U)) {
        LOG_ERROR("用户配置记录大小超出分区或不满足写入对齐");
        return -4;
    }

    // 新写入的记录必须已经转换为当前版本，并通过格式及 CRC32 校验。
    if ((record->version != DEVICE_CONFIG_VER_CODE) || (App_Device_Config_Record_Is_Valid(record) == false)) {
        LOG_ERROR("用户配置记录未准备完成或校验失败");
        return -5;
    }

    result = Drv_Flash_Erase_Sector_For_Address(DRV_FLASH_USERCFG_START_ADDR, DRV_FLASH_USERCFG_END_ADDR);
    if (result != DRV_FLASH_OK) {
        LOG_ERROR("擦除用户配置失败，错误码=%d", (int)result);
        return -6;
    }

    // 驱动内部包含写入后的逐字节回读比较。
    result = Drv_Flash_Write(DRV_FLASH_USERCFG_START_ADDR, record, sizeof(*record));
    if (result != DRV_FLASH_OK) {
        LOG_ERROR("写入用户配置失败，错误码=%d", (int)result);
        return -7;
    }

    return 0;
}




/**
 * @brief 将候选配置整理为可写入 Flash 的当前版本记录。
 * @param record 待准备的 RAM 候选记录，调用前须填好全部有效业务字段；
 *               成功后更新版本、有效长度、预留区和 CRC32，保留有效业务字段。
 * @return 0 表示准备成功；-8 表示空指针；-9 表示业务参数非法；-10 表示 CRC32 计算失败。
 * @note 调用者须持有 Flash 写事务锁，并保证计算期间独占 CRC 外设。
 * @note 本函数仅准备候选记录，不操作 Flash，不更新当前配置缓存和就绪状态。
 */
static int32_t App_Device_Config_Record_Prepare(DeviceConfigRecord_t *record) {
    if (record == NULL) {
        return -8;
    }

    if (Device_Config_Params_Is_Valid(&record->payload) == false) {
        LOG_ERROR("用户配置业务参数非法");
        return -9;
    }

    // 统一预留区内容，避免无意义的残留字节参与 CRC32 计算。
    memset(record->payload.Offset, 0, sizeof(record->payload.Offset));

    record->version = DEVICE_CONFIG_VER_CODE;
    record->payload_valid_size = DEVICE_CONFIG_VALID_SIZE_BYTES;

    // CRC32 覆盖完整载荷，包括清零后的预留区。
    if (App_Flash_Crc32_Calculate(&record->payload, sizeof(record->payload), &record->crc32) == false) {
        LOG_ERROR("用户配置 CRC32 计算失败");
        return -10;
    }

    return 0;
}

































//---------------------------------------------------------公开API-------------------------------------------------------------



/**
 * @brief 初始化Flash服务。
 * @return 0 表示初始化成功或已完成初始化；-11 表示 Flash 互斥量创建失败。
 * @note 本函数不读取用户配置、下载信息或OTA信息，各运行模式按需调用对应Load接口。
 */
int32_t App_Flash_Init(void) {
    if (s_app_flash_initialized == true) {
        return 0;
    }

    if (s_app_flash_mutex == NULL) {
        s_app_flash_mutex = xSemaphoreCreateMutex();
        if (s_app_flash_mutex == NULL) {
            LOG_ERROR("Flash互斥量创建失败");
            return -11;
        }
    }

    s_app_flash_initialized = true;
    return 0;
}





/**
 * @brief 从 Flash 加载用户配置，按需恢复默认值或升级后发布到当前 RAM 缓存。
 * @return 0 表示加载成功，包括默认配置回填或旧版本升级成功；
 *         -12 表示服务未初始化；-13 表示获取互斥锁失败；-14 表示 Flash 读取失败；
 *         -15 表示记录准备失败；-16 表示记录写回失败；-17 表示已存储的业务参数非法。
 * @note 仅在调度器正常运行的任务上下文调用，适用于启动加载或受控重载。
 * @note V1 系列按 512 字节载荷校验，V2 系列按 1024 字节载荷校验。
 * @note 记录头与旧业务字段布局及含义保持不变，新字段仅追加在旧有效字段之后。
 * @note 校验期间须独占 CRC 外设；本函数持有 Flash 互斥锁，不调用公共 Save() 或 Reset()。
 * @note 流程失败时保留当前 RAM 配置和就绪状态，单扇区写回失败仍可能影响原 Flash 记录。
 */
int32_t App_Flash_DeviceConfig_Load(void) {
    Drv_Flash_Result_t flash_result;            // Flash 读取操作结果
    int32_t result;                             // 获取互斥锁、记录准备及写回操作结果
    uint32_t old_valid_size;                    // 旧版本有效字段长度，单位字节
    uint8_t *payload_bytes;                     // 候选载荷的字节访问指针，用于定位新增字段
    const uint8_t *default_bytes;               // 当前默认配置的只读字节访问指针
    bool need_write_back = false;               // 是否需要将默认配置或升级后的记录写回 Flash


    if (s_app_flash_initialized == false) {
        LOG_ERROR("Flash服务尚未初始化");
        return -12;
    }

    result = App_Flash_Lock();
    if (result != 0) {
        return -13;
    }

    // 先读取到候选缓存，避免未校验的数据覆盖当前有效配置。
    flash_result = Drv_Flash_Read(DRV_FLASH_USERCFG_START_ADDR, &s_device_config_candidate, sizeof(s_device_config_candidate));
    if (flash_result != DRV_FLASH_OK) {
        LOG_ERROR("读取用户配置失败，错误码=%d", (int)flash_result);
        App_Flash_Unlock();
        return -14;
    }


    // 必须先校验原始记录，再修改候选载荷，避免破坏旧记录的 CRC32 校验依据。
    if (App_Device_Config_Record_Is_Valid(&s_device_config_candidate) == true) {
        if (s_device_config_candidate.version < DEVICE_CONFIG_VER_CODE) {
            old_valid_size = s_device_config_candidate.payload_valid_size;

            LOG_INFO("检测到旧配置，准备升级，版本=%u -> %u，有效长度=%u -> %u bytes", (unsigned int)s_device_config_candidate.version, (unsigned int)DEVICE_CONFIG_VER_CODE, (unsigned int)old_valid_size, (unsigned int)DEVICE_CONFIG_VALID_SIZE_BYTES);

            // 保留旧有效字段，只将新增字段所在区间替换为当前默认值。
            if (old_valid_size < DEVICE_CONFIG_VALID_SIZE_BYTES) {
                payload_bytes = (uint8_t *)&s_device_config_candidate.payload;
                default_bytes = (const uint8_t *)&DEFAULT_DEVICE_CFG;

                (void)memcpy(payload_bytes + old_valid_size, default_bytes + old_valid_size, DEVICE_CONFIG_VALID_SIZE_BYTES - old_valid_size);
            }

            need_write_back = true;
        }
    } else {
        LOG_WARN("用户配置记录无效，准备回填默认配置");

        s_device_config_candidate.payload = DEFAULT_DEVICE_CFG;
        need_write_back = true;
    }


    if (need_write_back == true) {
        // 校验业务参数、清零剩余预留区，并更新版本、有效长度和完整载荷 CRC32。
        result = App_Device_Config_Record_Prepare(&s_device_config_candidate);
        if (result != 0) {
            LOG_ERROR("准备用户配置记录失败，错误码=%d", (int)result);
            App_Flash_Unlock();
            return -15;
        }

        result = App_Device_Config_Record_Write(&s_device_config_candidate);
        if (result != 0) {
            LOG_ERROR("写回用户配置记录失败，错误码=%d", (int)result);
            App_Flash_Unlock();
            return -16;
        }
    } else {
        // 当前版本无需写回，但仍须检查业务参数。
        if (Device_Config_Params_Is_Valid(&s_device_config_candidate.payload) == false) {
            LOG_ERROR("已存储的用户配置业务参数非法");
            App_Flash_Unlock();
            return -17;
        }
    }

    // 记录验证完成，且需要的写回操作成功后，才发布到当前缓存。
    s_device_config_record = s_device_config_candidate;
    s_user_config_ready = true;

    App_Flash_Unlock();

    LOG_INFO("用户配置加载完成，版本=%u，有效长度=%u bytes", (unsigned int)DEVICE_CONFIG_VER_CODE, (unsigned int)DEVICE_CONFIG_VALID_SIZE_BYTES);
    return 0;
}





























/**
 * @brief 同步保存完整用户配置，写入成功后更新当前 RAM 缓存。
 * @param config 待保存的完整业务配置，调用期间须保持有效且不被其他上下文修改；
 *               仅复制有效业务字段，预留区由模块统一清零。
 * @return 0 表示保存成功；-18 表示空指针；-19 表示服务未初始化；
 *         -20 表示获取互斥锁失败；-21 表示记录准备失败；-22 表示记录写入失败。
 * @note 仅在调度器正常运行的任务上下文调用，须先完成 App_Flash_Init()。
 * @note 本接口执行全量配置替换；失败时保留原有当前 RAM 缓存和就绪状态。
 */
int32_t App_Flash_DeviceConfig_Save(const Device_CFG_Params_t *config) {
    int32_t result;

    if (config == NULL) {
        return -18;
    }


    if (s_app_flash_initialized == false) {
        LOG_ERROR("Flash服务尚未初始化");
        return -19;
    }

    result = App_Flash_Lock();
    if (result != 0) {
        return -20;
    }

    // 只复制有效业务字段，记录头和预留区由 Prepare() 统一处理。
    (void)memcpy(&s_device_config_candidate.payload, config, DEVICE_CONFIG_VALID_SIZE_BYTES);


    result = App_Device_Config_Record_Prepare(&s_device_config_candidate);
    if (result != 0) {
        LOG_ERROR("准备用户配置记录失败，错误码=%d", (int)result);
        App_Flash_Unlock();
        return -21;
    }

    result = App_Device_Config_Record_Write(&s_device_config_candidate);
    if (result != 0) {
        LOG_ERROR("保存用户配置记录失败，错误码=%d", (int)result);
        App_Flash_Unlock();
        return -22;
    }

    // Flash 写入和回读校验成功后，才发布新的当前配置。
    s_device_config_record = s_device_config_candidate;
    s_user_config_ready = true;

    App_Flash_Unlock();
    return 0;
}


/**
 * @brief 获取当前用户配置的独立 RAM 快照。
 * @param config 配置快照的输出地址，成功时写入完整配置，失败时保持原内容。
 * @return 0 表示获取成功；-23 表示空指针；-24 表示服务未初始化；
 *         -25 表示获取互斥锁失败；-26 表示当前配置尚未就绪。
 * @note 仅在调度器正常运行的任务上下文调用。
 * @note 配置须先成功加载或保存；获取后修改快照不会直接改变模块内部配置。
 */
int32_t App_Flash_DeviceConfig_Get(Device_CFG_Params_t *config) {
    int32_t result;

    if (config == NULL) {
        return -23;
    }

    if (s_app_flash_initialized == false) {
        return -24;
    }

    result = App_Flash_Lock();
    if (result != 0) {
        return -25;
    }

    // 就绪状态和当前配置必须在同一把锁的保护下读取。
    if (s_user_config_ready == false) {
        App_Flash_Unlock();
        return -26;
    }

    *config = s_device_config_record.payload;

    App_Flash_Unlock();
    return 0;
}










/**
 * @brief 恢复默认用户配置，并同步保存到 Flash。
 * @return 0 表示恢复默认并保存成功；错误码透传 Save()：-19 表示服务未初始化；
 *         -20 表示获取互斥锁失败；-21 表示记录准备失败；-22 表示记录写入失败。
 * @note 仅在调度器正常运行的任务上下文调用，失败时保留原有当前 RAM 缓存和就绪状态。
 * @note 本函数通过 Save() 获取互斥锁，调用前不得已持有本模块的 Flash 互斥锁。
 */
int32_t App_Flash_DeviceConfig_Reset(void) {
    return App_Flash_DeviceConfig_Save(&DEFAULT_DEVICE_CFG);
}


/**
 * @brief 获取当前用户配置快照，并按功能分组打印业务字段及配置容量。
 * @note 配置须已成功加载或保存，获取失败时只打印错误信息。
 */
void App_Flash_DeviceConfig_Prf(void) {
    Device_CFG_Params_t config;                 // 当前配置的独立 RAM 快照

    if (App_Flash_DeviceConfig_Get(&config) != 0) {
        LOG_ERROR("获取用户配置失败，无法打印");
        return;
    }

    LOG_INFO("========== 设备配置 ==========");
    LOG_INFO("设备业务模式: 当前 = %u", (unsigned int)config.Device_Business_Mode);
    LOG_INFO("系统开关: 硬=%u, 软=%u, 静默=%u", (unsigned int)config.Device_Hard_SW, (unsigned int)config.Device_Soft_SW, (unsigned int)config.Device_Silent_SW);

    LOG_INFO("夜间静音: 开关=%u, 时段=%02u:%02u-%02u:%02u", (unsigned int)config.Device_Night_Mute_SW, (unsigned int)config.Device_Night_Mute_Start_Hour, (unsigned int)config.Device_Night_Mute_Start_Minute, (unsigned int)config.Device_Night_Mute_End_Hour, (unsigned int)config.Device_Night_Mute_End_Minute);
    LOG_INFO("时间段静音: 总开关=%u", (unsigned int)config.Device_Fragment_Mute_SW);
    LOG_INFO("时间段 1: 开关=%u, 时段=%02u:%02u-%02u:%02u", (unsigned int)config.Device_Fragment_Mute_1_SW, (unsigned int)config.Device_Fragment_Mute_1_Start_Hour, (unsigned int)config.Device_Fragment_Mute_1_Start_Minute, (unsigned int)config.Device_Fragment_Mute_1_End_Hour, (unsigned int)config.Device_Fragment_Mute_1_End_Minute);
    LOG_INFO("时间段 2: 开关=%u, 时段=%02u:%02u-%02u:%02u", (unsigned int)config.Device_Fragment_Mute_2_SW, (unsigned int)config.Device_Fragment_Mute_2_Start_Hour, (unsigned int)config.Device_Fragment_Mute_2_Start_Minute, (unsigned int)config.Device_Fragment_Mute_2_End_Hour, (unsigned int)config.Device_Fragment_Mute_2_End_Minute);
    LOG_INFO("时间段 3: 开关=%u, 时段=%02u:%02u-%02u:%02u", (unsigned int)config.Device_Fragment_Mute_3_SW, (unsigned int)config.Device_Fragment_Mute_3_Start_Hour, (unsigned int)config.Device_Fragment_Mute_3_Start_Minute, (unsigned int)config.Device_Fragment_Mute_3_End_Hour, (unsigned int)config.Device_Fragment_Mute_3_End_Minute);

    LOG_INFO("超声波: 开关=%u, 模式=%u, 频率=%u-%u Hz", (unsigned int)config.Device_Ultrasonic_SW, (unsigned int)config.Device_Ultrasonic_Mode, (unsigned int)config.Device_Ultrasonic_Start_Hz, (unsigned int)config.Device_Ultrasonic_End_Hz);
    LOG_INFO("超声波定时: 开关=%u, 跨度=%u min", (unsigned int)config.Device_Ultrasonic_Schedule_SW, (unsigned int)config.Device_Ultrasonic_Schedule_Span);

    LOG_INFO("模拟声: 开关=%u, 模式=%u, 最多选取数量=%u", (unsigned int)config.Device_Simulated_SW, (unsigned int)config.Device_Simulated_Mode, (unsigned int)config.Device_Simulated_Select_Number_Max);
    LOG_INFO("模拟声定时: 开关=%u, 跨度=%u min", (unsigned int)config.Device_Simulated_Schedule_SW, (unsigned int)config.Device_Simulated_Schedule_Span);

    LOG_INFO("红蓝光: 开关=%u, 模式=%u, 频率=%u Hz", (unsigned int)config.Device_R_B_LED_SW, (unsigned int)config.Device_R_B_LED_Mode, (unsigned int)config.Device_R_B_LED_Frequency);
    LOG_INFO("红蓝光定时: 开关=%u, 跨度=%u min", (unsigned int)config.Device_R_B_LED_Schedule_SW, (unsigned int)config.Device_R_B_LED_Schedule_Span);

    LOG_INFO("激光: 开关=%u, 模式=%u, 频率=%u Hz", (unsigned int)config.Device_Laser_SW, (unsigned int)config.Device_Laser_Mode, (unsigned int)config.Device_Laser_Frequency);
    LOG_INFO("激光定时: 开关=%u, 跨度=%u min", (unsigned int)config.Device_Laser_Schedule_SW, (unsigned int)config.Device_Laser_Schedule_Span);

    LOG_INFO("软定时: 开关=%u, 跨度=%u min", (unsigned int)config.Device_Soft_Schedule_SW, (unsigned int)config.Device_Soft_Schedule_Span);
    LOG_INFO("硬定时: 模式=%u, 跨度=%u min", (unsigned int)config.Device_Hard_Schedule_Mode, (unsigned int)config.Device_Hard_Schedule_Span);
    LOG_INFO("音量: 逻辑等级=%u, 物理值=%u", (unsigned int)config.Device_Logic_Volume, (unsigned int)config.Device_Real_Volume);

    LOG_INFO("冷却: 模式=%u, 初始时间=%u min, 递增时间=%u min", (unsigned int)config.Device_Cooling_Mode, (unsigned int)config.Device_Cooling_Initial_Duration, (unsigned int)config.Device_Cooling_Increase_Time);
    LOG_INFO("定位: 总开关=%u, 定位播报开关=%u, 上报开关=%u, 上报播报开关=%u, 间隔=%u h %u min,", (unsigned int)config.Device_Location_SW, (unsigned int)config.Device_Location_Result_Broadcast_SW, (unsigned int)config.Device_Location_Report_SW, (unsigned int)config.Device_Location_Report_Broadcast_SW, (unsigned int)config.Device_Location_Frequency_Hour, (unsigned int)config.Device_Location_Frequency_Minute);
    LOG_INFO("定位方式开关: 北斗=%u, Wi-Fi=%u, 基站=%u", (unsigned int)config.Device_Location_BDS_SW, (unsigned int)config.Device_Location_WIFI_SW, (unsigned int)config.Device_Location_LBS_SW);

    // 限定 ID 字符数，未保存字符串结束符时也不会越界读取。
    LOG_INFO("遥控器 ID: %.*s", (int)(sizeof(config.Device_R5a_Id) - 1U), (const char *)config.Device_R5a_Id);
    LOG_INFO("配置容量: 总计=%u bytes, 有效=%u bytes, 预留=%u bytes", (unsigned int)sizeof(config), (unsigned int)DEVICE_CONFIG_VALID_SIZE_BYTES, (unsigned int)sizeof(config.Offset));
    LOG_INFO("==============================");
}













