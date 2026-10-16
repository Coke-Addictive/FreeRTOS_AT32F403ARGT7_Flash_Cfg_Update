#include "drv_flash.h"

#include "at32f403a_407_flash.h"

#include <string.h>


//--------------------------------------函数声明---------------------------------------
static Drv_Flash_Result_t Drv_Flash_Unlock(void);
static Drv_Flash_Result_t Drv_Flash_Lock(void);
static void Drv_Flash_Clear_All_Flags(void);
static uint32_t Drv_Flash_Get_Sector_Start_Address(uint32_t sector);
static uint32_t Drv_Flash_Get_Sector_By_Address(uint32_t addr);
static bool Drv_Flash_Address_Range_Valid(uint32_t start_address, uint32_t end_address);
static bool Drv_Flash_Sector_Range_Valid(uint32_t start_sector, uint32_t end_sector);
static Drv_Flash_Result_t Drv_Flash_Erase_Sector(uint32_t start_sector_address, uint32_t end_sector_address , uint32_t sector_count);
static Drv_Flash_Result_t Drv_Flash_Write_Words(uint32_t start_address, const void *data, size_t data_size);





//---------------------------------系统内部加锁/解锁/清标记函数---------------------------------
/**
 * @brief 解锁内部 Flash Bank1/Bank2 写擦控制器。
 * @return Flash 操作结果。
 */
static Drv_Flash_Result_t Drv_Flash_Unlock(void) {
    flash_unlock();

    // OPLK 寄存器标志位表示控制器状态：1 = 锁定, 0 = 解锁.这个if是检查2个扇区的解锁是否成功的，也可以屏蔽
    if ((FLASH->ctrl_bit.oplk != 0U) || (FLASH->ctrl2_bit.oplk != 0U)) {
        return DRV_FLASH_ERR_UNLOCK;
    }

    return DRV_FLASH_OK;
}

/**
 * @brief 锁定内部 Flash Bank1/Bank2 写擦控制器。
 * @return Flash 操作结果。
 */
static Drv_Flash_Result_t Drv_Flash_Lock(void) {
    flash_lock();

    if ((FLASH->ctrl_bit.oplk == 0U) || (FLASH->ctrl2_bit.oplk == 0U)) {
        return DRV_FLASH_ERR_LOCK;
    }

    return DRV_FLASH_OK;
}


/**
 * @brief 清除内部 Flash Bank1/Bank2 操作完成和错误标志。
 */
static void Drv_Flash_Clear_All_Flags(void) {
    flash_flag_clear(FLASH_BANK1_ODF_FLAG | FLASH_BANK1_PRGMERR_FLAG | FLASH_BANK1_EPPERR_FLAG);
    flash_flag_clear(FLASH_BANK2_ODF_FLAG | FLASH_BANK2_PRGMERR_FLAG | FLASH_BANK2_EPPERR_FLAG);
}












//---------------------------------辅助工具函数---------------------------------
/**
 * @brief 根据扇区编号获取扇区起始地址。
 * @param sector 扇区编号。
 * @return 扇区起始地址，扇区非法时返回 0。
 */
static uint32_t Drv_Flash_Get_Sector_Start_Address(uint32_t sector) {
    if (sector >= DRV_FLASH_SECTOR_COUNT) {
        return 0U;
    }
    return DRV_FLASH_START_ADDR + (sector * DRV_FLASH_SECTOR_SIZE);
}

/**
 * @brief 根据内部 Flash 地址获取扇区编号。
 * @param addr 内部 Flash 地址。
 * @return 扇区编号，地址非法时返回 DRV_FLASH_INVALID_SECTOR。
 */
static uint32_t Drv_Flash_Get_Sector_By_Address(uint32_t addr) {
    if ((addr < DRV_FLASH_START_ADDR) || (addr > DRV_FLASH_END_ADDR)) {
        return DRV_FLASH_INVALID_SECTOR;
    }
    return (addr - DRV_FLASH_START_ADDR) / DRV_FLASH_SECTOR_SIZE;
}



/**
 * @brief 检查指定地址范围是否合法。
 * @param start_address 起始地址
 * @param end_address 结束地址
 * @return 地址范围是否合法
 */
static bool Drv_Flash_Address_Range_Valid(uint32_t start_address, uint32_t end_address){
    if ((start_address < DRV_FLASH_APP_START_ADDR) || (end_address > DRV_FLASH_END_ADDR) || (end_address < start_address)) {
        return false;
    }
    return true;
}


/**
 * @brief 检查指定扇区范围是否合法
 * @param start_sector 起始扇区·
 * @param end_sector 结束扇区
 * @return 扇区范围是否合法
 */
static bool Drv_Flash_Sector_Range_Valid(uint32_t start_sector, uint32_t end_sector) {
    if ((start_sector < DRV_FLASH_APP_START_SECTOR) || (end_sector > DRV_FLASH_USERCFG_END_SECTOR) || (end_sector < start_sector)) {
        return false;
    }
    return true;
}











//---------------------------------重要的Flash操作函数---------------------------------
/**
 * @brief 正真的擦除函数-按 "扇区" 擦除
 * @param start_sector_address 扇区起始地址
 * @param end_sector_address 扇区结束地址
 * @param sector_count 需要擦除扇区总个数
 * @note 该函数只做了三个简单校验，请在上一步校验完毕再调用该底层操作函数
 */
static Drv_Flash_Result_t Drv_Flash_Erase_Sector(uint32_t start_sector_address, uint32_t end_sector_address , uint32_t sector_count) {
    // 检查一下地址范围是否合法
    uint32_t primask;
    uint32_t sector_address = 0 ;           // 扇区地址
    flash_status_type flash_status;         // 库函数中的Flash操作返回值类型
    Drv_Flash_Result_t lock_status;         // 加解锁状态临时存储

    // 核对地址是否对齐扇区边界
    if (((start_sector_address % DRV_FLASH_SECTOR_SIZE) != 0U) || (((end_sector_address + 1U) % DRV_FLASH_SECTOR_SIZE) != 0U)) {
        return DRV_FLASH_ERR_ALIGN;
    }

    // 简单核对一下地址范围
    if ((start_sector_address + (sector_count * DRV_FLASH_SECTOR_SIZE) - 1U) != end_sector_address) {
        return DRV_FLASH_ERR_PARAM;
    }

    // 保护区检查
    if ((start_sector_address <= DRV_FLASH_PROTECT_END_ADDR) && (end_sector_address >= DRV_FLASH_PROTECT_START_ADDR)) {
        return DRV_FLASH_ERR_PROTECT;
    }

    // 关中断
    primask = __get_PRIMASK();  
    __disable_irq();

    // 解锁并且核对
    lock_status = Drv_Flash_Unlock();
    if (lock_status != DRV_FLASH_OK) {
        Drv_Flash_Lock();
        __set_PRIMASK(primask);
        return DRV_FLASH_ERR_UNLOCK;
    }


    // 清除可能遗留的 操作完成和错误标志位
    Drv_Flash_Clear_All_Flags();

    for (int i = 0; i < sector_count; i++) {
        sector_address = start_sector_address + i * DRV_FLASH_SECTOR_SIZE;

        flash_status = flash_sector_erase(sector_address);
        if (flash_status != FLASH_OPERATE_DONE) {
            //返回状态 != 操作完成，说明擦除失败
            Drv_Flash_Lock();
            __set_PRIMASK(primask);
            return DRV_FLASH_ERR_ERASE;
        }
    }

    // 加锁并且核对
    lock_status = Drv_Flash_Lock();
    if (lock_status != DRV_FLASH_OK) {
        __set_PRIMASK(primask);
        return DRV_FLASH_ERR_LOCK;
    }



    // 开中断
    __set_PRIMASK(primask);
    return DRV_FLASH_OK;

}



/**
 * @brief 正真的写入函数-按 "字" 写入
 * @param start_address Flash 写入起始地址
 * @param data 待写入数据缓冲区
 * @param data_size 写入长度，单位：字节
 * @return Flash 操作结果
 * @note 该函数不会任何合法性校验，请在上一步校验完毕再调用该底层操作函数
 */
static Drv_Flash_Result_t Drv_Flash_Write_Words(uint32_t start_address, const void *data, size_t data_size) {
    const uint8_t *bytes;               // 按字节访问源数据
    size_t offset;                      // 当前 word 相对于起始地址的偏移量
    uint32_t word_data;                 // 当前准备写入的 32 位数据
    uint32_t primask;
    flash_status_type flash_status;     // 库函数中的Flash操作返回值类型
    Drv_Flash_Result_t lock_status;     // 加解锁状态临时存储

    bytes = (const uint8_t *)data;


    // 关中断
    primask = __get_PRIMASK();
    __disable_irq();

    // 解锁并且核对
    lock_status = Drv_Flash_Unlock();
    if (lock_status != DRV_FLASH_OK) {
        Drv_Flash_Lock();
        __set_PRIMASK(primask);
        return DRV_FLASH_ERR_UNLOCK;
    }

    // 清除可能遗留的 操作完成和错误标志位
    Drv_Flash_Clear_All_Flags();

    for (offset = 0U; offset < data_size; offset += DRV_FLASH_WRITE_ALIGN) {
        (void)memcpy(&word_data, &bytes[offset], sizeof(word_data));
        flash_status = flash_word_program(start_address + (uint32_t)offset,word_data);
        if (flash_status != FLASH_OPERATE_DONE) {
            // 返回状态 != 操作完成，说明写入失败
            Drv_Flash_Lock();
            __set_PRIMASK(primask);
            return DRV_FLASH_ERR_WRITE;
        }
    }

    //加锁并且核对
    lock_status = Drv_Flash_Lock();
    if (lock_status != DRV_FLASH_OK) {
        __set_PRIMASK(primask);
        return DRV_FLASH_ERR_LOCK;
    }

    // 开中断
    __set_PRIMASK(primask);
    return DRV_FLASH_OK;
}

/**
 * @brief 
 * @param start_sector 
 * @param end_sector 
 * @param sector_count 
 * @return 
 */
Drv_Flash_Result_t Drv_Flash_Erase_Sector_For_Number(uint32_t start_sector, uint32_t end_sector , uint32_t sector_count) {
    Drv_Flash_Result_t result;
    uint32_t start_sector_address = 0;
    uint32_t end_sector_address = 0;

    // 检查扇区范围是否合法
    if (Drv_Flash_Sector_Range_Valid(start_sector, end_sector) == false) {
        return DRV_FLASH_ERR_ADDR;
    }

    // 检查扇区数量是否正确
    if ((end_sector - start_sector + 1U) != sector_count) {
        return DRV_FLASH_ERR_PARAM;
    }

    start_sector_address = Drv_Flash_Get_Sector_Start_Address(start_sector);                            // 获取开始扇区地址
    end_sector_address = Drv_Flash_Get_Sector_Start_Address(end_sector) + DRV_FLASH_SECTOR_SIZE - 1U;   // 获取结束扇区地址

    // 校验地址合法性(这一步似乎可以省略，因为检查过扇区范围合法性，有调用了我写的Drv_Flash_Get_Sector_Start_Address，理论不会错了)
    Drv_Flash_Address_Range_Valid(start_sector_address, end_sector_address);

    
    result = Drv_Flash_Erase_Sector(start_sector_address, end_sector_address, sector_count);

    return result;
}


/**
 * @brief
 * @param start_address 扇区开始地址
 * @param end_address   扇区结束地址
 * @note 地址必须是扇区的开始和结束，实参请用宏
 * @note TODO：这个函数有必要升级一下地址自适应
 * @return
 */
Drv_Flash_Result_t Drv_Flash_Erase_Sector_For_Address(uint32_t start_address, uint32_t end_address) {
    uint32_t sector_count = 0;
    // 检查地址范围是否合法
    if (Drv_Flash_Address_Range_Valid(start_address, end_address) == false) {
        return DRV_FLASH_ERR_ADDR;
    }

    // 检查地址是否对齐扇区边界
    if (((start_address % DRV_FLASH_SECTOR_SIZE) != 0U) || (((end_address + 1U) % DRV_FLASH_SECTOR_SIZE) != 0U)) {
        return DRV_FLASH_ERR_ALIGN;
    }

    // 计算需要擦除的扇区数量
    sector_count = (end_address - start_address + 1U) / DRV_FLASH_SECTOR_SIZE;

    if(sector_count == 0) {
        return DRV_FLASH_ERR_PARAM;
    }

    return Drv_Flash_Erase_Sector(start_address, end_address, sector_count);
}






/**
 * @brief 从内部 Flash 区读取数据。
 * @param start_address Flash 读取起始地址。
 * @param data          接收数据的可写缓冲区。
 * @param data_size     需要读取的数据长度，单位：字节。
 * @return Flash 操作结果。
 * @note data 必须指向可写缓冲区，并且缓冲区可用空间不得小于 data_size 字节，否则会造成缓冲区越界。
 */
Drv_Flash_Result_t Drv_Flash_Read(uint32_t start_address, void *data, size_t data_size) {
    uint32_t available_size;

    // 非零检查
    if ((data == NULL) || (data_size == 0U)) {
        return DRV_FLASH_ERR_PARAM;
    }

    // 地址判断，这个函数允许读Flash范围内任意地址
    if ((start_address < DRV_FLASH_START_ADDR) || (start_address > DRV_FLASH_END_ADDR)) {
        return DRV_FLASH_ERR_ADDR;
    }

    /*
     * 使用“剩余空间”判断，避免计算
     * start_address + data_size - 1U 时发生整数溢出。
     */
    available_size = DRV_FLASH_END_ADDR - start_address + 1U;
    if (data_size > (size_t)available_size) {
        return DRV_FLASH_ERR_ADDR;
    }

    // 读取的具体实现
    (void)memcpy(data, (const void *)(uintptr_t)start_address, data_size);

    return DRV_FLASH_OK;
}





/**
 * @brief 以严格的 32 位 word 模式向内部 Flash 写入数据。
 * @param address   Flash 写入起始地址。
 * @param data      待写入数据缓冲区。
 * @param data_size 待写入数据长度，单位：字节。
 * @return Flash 操作结果。
 *
 * @note 本函数采用严格模式：
 *       address 必须按 4 字节对齐；
 *       data_size 必须大于 0 且为 4 的整数倍。
 * @note 本函数不会自动补齐数据。最后不足 4 字节的数据必须由调用方
 *       使用 0xFF 补齐后再调用。
 * @note data 必须指向至少包含 data_size 字节的可读缓冲区，
 *       但 data 指针本身不要求 4 字节对齐，且要注意小端问题。
 * @note 写入目标区域必须已经擦除，本函数不会自动执行擦除操作。
 */
Drv_Flash_Result_t Drv_Flash_Write(uint32_t address, const void *data, size_t data_size) {
    uint32_t available_size;            // 从写入地址到 Flash 末尾的剩余空间
    Drv_Flash_Result_t write_result;

    // 非空检查
    if ((data == NULL) || (data_size == 0U)) {
        return DRV_FLASH_ERR_PARAM;
    }

    // 检查写入地址合法性
    if (Drv_Flash_Address_Range_Valid(address, address) == false) {
        return DRV_FLASH_ERR_ADDR;
    }


    // 检查写入地址和写入长度都必须按照 4 字节对齐
    if (((address % DRV_FLASH_WRITE_ALIGN) != 0U) || ((data_size % DRV_FLASH_WRITE_ALIGN) != 0U)) {
        return DRV_FLASH_ERR_ALIGN;
    }

    // 剩余地址空间检查，避免发生无符号回绕
    available_size = DRV_FLASH_END_ADDR - address + 1U;
    if (data_size > (size_t)available_size) {
        return DRV_FLASH_ERR_ADDR;
    }

    // 检查目标范围是否处于已经擦除状态
    if (Drv_Flash_Is_Erased(address, data_size) == false) {
        return DRV_FLASH_ERR_UNERASED;
    }
    
    write_result = Drv_Flash_Write_Words(address, data, data_size);

    if (write_result != DRV_FLASH_OK) {
        return write_result;
    }

    
    // 回读 Flash，并与源数据逐字节比较，确认实际写入内容正确。
    if (memcmp((const void *)(uintptr_t)address, data, data_size) != 0) {
        return DRV_FLASH_ERR_VERIFY;
    }

    return DRV_FLASH_OK;
}



/**
 * @brief 写入任意正长度数据，并使用 0xFF 补齐最后不足一个 word 的尾巴。
 * @param address   Flash 写入起始地址，必须按 4 字节对齐。
 * @param data      待写入数据缓冲区。
 * @param data_size 真实数据长度，单位：字节，允许不是 4 的整数倍。
 * @return Flash 操作结果。
 *
 * @note Drv_Flash_Write() 继续保持严格模式，本函数只是建立在严格接口之上的便利封装。
 * @note 如果 data_size 已经是 4 的整数倍，本函数直接调用 Drv_Flash_Write()。
 * @note 如果存在 1/2/3 字节尾巴，本函数先写入对齐主体，再把尾巴和 0xFF
 *       组成一个完整的 32 位 word 写入。
 * @note 本操作不是事务操作：对齐主体写入成功后，如果最后一个 word 写入失败，
 *       Flash 中可能保留已经写入的主体数据，调用方必须按失败处理。
 */
Drv_Flash_Result_t Drv_Flash_Write_Padded(uint32_t address, const void *data, size_t data_size) {
    const uint8_t *bytes;               // 按字节访问调用方真实数据
    uint32_t available_size;             // 从起始地址到 Flash 末尾的剩余空间
    uint32_t tail_word;                  // 由真实尾巴和 0xFF 组成的最后一个 word
    size_t body_size;                    // 能够直接严格写入的 4 字节对齐主体长度
    size_t tail_size;                    // 最后不足一个 word 的真实字节数：1/2/3
    size_t padded_size;                  // 主体加补齐 word 后实际占用的 Flash 长度
    Drv_Flash_Result_t result;

    // 空指针和零长度仍然按照非法参数处理。
    if ((data == NULL) || (data_size == 0U)) {
        return DRV_FLASH_ERR_PARAM;
    }

    // 数据长度已经对齐时，不增加额外路径，直接复用严格写入接口。
    tail_size = data_size % DRV_FLASH_WRITE_ALIGN;
    if (tail_size == 0U) {
        return Drv_Flash_Write(address, data, data_size);
    }

    // 自动补齐只能解决长度问题，Flash 写入起始地址本身仍必须按 word 对齐。
    if (Drv_Flash_Address_Range_Valid(address, address) == false) {
        return DRV_FLASH_ERR_ADDR;
    }
    if ((address % DRV_FLASH_WRITE_ALIGN) != 0U) {
        return DRV_FLASH_ERR_ALIGN;
    }

    body_size = data_size - tail_size;
    available_size = DRV_FLASH_END_ADDR - address + 1U;

    /*
     * 先检查补齐后的完整范围，避免主体写完后才发现最后一个补齐 word 越界。
     * body_size 大于剩余空间时不能进行减法，否则 size_t 会发生无符号回绕。
     */
    if ((body_size > (size_t)available_size) || (((size_t)available_size - body_size) < DRV_FLASH_WRITE_ALIGN)) {
        return DRV_FLASH_ERR_ADDR;
    }
    padded_size = body_size + DRV_FLASH_WRITE_ALIGN;

    // 在任何实际写入前检查完整目标范围，尽量避免因尾部未擦除造成部分写入。
    if (Drv_Flash_Is_Erased(address, padded_size) == false) {
        return DRV_FLASH_ERR_UNERASED;
    }

    bytes = (const uint8_t *)data;

    // 主体长度可能为 0，例如 data_size 只有 1/2/3 字节，此时直接处理尾巴。
    if (body_size > 0U) {
        result = Drv_Flash_Write(address, data, body_size);
        if (result != DRV_FLASH_OK) {
            return result;
        }
    }

    /*
     * 先把整个 word 设为擦除值，再复制真实尾巴。
     * AT32F403A 为小端架构，最终 Flash 字节顺序为：真实尾巴 + 0xFF 补齐字节。
     */
    tail_word = DRV_FLASH_EMPTY_WORD;
    (void)memcpy(&tail_word, &bytes[body_size], tail_size);

    return Drv_Flash_Write(address + (uint32_t)body_size,
                           &tail_word,
                           sizeof(tail_word));
}



/**
 * @brief 检查指定的内部 Flash 地址范围是否已经全部擦除。
 * @param start_address Flash 检查起始地址。
 * @param data_size     需要检查的数据长度，单位：字节。
 * @return true：目标范围全部为 0xFF；false：参数非法、地址越界或存在非 0xFF 数据。
 * @note 本函数允许检查整个内部 Flash，包括 BootLoader 区域。
 * @note data_size 必须大于 0，并且从 start_address 开始的完整范围不能越过 Flash 末尾。
 */
bool Drv_Flash_Is_Erased(uint32_t start_address, size_t data_size) {
    const volatile uint8_t *flash_data;  // 指向需要检查的 Flash 映射地址
    uint32_t available_size;             // 从起始地址到 Flash 末尾的剩余空间
    size_t index;                        // 当前检查字节相对于起始地址的偏移量

    // 长度为 0 时没有实际检查对象，按参数非法处理。
    if (data_size == 0U) {
        return false;
    }

    // 本函数允许检查整个内部 Flash，但不允许访问 Flash 地址范围之外。
    if ((start_address < DRV_FLASH_START_ADDR) || (start_address > DRV_FLASH_END_ADDR)) {
        return false;
    }

    /*
     * 使用剩余空间判断完整检查范围，避免计算
     * start_address + data_size - 1U 时发生无符号整数回绕。
     */
    available_size = DRV_FLASH_END_ADDR - start_address + 1U;
    if (data_size > (size_t)available_size) {
        return false;
    }

    /*
     * Flash 擦除后的每个字节都应为 0xFF。
     * 使用 volatile 指针保证每次比较都实际读取 Flash 映射地址。
     */
    flash_data = (const volatile uint8_t *)(uintptr_t)start_address;
    for (index = 0U; index < data_size; index++) {
        if (flash_data[index] != DRV_FLASH_EMPTY_BYTE) {
            return false;
        }
    }

    return true;
}
