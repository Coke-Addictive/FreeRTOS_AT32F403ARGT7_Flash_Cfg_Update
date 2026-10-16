/**
 * @file drv_flash.h
 * @brief AT32F403ARGT7 内部 Flash 驱动接口与分区定义。
 * @author Treo
 * @date 2026-08-15
 * @version 1.0.1
 * @note Software designed and implemented by Treo.
 * @note 当前接口推荐全部用宏来传递实参！！！暂不支持任意地址范围，且Flash操作结果没用根据底层库细分到具体
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * AT32F403ARGT7 内部 Flash 划分（按 AT32F403A/407/407A 手册）：
 * 
 * 主存储器（1MB = 1024KB）：
 *  - 共 2 个 Bank，每个 Bank 512KB
 *  - 每个 Bank 256 个扇区，每扇区 2KB
 *
 * Bank1（sector 0 ~ 255）：
 *  - sector 0   : 0x08000000 - 0x080007FF
 *  - sector 1   : 0x08000800 - 0x08000FFF
 *  - ...
 *  - sector 255 : 0x0807F800 - 0x0807FFFF
 *
 * Bank2（sector 256 ~ 511）：
 *  - sector 256 : 0x08080000 - 0x080807FF
 *  - sector 257 : 0x08080800 - 0x08080FFF
 *  - ...
 *  - sector 511 : 0x080FF800 - 0x080FFFFF
 *
 * 本项目 Flash 分区规划（总计 1024KB）：BootLoader + APP + Download + 预留 + 运行模式 + 运行日志 + 更新信息保存 + OTA标志 + 用户配置信息
 * - BootLoader     :  48KB  (0x08000000 - 0x0800BFFF)  扇区：sector 0   ~ 23   大小:0xC000
 * - APP            : 472KB  (0x0800C000 - 0x08081FFF)  扇区：sector 24  ~ 259  大小:0x76000
 * - Download       : 472KB  (0x08082000 - 0x080F7FFF)  扇区：sector 260 ~ 495  大小:0x76000
 *
 * - Reserved       :  18KB  (0x080F8000 - 0x080FC7FF)  扇区：sector 496 ~ 504  大小:0x4800
 *
 * - SelfCheck      :   2KB  (0x080FC800 - 0x080FCFFF)  扇区：sector 505        大小:0x0800
 * - Running Log    :   4KB  (0x080FD000 - 0x080FDFFF)  扇区：sector 506 ~ 507  大小:0x1000
 * - UpDate Info    :   4KB  (0x080FE000 - 0x080FEFFF)  扇区：sector 508 ~ 509  大小:0x1000
 * - OTA            :   2KB  (0x080FF000 - 0x080FF7FF)  扇区：sector 510        大小:0x0800
 * - UserCfg        :   2KB  (0x080FF800 - 0x080FFFFF)  扇区：sector 511        大小:0x0800
 */
#ifndef DRV_FLASH_H_
#define DRV_FLASH_H_




#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



/* Flash 基础参数 */
#define DRV_FLASH_START_ADDR                 0x08000000U          // 内部 Flash 起始地址
#define DRV_FLASH_END_ADDR                   0x080FFFFFU          // 内部 Flash 结束地址
#define DRV_FLASH_TOTAL_SIZE                 (1024U * 1024U)      // 内部 Flash 总容量：1MB
#define DRV_FLASH_SECTOR_SIZE                2048U                // 内部 Flash 扇区大小：2KB
#define DRV_FLASH_SECTOR_COUNT               512U                 // 内部 Flash 总扇区数


#define DRV_FLASH_BANK_SIZE                  (512U * 1024U)       // 单个 Bank 容量：512KB
#define DRV_FLASH_BANK_SECTOR_COUNT          256U                 // 单个 Bank 扇区数


#define DRV_FLASH_WRITE_ALIGN                4U                   // 按 32 位 word 写入
#define DRV_FLASH_EMPTY_WORD                 0xFFFFFFFFU          // Flash 擦除后的默认 word
#define DRV_FLASH_EMPTY_BYTE                 0xFFU                // Flash 擦除后的默认 byte
#define DRV_FLASH_INVALID_SECTOR             0xFFFFFFFFU          // 无效扇区编号

/* BootLoader 区域：48KB，sector 0 ~ 23 */
#define DRV_FLASH_BL_START_ADDR              0x08000000U          // BootLoader 起始地址
#define DRV_FLASH_BL_END_ADDR                0x0800BFFFU          // BootLoader 结束地址
#define DRV_FLASH_BL_SIZE                    (48U * 1024U)        // BootLoader 大小
#define DRV_FLASH_BL_START_SECTOR            0U                   // BootLoader 起始扇区
#define DRV_FLASH_BL_END_SECTOR              23U                  // BootLoader 结束扇区
#define DRV_FLASH_BL_SECTOR_COUNT            24U                  // BootLoader 扇区数


/* APP 区域：472KB，sector 24 ~ 259 */
#define DRV_FLASH_APP_START_ADDR             0x0800C000U          // APP 起始地址
#define DRV_FLASH_APP_END_ADDR               0x08081FFFU          // APP 结束地址
#define DRV_FLASH_APP_SIZE                   (472U * 1024U)       // APP 大小
#define DRV_FLASH_APP_START_SECTOR           24U                  // APP 起始扇区
#define DRV_FLASH_APP_END_SECTOR             259U                 // APP 结束扇区
#define DRV_FLASH_APP_SECTOR_COUNT           236U                 // APP 扇区数


/* Download 区域：472KB，sector 260 ~ 495 */
#define DRV_FLASH_DOWNLOAD_START_ADDR        0x08082000U          // Download 起始地址
#define DRV_FLASH_DOWNLOAD_END_ADDR          0x080F7FFFU          // Download 结束地址
#define DRV_FLASH_DOWNLOAD_SIZE              (472U * 1024U)       // Download 大小
#define DRV_FLASH_DOWNLOAD_START_SECTOR      260U                 // Download 起始扇区
#define DRV_FLASH_DOWNLOAD_END_SECTOR        495U                 // Download 结束扇区
#define DRV_FLASH_DOWNLOAD_SECTOR_COUNT      236U                 // Download 扇区数

/* Reserved 区域：18KB，sector 496 ~ 504 */
#define DRV_FLASH_RESERVED_START_ADDR        0x080F8000U          // Reserved 起始地址
#define DRV_FLASH_RESERVED_END_ADDR          0x080FC7FFU          // Reserved 结束地址
#define DRV_FLASH_RESERVED_SIZE              (18U * 1024U)        // Reserved 大小
#define DRV_FLASH_RESERVED_START_SECTOR      496U                 // Reserved 起始扇区
#define DRV_FLASH_RESERVED_END_SECTOR        504U                 // Reserved 结束扇区
#define DRV_FLASH_RESERVED_SECTOR_COUNT      9U                   // Reserved 扇区数

/* Mode 区域：2KB，sector 505 */
#define DRV_FLASH_MODE_START_ADDR            0x080FC800U          // Mode 起始地址
#define DRV_FLASH_MODE_END_ADDR              0x080FCFFFU          // Mode 结束地址
#define DRV_FLASH_MODE_SIZE                  (2U * 1024U)         // Mode 大小
#define DRV_FLASH_MODE_START_SECTOR          505U                 // Mode 起始扇区
#define DRV_FLASH_MODE_END_SECTOR            505U                 // Mode 结束扇区
#define DRV_FLASH_MODE_SECTOR_COUNT          1U                   // Mode 扇区数


/* Running Log 区域：4KB，sector 506 ~ 507 */
#define DRV_FLASH_LOG_START_ADDR             0x080FD000U          // Running Log 起始地址
#define DRV_FLASH_LOG_END_ADDR               0x080FDFFFU          // Running Log 结束地址
#define DRV_FLASH_LOG_SIZE                   (4U * 1024U)         // Running Log 大小
#define DRV_FLASH_LOG_START_SECTOR           506U                 // Running Log 起始扇区
#define DRV_FLASH_LOG_END_SECTOR             507U                 // Running Log 结束扇区
#define DRV_FLASH_LOG_SECTOR_COUNT           2U                   // Running Log 扇区数


/* Update Info 区域：4KB，sector 508 ~ 509 */
#define DRV_FLASH_UPDATE_INFO_START_ADDR     0x080FE000U          // Update Info 起始地址
#define DRV_FLASH_UPDATE_INFO_END_ADDR       0x080FEFFFU          // Update Info 结束地址
#define DRV_FLASH_UPDATE_INFO_SIZE           (4U * 1024U)         // Update Info 大小
#define DRV_FLASH_UPDATE_INFO_START_SECTOR   508U                 // Update Info 起始扇区
#define DRV_FLASH_UPDATE_INFO_END_SECTOR     509U                 // Update Info 结束扇区
#define DRV_FLASH_UPDATE_INFO_SECTOR_COUNT   2U                   // Update Info 扇区数

/* OTA 区域：2KB，sector 510 */
#define DRV_FLASH_OTA_START_ADDR             0x080FF000U          // OTA 起始地址
#define DRV_FLASH_OTA_END_ADDR               0x080FF7FFU          // OTA 结束地址
#define DRV_FLASH_OTA_SIZE                   (2U * 1024U)         // OTA 大小
#define DRV_FLASH_OTA_START_SECTOR           510U                 // OTA 起始扇区
#define DRV_FLASH_OTA_END_SECTOR             510U                 // OTA 结束扇区
#define DRV_FLASH_OTA_SECTOR_COUNT           1U                   // OTA 扇区数


/* User Config 区域：2KB，sector 511 */
#define DRV_FLASH_USERCFG_START_ADDR         0x080FF800U          // UserCfg 起始地址
#define DRV_FLASH_USERCFG_END_ADDR           0x080FFFFFU          // UserCfg 结束地址
#define DRV_FLASH_USERCFG_SIZE               (2U * 1024U)         // UserCfg 大小
#define DRV_FLASH_USERCFG_START_SECTOR       511U                 // UserCfg 起始扇区
#define DRV_FLASH_USERCFG_END_SECTOR         511U                 // UserCfg 结束扇区
#define DRV_FLASH_USERCFG_SECTOR_COUNT       1U                   // UserCfg 扇区数




/* BootLoader 写擦保护范围 */
#define DRV_FLASH_PROTECT_START_ADDR         DRV_FLASH_BL_START_ADDR
#define DRV_FLASH_PROTECT_END_ADDR           DRV_FLASH_BL_END_ADDR





typedef enum {
    DRV_FLASH_OK = 0,              // 执行成功
    DRV_FLASH_FAIL,                // 执行失败
    DRV_FLASH_ERR_PARAM,           // 参数错误
    DRV_FLASH_ERR_UNERASED,        // Flash 未擦除
    DRV_FLASH_ERR_ADDR,            // 地址不合法
    DRV_FLASH_ERR_ALIGN,           // 地址或长度未对齐
    DRV_FLASH_ERR_PROTECT,         // 命中保护区域
    DRV_FLASH_ERR_BUSY,            // Flash 正忙
    DRV_FLASH_ERR_UNLOCK,          // 解锁失败
    DRV_FLASH_ERR_LOCK,            // 加锁失败
    DRV_FLASH_ERR_ERASE,           // 擦除失败
    DRV_FLASH_ERR_WRITE,           // 写入失败
    DRV_FLASH_ERR_VERIFY,          // 校验失败
    DRV_FLASH_ERR_TIMEOUT,         // 操作超时
} Drv_Flash_Result_t;





Drv_Flash_Result_t Drv_Flash_Erase_Sector_For_Number(uint32_t start_sector, uint32_t end_sector , uint32_t sector_count); // 按编号擦除扇区
Drv_Flash_Result_t Drv_Flash_Erase_Sector_For_Address(uint32_t start_address, uint32_t end_address);    // 按地址擦除扇区

Drv_Flash_Result_t Drv_Flash_Read(uint32_t start_address, void *data, size_t data_size);                // 读取数据
Drv_Flash_Result_t Drv_Flash_Write(uint32_t address, const void *data, size_t data_size);               // 写入数据(按字写入)


Drv_Flash_Result_t Drv_Flash_Write_Padded(uint32_t address, const void *data, size_t data_size);

bool Drv_Flash_Is_Erased(uint32_t start_address, size_t data_size);                                     // 检查指定地址范围是否已擦除

#endif /* DRV_FLASH_H_ */










/*
 @changelog - 2026-08-15 V1.0.0 Treo
- 初始版本。
- 完成 AT32F403ARGT7 内部 Flash 分区定义。
- 支持按扇区编号擦除和按地址擦除。
- 支持 Flash 数据读取和 32 位 Word 写入。
- 支持非 4 字节长度数据补齐写入。
- BootLoader 区域写擦保护。
- Flash 擦除状态检查。
 --------------------------------------------------------------------------
 @changelog - 2026-08-18 V1.0.1 Treo
- 给所有函数名风格统一，一个单次一个下划线隔开
- Drv_Flash_Erase_Sector函数内部增加了核对地址是否对齐扇区
- 给Drv_Flash_Erase_Sector_For_Number函数内部，换算得到扇区地址后增加了二次校验地址合法性
- 发现了Drv_Flash_Erase_Sector_For_Address 公开接口不是地址自适应的问题，目前是严格检查扇区边界，已经做了TODO标记










*/



 

 




