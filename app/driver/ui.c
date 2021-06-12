
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "driver/ui.h"
#include "driver/oled.h"
#include "driver/esp8266_sntp.h"
#include "driver/tcp_client.h"

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
//		system_soft_wdt_feed();			// ι��(��ֹESP8266��λ)
//	}
//
//	delay_ms(1000);
//	system_soft_wdt_feed();			// ι��(��ֹESP8266��λ)


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

		system_soft_wdt_feed();			// ι��(��ֹESP8266��λ)
		os_printf("\r\nend time:  %d\r\n",system_get_time()/1000);
	}
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

