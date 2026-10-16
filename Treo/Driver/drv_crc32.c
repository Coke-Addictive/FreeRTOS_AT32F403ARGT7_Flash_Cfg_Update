#include "drv_crc32.h"

#include "wk_crc.h"
#include "stdbool.h"
#include "string.h"



void Drv_Crc32_Init(void) {
    //main.c中已经启动了
}


/**
 * @brief 将 CRC 累计结果复位为 Workbench 配置的初始值，开始一轮独立计算。
 * @return true 表示复位成功，false 表示驱动尚未初始化。
 */
bool Drv_Crc32_Reset(void) {

    crc_data_reset();

    return true;
}


/**
 * @brief 按 32 位字顺序追加数据，保留之前的 CRC 累计结果。
 * @param data 待追加数据的只读缓冲区，地址不要求按 4 字节对齐，调用期间须保持有效。
 * @param data_size 数据长度，单位字节，须大于 0 且为 4 的倍数，不自动补齐。
 * @return true 表示追加成功，false 表示未初始化、空指针或长度非法，失败时不改变 CRC。
 */
bool Drv_Crc32_Update(const void *data, size_t data_size) {
    const uint8_t *bytes;
    size_t offset;
    uint32_t word;


    if ((data == NULL) || (data_size == 0U) || ((data_size % sizeof(word)) != 0U)) {
        return false;
    }

    bytes = (const uint8_t *)data;

    // 用 memcpy 取出每个字，避免未对齐访问和将字节数组强转为 uint32_t 数组。
    for (offset = 0U; offset < data_size; offset += sizeof(word)) {
        (void)memcpy(&word, &bytes[offset], sizeof(word));
        (void)crc_one_word_calculate(word);
    }

    return true;
}


/**
 * @brief 读取当前硬件 CRC 累计结果，不复位且不执行末尾异或。
 * @return 已初始化时返回当前 CRC 值，未初始化时返回 0U；合法 CRC 也可能为 0U。
 */
uint32_t Drv_Crc32_Get(void) {

    return crc_data_get();
}


uint32_t Drv_Crc32_Calculate(uint32_t *pbuffer, uint32_t length) {
    uint32_t crc_value = 0;


    // 空检查
    if ((pbuffer == NULL) || (length == 0U)) {
        return 0U;
    }

    // 重置
    crc_data_reset();

    // 计算
    crc_value = crc_block_calculate(pbuffer, length);

    return crc_value;
}






