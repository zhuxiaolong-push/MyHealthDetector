#ifndef __TOUCH_H__
#define __TOUCH_H__

#include "Com_sys.h"
#include "ott2001a.h"	    
#include "gt9147.h"	    
#include "ft5206.h"	    
#include "W25Q16.h"        // 添加 W25Q16 头文件

// 校准参数在 W25Q16 中的存储地址 -> 第一个扇区（0x000000）
#define TP_ADJ_DATA_ADDR    0x000000

// 校准数据标记
#define TP_ADJ_MAGIC_NUM    0x5A5A           // 校准有效标记

#define TP_PRES_DOWN 0x80  //触屏被按下	  
#define TP_CATH_PRES 0x40  //有按键按下了 
#define CT_MAX_TOUCH  5    //电容屏支持的点数,固定为5点

// 校准数据结构（用于保存到 Flash）
typedef struct
{
    uint16_t magic;          // 标记 0x5A5A 表示已校准
    float xfac;
    float yfac;
    short xoff;
    short yoff;
    uint8_t touchtype;
    uint16_t checksum;       // 校验和
}_tp_adj_data;

//触摸屏控制器
typedef struct
{
	u8 (*init)(void);			//初始化触摸屏控制器
	u8 (*scan)(u8);				//扫描触摸屏.0,屏幕扫描;1,物理坐标;	 
	void (*adjust)(void);		//触摸屏校准 
	u16 x[CT_MAX_TOUCH]; 		//当前坐标
	u16 y[CT_MAX_TOUCH];		//电容屏有最多5组坐标,电阻屏则用x[0],y[0]代表:此次扫描时,触屏的坐标,用
								//x[4],y[4]存储第一次按下时的坐标. 
	u8  sta;					//笔的状态 
								//b7:按下1/松开0; 
	                            //b6:0,没有按键按下;1,有按键按下. 
								//b5:保留
								//b4~b0:电容触摸屏按下的点数(0,表示未按下,1表示按下)
/////////////////////触摸屏校准参数(电容屏不需要校准)//////////////////////								
	float xfac;					
	float yfac;
	short xoff;
	short yoff;	   
//新增的参数,当触摸屏的左右上下完全颠倒时需要用到.
//b0:0,竖屏(适合左右为X坐标,上下为Y坐标的TP)
//   1,横屏(适合左右为Y坐标,上下为X坐标的TP) 
//b1~6:保留.
//b7:0,电阻屏
//   1,电容屏 
	u8 touchtype;
}_m_tp_dev;

extern _m_tp_dev tp_dev;	 	//触屏控制器在touch.c里面定义

//电阻屏芯片连接引脚	   
#define PEN  		PEin(11)  	//T_PEN
#define DOUT 		PEin(8)   	//T_MISO
#define TDIN 		PEout(9)  	//T_MOSI
#define TCLK 		PEout(7)  	//T_SCK
#define TCS  		PBout(1)  	//T_CS  
   
//电阻屏函数
void TP_Write_Byte(u8 num);						//向控制芯片写入一个数据
u16 TP_Read_AD(u8 CMD);							//读取AD转换值
u16 TP_Read_XOY(u8 xy);							//带滤波的坐标读取(X/Y)
u8 TP_Read_XY(u16 *x,u16 *y);					//双方向读取(X+Y)
u8 TP_Read_XY2(u16 *x,u16 *y);					//带加强滤波的双方向坐标读取
void TP_Drow_Touch_Point(u16 x,u16 y,u16 color);//画一个坐标校准点
void TP_Draw_Big_Point(u16 x,u16 y,u16 color);	//画一个大点

// 新的 Flash 存储函数
void TP_Save_Adjdata(void);						//保存校准参数到 W25Q16
u8 TP_Get_Adjdata(void);						//从 W25Q16 读取校准参数
void TP_Clear_Adjdata(void);					//清除校准数据（强制重新校准）

void TP_Adjust(void);							//触摸屏校准
void TP_Adj_Info_Show(u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2,u16 x3,u16 y3,u16 fac);//显示校准信息
//电阻屏/电容屏 共用函数
u8 TP_Scan(u8 tp);								//扫描
u8 TP_Init(void);								//初始化
 
#endif
