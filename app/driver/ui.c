
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "driver/ui.h"
#include "driver/oled.h"
#include "driver/esp8266_sntp.h"
#include "driver/tcp_client.h"

//静态全局变量
static os_timer_t ui_timer;	//系统指示灯运行定时器

//宏定义
#define UI_DISPLAY_PERIOD_MS  	1000//定时周期
#define UI_DISPLAY_TIMER_REPEAT		1//重复运行
#define WEATHER_DISPLAY_PERIOD_MS (60*1000)//天气显示周期
/******************************************************************************
 * FunctionName : ui_display
 * Description  : ui显示
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
static void ICACHE_FLASH_ATTR ui_display(void* arg) {
//	static uint16_t cnt = 0,i;
//	if (cnt >= (WEATHER_DISPLAY_PERIOD_MS / UI_DISPLAY_PERIOD_MS)) {
//		weather_t* weather=get_weather_info();
//
//	}
//	cnt++;
//	uint16_t i;
//	weather_t* weather=get_weather_info();
//	for(i=0;i<WEATHER_DAYS;i++)
//	{
//		OLED_Clear();
//		delay_ms(1000);
//		system_soft_wdt_feed();			// 喂狗(防止ESP8266复位)
//	}
//
//	delay_ms(1000);
//	system_soft_wdt_feed();			// 喂狗(防止ESP8266复位)


	mytime_t * real_time= get_real_time();
	if(strlen(real_time->time)!=0)
	{
		os_printf("\r\nbegin time:%d\r\n",system_get_time()/1000);
		OLED_Clear();
		os_printf("\r\nclear time:%d\r\n",system_get_time()/1000);
		OLED_ShowString(0, 0, "Shenzhen");
		OLED_ShowString(0, 2, real_time->time);
		OLED_ShowString(0, 4, real_time->date);
		OLED_ShowString(0, 6, real_time->week);

		system_soft_wdt_feed();			// 喂狗(防止ESP8266复位)
		os_printf("\r\nend time:  %d\r\n",system_get_time()/1000);
	}
}
/******************************************************************************
 * FunctionName : ui_display_start
 * Description  : 开启显示ui
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR ui_display_start(void) {
	os_timer_disarm(&ui_timer);			//取消软件定时器定时
	os_timer_setfn(&ui_timer, ui_display, NULL);		//设置软件定时器的回调函数和回调函数的参数
	os_timer_arm(&ui_timer, UI_DISPLAY_PERIOD_MS, UI_DISPLAY_TIMER_REPEAT);	//使能毫秒定时器

}
/******************************************************************************
 * FunctionName : ui_display_start
 * Description  : 停止显示ui
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR ui_display_stop(void) {
	os_timer_disarm(&ui_timer);			//取消软件定时器定时
}

