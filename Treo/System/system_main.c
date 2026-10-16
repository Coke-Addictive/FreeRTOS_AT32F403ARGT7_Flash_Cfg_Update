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
    int32_t result;
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
    
    while (1) {
        LOG("Main Task Running...");
        vTaskDelay(pdMS_TO_TICKS(30000U));
    }
}
