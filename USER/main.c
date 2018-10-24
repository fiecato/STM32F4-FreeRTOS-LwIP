#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "timer.h"
#include "lcd.h"
#include "key.h"
#include "beep.h"
#include "string.h"
#include "malloc.h"
#include "FreeRTOS.h"
#include "task.h"
/************************************************
 ALIENTEK 探索者STM32F407开发板 FreeRTOS实验20-1
 FreeRTOS内存管理实验-库函数版本
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/

//任务优先级
#define START_TASK_PRIO		1
//任务堆栈大小	
#define START_STK_SIZE 		128  
//任务句柄
TaskHandle_t StartTask_Handler;
//任务函数
void start_task(void *pvParameters);

//任务优先级
#define MALLOC_TASK_PRIO	2
//任务堆栈大小	
#define MALLOC_STK_SIZE 	128
//任务句柄
TaskHandle_t MallocTask_Handler;
//任务函数
void malloc_task(void *p_arg);

int main(void)
{ 
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//设置系统中断优先级分组4
	delay_init(168);					//初始化延时函数
	uart_init(115200);     				//初始化串口
	LED_Init();		        			//初始化LED端口
	KEY_Init();							//初始化按键
	LCD_Init();							//初始化LCD
	my_mem_init(SRAMIN);            	//初始化内部内存池
	
	POINT_COLOR = RED;
	LCD_ShowString(30,10,200,16,16,"ATK STM32F103/407");	
	LCD_ShowString(30,30,200,16,16,"FreeRTOS Examp 20-1");
	LCD_ShowString(30,50,200,16,16,"Mem Manage");
	LCD_ShowString(30,70,200,16,16,"KEY_UP:Malloc,KEY1:Free");
	LCD_ShowString(30,90,200,16,16,"KEY0:Use Mem");
	LCD_ShowString(30,110,200,16,16,"ATOM@ALIENTEK");
	LCD_ShowString(30,130,200,16,16,"2016/11/14");
	
	LCD_ShowString(30,170,200,16,16,"Total Mem:      Bytes");
	LCD_ShowString(30,190,200,16,16,"Free  Mem:      Bytes");
	LCD_ShowString(30,210,200,16,16,"Message:    ");
	POINT_COLOR = BLUE;
	
	//创建开始任务
    xTaskCreate((TaskFunction_t )start_task,            //任务函数
                (const char*    )"start_task",          //任务名称
                (uint16_t       )START_STK_SIZE,        //任务堆栈大小
                (void*          )NULL,                  //传递给任务函数的参数
                (UBaseType_t    )START_TASK_PRIO,       //任务优先级
                (TaskHandle_t*  )&StartTask_Handler);   //任务句柄              
    vTaskStartScheduler();          //开启任务调度
}

//开始任务任务函数
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //进入临界区
    //创建TASK1任务
    xTaskCreate((TaskFunction_t )malloc_task,             
                (const char*    )"malloc_task",           
                (uint16_t       )MALLOC_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )MALLOC_TASK_PRIO,        
                (TaskHandle_t*  )&MallocTask_Handler);   
    vTaskDelete(StartTask_Handler); //删除开始任务
    taskEXIT_CRITICAL();            //退出临界区
}


//MALLOC任务函数 
void malloc_task(void *pvParameters)
{
	u8 *buffer;		
	u8 times,i,key=0;
	u32 freemem;

	LCD_ShowxNum(110,170,configTOTAL_HEAP_SIZE,5,16,0);//显示内存总容量	
    while(1)
    {
		key=KEY_Scan(0);
		switch(key)
		{
			case WKUP_PRES:				
				buffer=pvPortMalloc(30);			//申请内存，30个字节
				printf("申请到的内存地址为:%#x\r\n",(int)buffer);
				break;
			case KEY1_PRES:				
				if(buffer!=NULL)vPortFree(buffer);	//释放内存
				buffer=NULL;
				break;
			case KEY0_PRES:
				if(buffer!=NULL)					//buffer可用,使用buffer
				{
					times++;
					sprintf((char*)buffer,"User %d Times",times);//向buffer中填写一些数据
					LCD_ShowString(94,210,200,16,16,buffer);
				}
				break;
		}
		freemem=xPortGetFreeHeapSize();		//获取剩余内存大小
		LCD_ShowxNum(110,190,freemem,5,16,0);//显示内存总容量	
		i++;
		if(i==50)
		{
			i=0;
			LED0=~LED0;
		}
        vTaskDelay(10);
    }
} 

