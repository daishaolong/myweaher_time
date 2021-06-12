
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "driver/ui.h"
#include "driver/oled.h"
#include "driver/esp8266_sntp.h"
#include "driver/tcp_client.h"
#include "driver/myhttp.h"
#include "stdio.h"


//��̬ȫ�ֱ���
static os_timer_t ui_timer;	//ϵͳָʾ�����ж�ʱ��

//�궨��
#define UI_DISPLAY_PERIOD_MS  	1000//��ʱ����
#define UI_DISPLAY_TIMER_REPEAT		1//�ظ�����
#define WEATHER_DISPLAY_PERIOD_MS (60*1000)//������ʾ����
/******************************************************************************
 * FunctionName : ui_display
 * Description  : ui��ʾ
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
		system_soft_wdt_feed();			// ι��(��ֹESP8266��λ)
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
 * Description  : ������ʾui
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR ui_display_start(void) {
	os_timer_disarm(&ui_timer);			//ȡ�������ʱ����ʱ
	os_timer_setfn(&ui_timer, ui_display, NULL);		//���������ʱ���Ļص������ͻص������Ĳ���
	os_timer_arm(&ui_timer, UI_DISPLAY_PERIOD_MS, UI_DISPLAY_TIMER_REPEAT);	//ʹ�ܺ��붨ʱ��

}
/******************************************************************************
 * FunctionName : ui_display_start
 * Description  : ֹͣ��ʾui
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR ui_display_stop(void) {
	os_timer_disarm(&ui_timer);			//ȡ�������ʱ����ʱ
}

