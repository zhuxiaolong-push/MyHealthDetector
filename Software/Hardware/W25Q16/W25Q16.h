#ifndef __W25Q16_H
#define __W25Q16_H

#include <stdint.h>

// W25Q16 容量定义
#define W25Q16_SIZE                (2 * 1024 * 1024)   // 2MB
#define W25Q16_SECTOR_SIZE         4096                // 4KB 扇区
#define W25Q16_PAGE_SIZE           256                 // 256B 页
#define W25Q16_MAX_ADDRESS         (W25Q16_SIZE - 1)   // 最大地址 0x1FFFFF

void W25Q16_Init(void);
void W25Q16_ReadID(uint8_t *MID, uint16_t *DID);
void W25Q16_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count);
void W25Q16_SectorErase(uint32_t Address);
void W25Q16_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count);

#endif
