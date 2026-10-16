#ifndef DRV_CRC32_H_
#define DRV_CRC32_H_

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"

// 初始化；调用前须由 wk_periph_clock_config() 开启 CRC 时钟。
void Drv_Crc32_Init(void);



// 分段累积；同一轮计算期间不得穿插其他业务或中断中的 CRC 计算。
bool Drv_Crc32_Reset(void);
// 长度单位字节，须大于 0 且为 4 的倍数；缓冲区地址不要求对齐，失败时不改变 CRC。
bool Drv_Crc32_Update(const void *data, size_t data_size);
// 读取当前累计结果，不复位、不做末尾异或；未初始化时返回 0U。
uint32_t Drv_Crc32_Get(void);







// 独立计算，每次自动复位；length 为 32 位字的个数，pbuffer 须按 4 字节对齐。
uint32_t Drv_Crc32_Calculate(uint32_t *pbuffer, uint32_t length);


#endif // DRV_CRC32_H_


