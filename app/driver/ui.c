
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
//
static const hanzi_t code0[]={HANZI_QING};//晴
static const hanzi_t code1[]={HANZI_QING};//晴
static const hanzi_t code2[]={HANZI_QING};//晴
static const hanzi_t code3[]={HANZI_QING};//晴
static const hanzi_t code4[]={HANZI_DUO,HANZI_YUN};//多云
static const hanzi_t code5[]={HANZI_QING,HANZI_JIAN,HANZI_DUO,HANZI_YUN};//晴间多云
static const hanzi_t code6[]={HANZI_QING,HANZI_JIAN,HANZI_DUO,HANZI_YUN};//晴间多云
static const hanzi_t code7[]={HANZI_DA,HANZI_BU,HANZI_DUO,HANZI_YUN};//大部多云
static const hanzi_t code8[]={HANZI_DA,HANZI_BU,HANZI_DUO,HANZI_YUN};//大部多云
static const hanzi_t code9[]={HANZI_YIN};//阴
static const hanzi_t code10[]={HANZI_ZHEN1,HANZI_YU};//阵雨
static const hanzi_t code11[]={HANZI_LEI,HANZI_ZHEN1,HANZI_YU};//雷阵雨
static const hanzi_t code12[]={HANZI_BING,HANZI_BAO1};//冰雹
static const hanzi_t code13[]={HANZI_XIAO,HANZI_YU};//小雨
static const hanzi_t code14[]={HANZI_ZHONG,HANZI_YU};//中雨
static const hanzi_t code15[]={HANZI_DA,HANZI_YU};//大雨
static const hanzi_t code16[]={HANZI_BAO,HANZI_YU};//暴雨
static const hanzi_t code17[]={HANZI_DA,HANZI_BAO,HANZI_YU};//大暴雨
static const hanzi_t code18[]={HANZI_TE,HANZI_DA,HANZI_BAO,HANZI_YU};//特大暴雨
static const hanzi_t code19[]={HANZI_DONG,HANZI_YU};//冻雨
static const hanzi_t code20[]={HANZI_YU,HANZI_JIA,HANZI_XUE};//雨夹雪
static const hanzi_t code21[]={HANZI_ZHEN1,HANZI_XUE};//阵雪
static const hanzi_t code22[]={HANZI_XIAO,HANZI_XUE};//小雪
static const hanzi_t code23[]={HANZI_ZHONG,HANZI_XUE};//中雪
static const hanzi_t code24[]={HANZI_DA,HANZI_XUE};//大雪
static const hanzi_t code25[]={HANZI_BAO,HANZI_XUE};//暴雪
typedef struct
{
	const hanzi_t * addr;
	u16 len;
}code_info_t;
static const code_info_t code_info[]={
		{code0,sizeof(code0)/sizeof(code0[0])},
		{code1,sizeof(code1)/sizeof(code1[0])},
		{code2,sizeof(code2)/sizeof(code2[0])},
		{code3,sizeof(code3)/sizeof(code3[0])},
		{code4,sizeof(code4)/sizeof(code4[0])},
		{code5,sizeof(code5)/sizeof(code5[0])},
		{code6,sizeof(code6)/sizeof(code6[0])},
		{code7,sizeof(code7)/sizeof(code7[0])},
		{code8,sizeof(code8)/sizeof(code8[0])},
		{code9,sizeof(code9)/sizeof(code9[0])},
		{code10,sizeof(code10)/sizeof(code10[0])},
		{code11,sizeof(code11)/sizeof(code11[0])},
		{code12,sizeof(code12)/sizeof(code12[0])},
		{code13,sizeof(code13)/sizeof(code13[0])},
		{code14,sizeof(code14)/sizeof(code14[0])},
		{code15,sizeof(code15)/sizeof(code15[0])},
		{code16,sizeof(code16)/sizeof(code16[0])},
		{code17,sizeof(code17)/sizeof(code17[0])},
		{code18,sizeof(code18)/sizeof(code18[0])},
		{code19,sizeof(code19)/sizeof(code19[0])},
		{code20,sizeof(code20)/sizeof(code20[0])},
		{code21,sizeof(code21)/sizeof(code21[0])},
		{code22,sizeof(code22)/sizeof(code22[0])},
		{code23,sizeof(code23)/sizeof(code23[0])},
		{code24,sizeof(code24)/sizeof(code24[0])},
		{code25,sizeof(code25)/sizeof(code25[0])},
};
#define CODE_INFO_SIZE (sizeof(code_info)/sizeof(code_info[0]))
//由天气气象码获取提示汉字提示信息
static void ICACHE_FLASH_ATTR get_info_by_code(const char *code,const hanzi_t **out,u16* out_len)
{
	s32 num=atoi(code);
	if(num<0 || num>=CODE_INFO_SIZE)
	{
		(*out_len)=0;
		return ;
	}

	(*out)=code_info[num].addr;
	(*out_len)=code_info[num].len;

//	os_printf("code:%s,%d;out_len:%d\r\n",code,num,*out_len);
}
//显示天气
static void ICACHE_FLASH_ATTR display_weather(u8 index)
{
	u8 x;
	u16 out_len;
	const hanzi_t *hanzi;
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
	OLED_ShowString(x,4,":");
	x+=8;
	get_info_by_code(weather[index].code_day,&hanzi,&out_len);
	OLED_ShowHanzi(x,4,hanzi,out_len);

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

