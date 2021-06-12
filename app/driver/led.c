

#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "driver/led.h"

//静态全局变量
static os_timer_t sys_run_led_timer;	//系统指示灯运行定时器
static uint8 sys_led_status;			//SYS LED灯当前状态

//宏定义
#define SYS_RUN_LED_TIMER_PERIOD_MS  	1000//系统运行指示灯定时器周期时间
#define SYS_RUN_LED_TIMER_REPEAT		1//系统运行指示灯定时器周期运行
#define SYS_LED_ON() 					GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0)//GPIO4输出使能，并输出低电平
#define SYS_LED_OFF() 					GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1)//GPIO4输出使能，并输出高电平
#define SYS_LED_PIN_NAME 				PERIPHS_IO_MUX_GPIO4_U//SYS LED引脚寄存器
#define SYS_LED_FUNC	 				FUNC_GPIO4//SYS LED引脚功能
#define LED_ON   1						//LED 亮状态
#define LED_OFF  0						//LED 灭状态
/******************************************************************************
 * FunctionName : sys_run_led
 * Description  : 系统运行指示灯运行，根据sys_led_status状态设置led亮灭
 * Parameters   : parg  sys_led_status变量地址
 * Returns      : none
*******************************************************************************/
static void ICACHE_FLASH_ATTR
sys_run_led(void *parg) {
	uint8 *led_status = (uint8*) parg;
	(LED_ON == (*led_status)) ? SYS_LED_OFF() : SYS_LED_ON();
	(*led_status) = (LED_ON == (*led_status)) ? LED_OFF : LED_ON;
//	os_printf("\r\n-sys running!!!--\r\n");
}
/******************************************************************************
 * FunctionName : sys_run_led_init
 * Description  : 系统运行指示灯初始化
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
sys_run_led_init(void) {
	PIN_FUNC_SELECT(SYS_LED_PIN_NAME, SYS_LED_FUNC);		//LED IO配置成GPIO功能
	SYS_LED_OFF();			//关闭LED灯
	sys_led_status = LED_OFF;			//灯状态为关闭

	os_timer_disarm(&sys_run_led_timer);			//取消软件定时器定时
	os_timer_setfn(&sys_run_led_timer, sys_run_led, &sys_led_status);//设置软件定时器的回调函数和回调函数的参数
	os_timer_arm(&sys_run_led_timer, SYS_RUN_LED_TIMER_PERIOD_MS,
			SYS_RUN_LED_TIMER_REPEAT);			//使能毫秒定时器

}

