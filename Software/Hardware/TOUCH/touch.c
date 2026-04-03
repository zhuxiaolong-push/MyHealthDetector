#include "touch.h" 
#include "lcd.h"
#include "Com_Delay.h"
#include "stdlib.h"
#include "math.h"
#include "W25Q16.h"        // 替换 24cxx.h

_m_tp_dev tp_dev=
{
	TP_Init,
	TP_Scan,
	TP_Adjust,
	0,
	0, 
	0,
	0,
	0,
	0,	  	 		
	0,
	0,	  	 		
};					
//默认为touchtype=0的数据.
u8 CMD_RDX=0XD0;
u8 CMD_RDY=0X90;
 	 			    					   
//SPI写数据（保持不变）
void TP_Write_Byte(u8 num)    
{  
	u8 count=0;   
	for(count=0;count<8;count++)  
	{ 	  
		if(num&0x80)TDIN=1;  
		else TDIN=0;   
		num<<=1;    
		TCLK=0; 
		delay_us(1);
		TCLK=1;		//上升沿有效	        
	}		 			    
} 		 

//SPI读数据（保持不变）
u16 TP_Read_AD(u8 CMD)	  
{ 	 
	u8 count=0; 	  
	u16 Num=0; 
	TCLK=0;		//先拉低时钟 	 	 
	TDIN=0; 	//拉低数据线
	TCS=0; 		//选中触摸屏IC
	TP_Write_Byte(CMD);//发送命令字
	delay_us(6);//ADS7846的转换时间最长为6us
	TCLK=0; 	     	    	   
	delay_us(1);    	   
	TCLK=1;		//给1个时钟，清除BUSY
	delay_us(1);    
	TCLK=0; 	     	    
	for(count=0;count<16;count++)//读出16位数据,只有高12位有效 
	{ 				  
		Num<<=1; 	 
		TCLK=0;	//下降沿有效  	    	   
		delay_us(1);    
 		TCLK=1;
 		if(DOUT)Num++; 		 
	}  	
	Num>>=4;   	//只有高12位有效.
	TCS=1;		//释放片选	 
	return(Num);   
}

#define READ_TIMES 5 	
#define LOST_VAL 1	  	
u16 TP_Read_XOY(u8 xy)
{
	u16 i, j;
	u16 buf[READ_TIMES];
	u16 sum=0;
	u16 temp;
	for(i=0;i<READ_TIMES;i++)buf[i]=TP_Read_AD(xy);		 		    
	for(i=0;i<READ_TIMES-1; i++)
	{
		for(j=i+1;j<READ_TIMES;j++)
		{
			if(buf[i]>buf[j])
			{
				temp=buf[i];
				buf[i]=buf[j];
				buf[j]=temp;
			}
		}
	}	  
	sum=0;
	for(i=LOST_VAL;i<READ_TIMES-LOST_VAL;i++)sum+=buf[i];
	temp=sum/(READ_TIMES-2*LOST_VAL);
	return temp;   
} 

u8 TP_Read_XY(u16 *x,u16 *y)
{
	u16 xtemp,ytemp;			 	 		  
	xtemp=TP_Read_XOY(CMD_RDX);
	ytemp=TP_Read_XOY(CMD_RDY);	  												   
	*x=xtemp;
	*y=ytemp;
	return 1;
}

#define ERR_RANGE 50 
u8 TP_Read_XY2(u16 *x,u16 *y) 
{
	u16 x1,y1;
 	u16 x2,y2;
 	u8 flag;    
    flag=TP_Read_XY(&x1,&y1);   
    if(flag==0)return(0);
    flag=TP_Read_XY(&x2,&y2);	   
    if(flag==0)return(0);   
    if(((x2<=x1&&x1<x2+ERR_RANGE)||(x1<=x2&&x2<x1+ERR_RANGE))
    &&((y2<=y1&&y1<y2+ERR_RANGE)||(y1<=y2&&y2<y1+ERR_RANGE)))
    {
        *x=(x1+x2)/2;
        *y=(y1+y2)/2;
        return 1;
    }else return 0;	  
}  

void TP_Drow_Touch_Point(u16 x,u16 y,u16 color)
{
	POINT_COLOR=color;
	LCD_DrawLine(x-12,y,x+13,y);
	LCD_DrawLine(x,y-12,x,y+13);
	LCD_DrawPoint(x+1,y+1);
	LCD_DrawPoint(x-1,y+1);
	LCD_DrawPoint(x+1,y-1);
	LCD_DrawPoint(x-1,y-1);
	LCD_Draw_Circle(x,y,6);
}	  

void TP_Draw_Big_Point(u16 x,u16 y,u16 color)
{	    
	POINT_COLOR=color;
	LCD_DrawPoint(x,y);
	LCD_DrawPoint(x+1,y);
	LCD_DrawPoint(x,y+1);
	LCD_DrawPoint(x+1,y+1);	 	  	
}						  

u8 TP_Scan(u8 tp)
{			   
	if(PEN==0)
	{
		if(tp)TP_Read_XY2(&tp_dev.x[0],&tp_dev.y[0]);
		else if(TP_Read_XY2(&tp_dev.x[0],&tp_dev.y[0]))
		{
	 		tp_dev.x[0]=tp_dev.xfac*tp_dev.x[0]+tp_dev.xoff;
			tp_dev.y[0]=tp_dev.yfac*tp_dev.y[0]+tp_dev.yoff;  
	 	} 
		if((tp_dev.sta&TP_PRES_DOWN)==0)
		{		 
			tp_dev.sta=TP_PRES_DOWN|TP_CATH_PRES;
			tp_dev.x[4]=tp_dev.x[0];
			tp_dev.y[4]=tp_dev.y[0];  	   			 
		}			   
	}else
	{
		if(tp_dev.sta&TP_PRES_DOWN)
		{
			tp_dev.sta&=~(1<<7);
		}else
		{
			tp_dev.x[4]=0;
			tp_dev.y[4]=0;
			tp_dev.x[0]=0xffff;
			tp_dev.y[0]=0xffff;
		}	    
	}
	return tp_dev.sta&TP_PRES_DOWN;
}	  

//////////////////////////////////////////////////////////////////////////	 
// 计算校验和
static uint16_t TP_Calc_Checksum(_tp_adj_data *data)
{
    uint16_t sum = 0;
    uint8_t *p = (uint8_t *)data;
    for(int i = 0; i < sizeof(_tp_adj_data) - 2; i++)
    {
        sum += p[i];
    }
    return sum;
}

// 保存校准参数到 W25Q16 第一个扇区（0x000000）
void TP_Save_Adjdata(void)
{
    _tp_adj_data save_data;
    
    save_data.magic = TP_ADJ_MAGIC_NUM;
    save_data.xfac = tp_dev.xfac;
    save_data.yfac = tp_dev.yfac;
    save_data.xoff = tp_dev.xoff;
    save_data.yoff = tp_dev.yoff;
    save_data.touchtype = tp_dev.touchtype;
    save_data.checksum = TP_Calc_Checksum(&save_data);
    
    // 擦除第一个扇区（地址 0x000000）
    W25Q16_SectorErase(TP_ADJ_DATA_ADDR);
    
    // 写入数据到第一页
    W25Q16_PageProgram(TP_ADJ_DATA_ADDR, (uint8_t *)&save_data, sizeof(_tp_adj_data));
}

// 从 W25Q16 第一个扇区读取校准参数
u8 TP_Get_Adjdata(void)
{					  
    _tp_adj_data read_data;
    uint16_t calc_checksum;
    
    W25Q16_ReadData(TP_ADJ_DATA_ADDR, (uint8_t *)&read_data, sizeof(_tp_adj_data));
    
    if(read_data.magic != TP_ADJ_MAGIC_NUM)
    {
        return 0;
    }
    
    calc_checksum = TP_Calc_Checksum(&read_data);
    if(calc_checksum != read_data.checksum)
    {
        return 0;
    }
    
    tp_dev.xfac = read_data.xfac;
    tp_dev.yfac = read_data.yfac;
    tp_dev.xoff = read_data.xoff;
    tp_dev.yoff = read_data.yoff;
    tp_dev.touchtype = read_data.touchtype;
    
    if(tp_dev.touchtype)
    {
        CMD_RDX = 0X90;
        CMD_RDY = 0XD0;	 
    }
    else
    {
        CMD_RDX = 0XD0;
        CMD_RDY = 0X90;	 
    }
    
    return 1;
}	 

// 清除校准数据（强制重新校准）
void TP_Clear_Adjdata(void)
{
    W25Q16_SectorErase(TP_ADJ_DATA_ADDR);
}

u8* const TP_REMIND_MSG_TBL="Please use the stylus click the cross on the screen.The cross will always move until the screen adjustment is completed.";
 					  
void TP_Adj_Info_Show(u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2,u16 x3,u16 y3,u16 fac)
{	  
	POINT_COLOR=RED;
	LCD_ShowString(40,160,lcddev.width,lcddev.height,16,"x1:");
 	LCD_ShowString(40+80,160,lcddev.width,lcddev.height,16,"y1:");
 	LCD_ShowString(40,180,lcddev.width,lcddev.height,16,"x2:");
 	LCD_ShowString(40+80,180,lcddev.width,lcddev.height,16,"y2:");
	LCD_ShowString(40,200,lcddev.width,lcddev.height,16,"x3:");
 	LCD_ShowString(40+80,200,lcddev.width,lcddev.height,16,"y3:");
	LCD_ShowString(40,220,lcddev.width,lcddev.height,16,"x4:");
 	LCD_ShowString(40+80,220,lcddev.width,lcddev.height,16,"y4:");  
 	LCD_ShowString(40,240,lcddev.width,lcddev.height,16,"fac is:");     
	LCD_ShowNum(40+24,160,x0,4,16);
	LCD_ShowNum(40+24+80,160,y0,4,16);
	LCD_ShowNum(40+24,180,x1,4,16);
	LCD_ShowNum(40+24+80,180,y1,4,16);
	LCD_ShowNum(40+24,200,x2,4,16);
	LCD_ShowNum(40+24+80,200,y2,4,16);
	LCD_ShowNum(40+24,220,x3,4,16);
	LCD_ShowNum(40+24+80,220,y3,4,16);
 	LCD_ShowNum(40+56,240,fac,3,16);
}

void TP_Adjust(void)
{								 
	u16 pos_temp[4][2];
	u8  cnt=0;	
	u16 d1,d2;
	u32 tem1,tem2;
	double fac; 	
	u16 outtime=0;
 	cnt=0;				
	POINT_COLOR=BLUE;
	BACK_COLOR =WHITE;
	LCD_Clear(WHITE);
	POINT_COLOR=RED;
	LCD_Clear(WHITE); 	   
	POINT_COLOR=BLACK;
	LCD_ShowString(40,40,160,100,16,(u8*)TP_REMIND_MSG_TBL);
	TP_Drow_Touch_Point(20,20,RED);
	tp_dev.sta=0;
	tp_dev.xfac=0;
	 
	while(1)
	{
		tp_dev.scan(1);
		if((tp_dev.sta&0xc0)==TP_CATH_PRES)
		{	
			outtime=0;		
			tp_dev.sta&=~(1<<6);
						   			   
			pos_temp[cnt][0]=tp_dev.x[0];
			pos_temp[cnt][1]=tp_dev.y[0];
			cnt++;	  
			switch(cnt)
			{			   
				case 1:						 
					TP_Drow_Touch_Point(20,20,WHITE);
					TP_Drow_Touch_Point(lcddev.width-20,20,RED);
					break;
				case 2:
 					TP_Drow_Touch_Point(lcddev.width-20,20,WHITE);
					TP_Drow_Touch_Point(20,lcddev.height-20,RED);
					break;
				case 3:
 					TP_Drow_Touch_Point(20,lcddev.height-20,WHITE);
 					TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,RED);
					break;
				case 4:
					tem1=abs(pos_temp[0][0]-pos_temp[1][0]);
					tem2=abs(pos_temp[0][1]-pos_temp[1][1]);
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);
					
					tem1=abs(pos_temp[2][0]-pos_temp[3][0]);
					tem2=abs(pos_temp[2][1]-pos_temp[3][1]);
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05||d1==0||d2==0)
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);
   	 					TP_Drow_Touch_Point(20,20,RED);
 						TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);
 						continue;
					}
					tem1=abs(pos_temp[0][0]-pos_temp[2][0]);
					tem2=abs(pos_temp[0][1]-pos_temp[2][1]);
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);
					
					tem1=abs(pos_temp[1][0]-pos_temp[3][0]);
					tem2=abs(pos_temp[1][1]-pos_temp[3][1]);
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05)
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);
   	 					TP_Drow_Touch_Point(20,20,RED);
 						TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);
						continue;
					}
								   
					tem1=abs(pos_temp[1][0]-pos_temp[2][0]);
					tem2=abs(pos_temp[1][1]-pos_temp[2][1]);
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);
	
					tem1=abs(pos_temp[0][0]-pos_temp[3][0]);
					tem2=abs(pos_temp[0][1]-pos_temp[3][1]);
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05)
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);
   	 					TP_Drow_Touch_Point(20,20,RED);
 						TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);
						continue;
					}
					
					tp_dev.xfac=(float)(lcddev.width-40)/(pos_temp[1][0]-pos_temp[0][0]);
					tp_dev.xoff=(lcddev.width-tp_dev.xfac*(pos_temp[1][0]+pos_temp[0][0]))/2;
						  
					tp_dev.yfac=(float)(lcddev.height-40)/(pos_temp[2][1]-pos_temp[0][1]);
					tp_dev.yoff=(lcddev.height-tp_dev.yfac*(pos_temp[2][1]+pos_temp[0][1]))/2;
  
					if(abs(tp_dev.xfac)>2||abs(tp_dev.yfac)>2)
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);
   	 					TP_Drow_Touch_Point(20,20,RED);
						LCD_ShowString(40,26,lcddev.width,lcddev.height,16,"TP Need readjust!");
						tp_dev.touchtype=!tp_dev.touchtype;
						if(tp_dev.touchtype)
						{
							CMD_RDX=0X90;
							CMD_RDY=0XD0;	 
						}else
						{
							CMD_RDX=0XD0;
							CMD_RDY=0X90;	 
						}			    
						continue;
					}		
					POINT_COLOR=BLUE;
					LCD_Clear(WHITE);
					LCD_ShowString(35,110,lcddev.width,lcddev.height,16,"Touch Screen Adjust OK!");
					delay_ms(1000);
					TP_Save_Adjdata();  // 保存到第一个扇区
 					LCD_Clear(WHITE);
					return;				 
			}
		}
		delay_ms(10);
		outtime++;
		if(outtime>1000)
		{
			TP_Get_Adjdata();
			break;
	 	} 
 	}
}	

u8 TP_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;	
	
    // 先初始化 W25Q16
    W25Q16_Init();
    
	if(lcddev.id==0X5510)
	{
		if(GT9147_Init()==0)
		{ 
			tp_dev.scan=GT9147_Scan;
		}else
		{
			OTT2001A_Init();
			tp_dev.scan=OTT2001A_Scan;
		}
		tp_dev.touchtype|=0X80;
		tp_dev.touchtype|=lcddev.dir&0X01;
		return 0;
	}else if(lcddev.id==0X1963)
	{
		FT5206_Init();
		tp_dev.scan=FT5206_Scan;
		tp_dev.touchtype|=0X80;
		tp_dev.touchtype|=lcddev.dir&0X01;
		return 0;
	}else
	{
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOE, ENABLE);

        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIOE, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
        GPIO_Init(GPIOE, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
        GPIO_Init(GPIOB, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIOE, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
        GPIO_Init(GPIOE, &GPIO_InitStructure);
		
        TP_Read_XY(&tp_dev.x[0],&tp_dev.y[0]);
        
        if(TP_Get_Adjdata())
        {
            return 0;
        }
        else
        {
            LCD_Clear(WHITE);
            TP_Adjust();
        }
        
        TP_Get_Adjdata();
    }
    
    return 1;
}
