#include "MySPI.h"
#include "W25Q16.h"
#include "W25Q16_Ins.h"

static void W25Q16_WriteEnable(void)
{
    MySPI_Start();
    MySPI_SwapByte(W25Q16_WRITE_ENABLE);
    MySPI_Stop();
}

static void W25Q16_WaitBusy(void)
{
    uint32_t Timeout;
    MySPI_Start();
    MySPI_SwapByte(W25Q16_READ_STATUS_REGISTER_1);
    Timeout = 100000;
    while ((MySPI_SwapByte(W25Q16_DUMMY_BYTE) & 0x01) == 0x01)
    {
        if (--Timeout == 0) break;
    }
    MySPI_Stop();
}

void W25Q16_Init(void)
{
    MySPI_Init();
}

void W25Q16_ReadID(uint8_t *MID, uint16_t *DID)
{
    MySPI_Start();
    MySPI_SwapByte(W25Q16_JEDEC_ID);
    *MID = MySPI_SwapByte(W25Q16_DUMMY_BYTE);
    *DID = MySPI_SwapByte(W25Q16_DUMMY_BYTE);
    *DID <<= 8;
    *DID |= MySPI_SwapByte(W25Q16_DUMMY_BYTE);
    MySPI_Stop();
}

void W25Q16_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count)
{
    // 检查地址是否在有效范围内，并且不跨页（可由调用者保证）
    if (Address + Count > W25Q16_SIZE)
        return;  // 错误处理，可添加断言或返回错误码

    W25Q16_WriteEnable();

    MySPI_Start();
    MySPI_SwapByte(W25Q16_PAGE_PROGRAM);
    MySPI_SwapByte(Address >> 16);
    MySPI_SwapByte(Address >> 8);
    MySPI_SwapByte(Address);
    for (uint16_t i = 0; i < Count; i++)
    {
        MySPI_SwapByte(DataArray[i]);
    }
    MySPI_Stop();

    W25Q16_WaitBusy();
}

void W25Q16_SectorErase(uint32_t Address)
{
    // 地址必须对齐到扇区（一般由调用者保证）
    if (Address >= W25Q16_SIZE)
        return;

    W25Q16_WriteEnable();

    MySPI_Start();
    MySPI_SwapByte(W25Q16_SECTOR_ERASE_4KB);
    MySPI_SwapByte(Address >> 16);
    MySPI_SwapByte(Address >> 8);
    MySPI_SwapByte(Address);
    MySPI_Stop();

    W25Q16_WaitBusy();
}

void W25Q16_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count)
{
    if (Address + Count > W25Q16_SIZE)
        return;

    MySPI_Start();
    MySPI_SwapByte(W25Q16_READ_DATA);
    MySPI_SwapByte(Address >> 16);
    MySPI_SwapByte(Address >> 8);
    MySPI_SwapByte(Address);
    for (uint32_t i = 0; i < Count; i++)
    {
        DataArray[i] = MySPI_SwapByte(W25Q16_DUMMY_BYTE);
    }
    MySPI_Stop();
}
