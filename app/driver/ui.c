
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "driver/ui.h"
#include "driver/oled.h"
#include "driver/esp8266_sntp.h"
#include "driver/tcp_client.h"
#include "driver/myhttp.h"
#include "stdio.h"


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
	char buf[16];
	static uint16_t cnt = 0;
	static uint16_t day_index=0;
	if (cnt >= (WEATHER_DISPLAY_PERIOD_MS / UI_DISPLAY_PERIOD_MS)) {
		char* days[]={"111","222","333"};
		weather_t* weather=get_weather_info();
		if(strlen(weather[day_index].high)==0)
		{
			return ;
		}
		OLED_Clear();
		OLED_ShowString(0, 0, days[day_index]);
		os_sprintf(buf,"%s-%s",weather[day_index].low,weather[day_index].high);
		OLED_ShowString(0,2,buf);
		OLED_ShowString(0,4,weather[day_index].text_day);
		os_sprintf(buf,"%s%",weather[day_index].humidity);
		OLED_ShowString(0,6,buf);
		day_index=(day_index+1)%WEATHER_DAYS;

		cnt=(0==day_index)?0:cnt;
		system_soft_wdt_feed();			// 喂狗(防止ESP8266复位)
	}
	else
	{
		mytime_t * real_time= get_real_time();
		if(strlen(real_time->time)!=0)
		{
			const hanzi_t arr[]={HANZI_SHEN,HANZI_ZHEN,	HANZI_JIN,
					HANZI_MING,
					HANZI_HOU,
					HANZI_TIAN,};
			OLED_ShowHanzi(0,0,arr,sizeof(arr)/sizeof(arr[0]));
			OLED_ShowString_LineCenter(2, real_time->time);
			OLED_ShowString_LineCenter(4, real_time->date);
			OLED_ShowString_LineCenter(6, real_time->week);
		}
	}
	cnt++;
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

