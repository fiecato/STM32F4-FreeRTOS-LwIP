#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lwip_comm.h"
#include "LAN8720.h"
#include "usmart.h"
#include "lcd.h"
#include "sram.h"
#include "malloc.h"
#include "lwip/netif.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "lwipopts.h"

//ALIENTEK 探索者STM32F407开发板
//LWIP LWIP无操作系统移植测试
//技术支持：www.openedv.com
//广州市星翼电子科技有限公司


//在LCD上显示地址信息任务
//任务优先级
#define DISPLAYT_TASK_PRIO		8
//任务堆栈大小	
#define DISPLAY_STK_SIZE 		128  
//任务句柄
TaskHandle_t DISPLAY_Task_Handler;
//任务函数
void display_task(void *pvParameters);

//LED任务
//任务优先级
#define LED_TASK_PRIO		9
//任务堆栈大小	
#define LED_STK_SIZE 		64  
//任务句柄
TaskHandle_t LED_Task_Handler;
//任务函数
void led_task(void *pvParameters);

//START任务
//任务优先级
#define START_TASK_PRIO		10
//任务堆栈大小	
#define START_STK_SIZE 		128  
//任务句柄
TaskHandle_t START_Task_Handler;
//任务函数
void start_task(void *pvParameters);

//在LCD上显示地址信息
//mode:1 显示DHCP获取到的地址
//	  其他 显示静态地址
void show_address(u8 mode)
{
	u8 buf[30];
	if(mode==2)
	{
		sprintf((char*)buf,"MAC    :%d.%d.%d.%d.%d.%d",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);//打印MAC地址
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"DHCP IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//打印动态IP地址
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"DHCP GW:%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//打印网关地址
		LCD_ShowString(30,170,210,16,16,buf); 
		sprintf((char*)buf,"DHCP IP:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//打印子网掩码地址
		LCD_ShowString(30,190,210,16,16,buf); 
	}
	else 
	{
		sprintf((char*)buf,"MAC      :%d.%d.%d.%d.%d.%d",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);//打印MAC地址
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//打印动态IP地址
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"Static GW:%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//打印网关地址
		LCD_ShowString(30,170,210,16,16,buf); 
		sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//打印子网掩码地址
		LCD_ShowString(30,190,210,16,16,buf); 
	}	
}

int main(void)
{
	delay_init(168);       								//延时初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);		//设置NVIC中断分组4
	uart_init(115200);    								//串口波特率设置
	usmart_dev.init(84); 								//初始化USMART
	LED_Init();  										//LED初始化
	KEY_Init();  										//按键初始化
	LCD_Init(); 										//LCD初始化
	FSMC_SRAM_Init();									//初始化外部SRAM  
	
	my_mem_init(SRAMIN);								//初始化内部内存池
	my_mem_init(SRAMEX);								//初始化外部内存池
	my_mem_init(SRAMCCM);	  							//初始化CCM内存池
	
	POINT_COLOR = RED; 		
	LCD_ShowString(30,30,200,16,16,"Explorer STM32F4");
	LCD_ShowString(30,50,200,16,16,"Ethernet lwIP Test");
	LCD_ShowString(30,70,200,16,16,"ATOM@ALIENTEK");
	LCD_ShowString(30,90,200,16,16,"2018-10-22  17:00"); 	
  
	while(lwip_comm_init()) //lwip初始化
	{
		LCD_ShowString(30,110,200,20,16,"LWIP Init Falied!");   //lwip初始化失败
		delay_ms(1200);
		LCD_Fill(30,110,230,130,WHITE); //清除显示
		LCD_ShowString(30,110,200,16,16,"Retrying...");
	}
	LCD_ShowString(30,110,200,20,16,"LWIP Init Success!");    //lwip初始化成功
	
	//创建开始任务
	xTaskCreate((TaskFunction_t )start_task,            //任务函数
			  (const char*    )"start_task",          	//任务名称
			  (uint16_t       )START_STK_SIZE,        	//任务堆栈大小
			  (void*          )NULL,                 	//传递给任务函数的参数
			  (UBaseType_t    )START_TASK_PRIO,       	//任务优先级
			  (TaskHandle_t*  )&START_Task_Handler);  	//任务句柄              
	vTaskStartScheduler();          					//开启任务调度
}

//start任务
void start_task(void *pvParameters)
{
	taskENTER_CRITICAL();      							//进入临界区
#if LWIP_DHCP
	lwip_comm_dhcp_creat(); 							//创建DHCP任务
#endif
	//创建LED任务
  xTaskCreate((TaskFunction_t )led_task,              	//任务函数
              (const char*    )"led_task",           	//任务名称
              (uint16_t       )LED_STK_SIZE,        	//任务堆栈大小
              (void*          )NULL,                  	//传递给任务函数的参数
              (UBaseType_t    )LED_TASK_PRIO,       	//任务优先级
              (TaskHandle_t*  )&LED_Task_Handler);   	//任务句柄 
	
 	//创建DISPLAY任务
  xTaskCreate((TaskFunction_t )display_task,          	//任务函数
              (const char*    )"display_task",        	//任务名称
              (uint16_t       )DISPLAY_STK_SIZE,      	//任务堆栈大小
              (void*          )NULL,                  	//传递给任务函数的参数
              (UBaseType_t    )DISPLAYT_TASK_PRIO,    	//任务优先级
              (TaskHandle_t*  )&DISPLAY_Task_Handler);	//任务句柄 
  vTaskSuspend(START_Task_Handler);						//挂起开始任务
  taskEXIT_CRITICAL();             						//退出临界区								
}

//显示地址等信息
void display_task(void *pvParameters)
{
	while(1)
	{ 

#if LWIP_DHCP									        //当开启DHCP的时候
		if(lwipdev.dhcpstatus != 0) 					//开启DHCP
		{
			show_address(lwipdev.dhcpstatus );			//显示地址信息
			vTaskSuspend(DISPLAY_Task_Handler); 		//显示完地址信息后挂起自身任务
		}
#else
		show_address(0); 						        //显示静态地址
		vTaskSuspend(DISPLAY_Task_Handler); 			//显示完地址信息后挂起自身任务
#endif //LWIP_DHCP
		vTaskDelay(500);      							//延时500ms
	}
}

//led任务
void led_task(void *pvParameters)
{
	while(1)
	{
		LED0 = !LED0;
		vTaskDelay(500);     							 //延时500ms
 	}
}
