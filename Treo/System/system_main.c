#include "system_main.h"

// FreeRTOS资源
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"


#include "app_flash_simple.h"

#define LOG_TAG "SYS"
#include "log.h"

static TaskHandle_t s_system_main_task_handle = NULL;

// 函数声明
static void System_Main_Task(void *parameter);

/**
 * @brief 创建System Main任务和业务事件队列。
 */
int32_t System_Main_Init(void) {
    BaseType_t result;
    result = xTaskCreate(System_Main_Task,
                         "主任务",
                         512,
                         NULL,
                         10,
                         &s_system_main_task_handle);

    if (result != pdPASS) {
        s_system_main_task_handle = NULL;
        return -1;
    }

    return 0;
}

static void System_Main_Task(void *parameter) {
    Device_CFG_Params_t config;                 // 当前配置的独立 RAM 快照
    int32_t result;                             // 获取配置的操作结果
    uint32_t location_interval_minutes;         // 换算后的定位间隔，单位分钟

    (void)parameter;

    LOG("主任务启动成功");
    result = App_Flash_Init();


    if (result == 0) {
        result = App_Flash_DeviceConfig_Load();
    }

    if (result == 0) {
        App_Flash_DeviceConfig_Prf();
    } else {
        LOG_ERROR("配置服务启动失败，错误码=%d", (int)result);
    }

    LOG("");
    LOG("测试开始");
    result = App_Flash_DeviceConfig_Get(&config);
    if (result != 0) {
        LOG_ERROR("读取配置失败，错误码=%d", (int)result);
        return ;
    }

    // 使用开关决定业务分支，这里通过日志演示。
    if (config.Device_Soft_SW == 1U) {
        LOG_INFO("软开关开启，允许执行业务");
        config.Device_Soft_SW = 0U;
    } else {
        LOG_INFO("软开关关闭，业务保持停止");
        config.Device_Soft_SW = 1U;
    }

    location_interval_minutes = (uint32_t)config.Device_Location_Frequency_Hour * 60U + (uint32_t)config.Device_Location_Frequency_Minute;
    // 使用配置中的小时、分钟，换算业务需要的总分钟数。
    LOG_INFO("定位间隔=%u min", (unsigned int)location_interval_minutes);
    LOG_INFO("夜间静音开关=%u", (unsigned int)config.Device_Night_Mute_SW);
    LOG_INFO("静音开始=%02u:%02u", (unsigned int)config.Device_Night_Mute_Start_Hour, (unsigned int)config.Device_Night_Mute_Start_Minute);
    LOG_INFO("静音结束=%02u:%02u", (unsigned int)config.Device_Night_Mute_End_Hour, (unsigned int)config.Device_Night_Mute_End_Minute);

    LOG("");
    LOG("修改测试开始");


    config.Device_Night_Mute_SW = 1U;                    // 开启夜间静音
    config.Device_Night_Mute_Start_Hour = 22U;           // 静音开始小时
    config.Device_Night_Mute_Start_Minute = 30U;         // 静音开始分钟
    config.Device_Night_Mute_End_Hour = 7U;              // 静音结束小时
    config.Device_Night_Mute_End_Minute = 0U;            // 静音结束分钟

    config.Device_Location_Frequency_Hour = 1U;          // 定位间隔的小时部分
    config.Device_Location_Frequency_Minute = 30U;       // 定位间隔的分钟部分

    result = App_Flash_DeviceConfig_Save(&config);

    if (result != 0) {
        LOG_ERROR("保存配置失败，错误码=%d", (int)result);
        return ;
    }

    LOG_INFO("修改后的配置已保存");

    App_Flash_DeviceConfig_Prf();

    while (1) {
        LOG("Main Task Running...");
        vTaskDelay(pdMS_TO_TICKS(30000U));
    }
}
