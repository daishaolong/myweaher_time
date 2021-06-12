
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "driver/ui.h"
#include "driver/oled.h"
#include "driver/esp8266_sntp.h"
#include "driver/tcp_client.h"
#include "driver/myhttp.h"
#include "stdio.h"
#include "string.h"


//静态全局变量
static os_timer_t ui_timer;
static os_timer_t display_weather_timer;
static os_timer_t display_date_time_timer;

//宏定义
#define UI_DISPLAY_PERIOD_MS  	1000//定时周期
#define UI_DISPLAY_TIMER_REPEAT		10//重复运行
#define WEATHER_DISPLAY_PERIOD_MS (8*1000)//天气显示周期

//
static const char * week_str[]={"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
static s8 ICACHE_FLASH_ATTR get_week_index(char *str)
{
	s8 i;
	for(i=0;i<sizeof(week_str)/sizeof(week_str[0]);i++)
	{
		if(strcmp(week_str[i],str)==0)
		{
			return i;
		}
	}
	return -1;
}
//显示天气
static void ICACHE_FLASH_ATTR display_weather(u8 index)
{
	u8 x;
	char buf[WEATHER_CHAR_LEN*2];
	hanzi_t days[WEATHER_DAYS][2]={{HANZI_JIN,HANZI_TIAN},{HANZI_MING,HANZI_TIAN},{HANZI_HOU,HANZI_TIAN}};
	weather_t* weather=get_weather_info();
	if(index>=WEATHER_DAYS || strlen(weather[index].high)==0)
	{
		return ;
	}
	OLED_Clear();
	OLED_ShowHanzi(3*16,0,days[index],sizeof(days[index])/sizeof(days[index][0]));//日期

	os_sprintf(buf,":%s-%s",weather[index].low,weather[index].high);//温度
	hanzi_t wendu[]={HANZI_WEN,HANZI_DU};
	OLED_ShowHanzi(0,2,wendu,sizeof(wendu)/sizeof(wendu[0]));
	x=sizeof(wendu)/sizeof(wendu[0])*16;
	OLED_ShowString(x,2,buf);
	x+=strlen(buf)*8;
	OLED_ShowOneHanzi(x,2,HANZI_SHESHIDU);

//		OLED_ShowString(0,4,weather[index].text_day);//天气
	hanzi_t tianqi[]={HANZI_TIAN,HANZI_QI1};
	OLED_ShowHanzi(0,4,tianqi,sizeof(tianqi)/sizeof(tianqi[0]));
	x=sizeof(tianqi)/sizeof(tianqi[0])*16;
	os_sprintf(buf,":%s-%s",weather[index].code_day,weather[index].code_night);//天气码
	OLED_ShowString(x,4,buf);

	os_sprintf(buf,":%s%",weather[index].humidity);//湿度
	hanzi_t shidu[]={HANZI_SHI,HANZI_DU};
	OLED_ShowHanzi(0,6,shidu,sizeof(shidu)/sizeof(shidu[0]));
	x=sizeof(shidu)/sizeof(shidu[0])*16;
	OLED_ShowString(x,6,buf);
}
//显示时钟
static void ICACHE_FLASH_ATTR display_date_time(void)
{
	mytime_t * real_time= get_real_time();
	if(strlen(real_time->time)==0)
	{
		return ;
	}
	const hanzi_t arr[]={HANZI_SHEN,HANZI_ZHEN};
//			OLED_ShowHanzi(3*16,0,arr,sizeof(arr)/sizeof(arr[0]));
	OLED_ShowHanzi_LineCenter(0,arr,sizeof(arr)/sizeof(arr[0]));//城市
	OLED_ShowString_LineCenter(2, real_time->time);//时间
	OLED_ShowString_LineCenter(4, real_time->date);//日期
	s8 index=get_week_index(real_time->week);//星期
	if(-1==index)
	{
		OLED_ShowString_LineCenter(6, "week err!");
		return ;
	}
	hanzi_t week_days[][3]={{HANZI_XING,HANZI_QI,HANZI_YI},{HANZI_XING,HANZI_QI,HANZI_ER},{HANZI_XING,HANZI_QI,HANZI_SAN},
	{HANZI_XING,HANZI_QI,HANZI_SI},{HANZI_XING,HANZI_QI,HANZI_WU},{HANZI_XING,HANZI_QI,HANZI_LIU},{HANZI_XING,HANZI_QI,HANZI_RI}};
//				OLED_ShowString_LineCenter(6, real_time->week);
//				OLED_ShowHanzi(3*16,6,week_days[index],sizeof(week_days[index])/sizeof(week_days[index][0]));
	OLED_ShowHanzi_LineCenter(6,week_days[index],sizeof(week_days[index])/sizeof(week_days[index][0]));
}
////
//static void ICACHE_FLASH_ATTR display_weather_start(void)
//{
//	os_timer_disarm(&display_weather_timer);			//取消软件定时器定时
//	os_timer_setfn(&display_weather_timer, display_weather, NULL);		//设置软件定时器的回调函数和回调函数的参数
//	os_timer_arm(&display_weather_timer, 3000, 1);	//使能毫秒定时器
//}
//static void ICACHE_FLASH_ATTR display_weather_stop(void)
//{
//	os_timer_disarm(&display_weather_timer);			//取消软件定时器定时
//}
//static void ICACHE_FLASH_ATTR display_date_time_start(void)
//{
//	os_timer_disarm(&display_date_time_timer);			//取消软件定时器定时
//	os_timer_setfn(&display_date_time_timer, display_date_time, NULL);		//设置软件定时器的回调函数和回调函数的参数
//	os_timer_arm(&display_date_time_timer, 1000, 1);	//使能毫秒定时器
//}
//static void ICACHE_FLASH_ATTR display_date_time_stop(void)
//{
//	os_timer_disarm(&display_date_time_timer);			//取消软件定时器定时
//}
/******************************************************************************
 * FunctionName : ui_display
 * Description  : ui显示
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
static void ICACHE_FLASH_ATTR ui_display(void* arg) {
	static uint16_t cnt = 0;
	static uint16_t day_index=0;
	static uint16_t times=0;

//	display_weather_stop();
//	display_date_time_start();

	if (cnt >= (WEATHER_DISPLAY_PERIOD_MS / UI_DISPLAY_PERIOD_MS)) {
		if(0==times)
		{
			display_weather(day_index);
			day_index=(day_index+1)%WEATHER_DAYS;
		}
		times++;
		if(3==times)
		{
			times=0;
			cnt=(0==day_index)?0:cnt;
		}

		system_soft_wdt_feed();			// 喂狗(防止ESP8266复位)
	}
	else
	{
		display_date_time();
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
	os_timer_disarm(&display_weather_timer);			//取消软件定时器定时
	os_timer_disarm(&display_date_time_timer);			//取消软件定时器定时
}

