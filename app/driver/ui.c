
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


//��̬ȫ�ֱ���
static os_timer_t ui_timer;
static os_timer_t display_weather_timer;
static os_timer_t display_date_time_timer;

//�궨��
#define UI_DISPLAY_PERIOD_MS  	1000//��ʱ����
#define UI_DISPLAY_TIMER_REPEAT		10//�ظ�����
#define WEATHER_DISPLAY_PERIOD_MS (8*1000)//������ʾ����

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
static const hanzi_t code0[]={HANZI_QING};//��
static const hanzi_t code1[]={HANZI_QING};//��
static const hanzi_t code2[]={HANZI_QING};//��
static const hanzi_t code3[]={HANZI_QING};//��
static const hanzi_t code4[]={HANZI_DUO,HANZI_YUN};//����
static const hanzi_t code5[]={HANZI_QING,HANZI_JIAN,HANZI_DUO,HANZI_YUN};//������
static const hanzi_t code6[]={HANZI_QING,HANZI_JIAN,HANZI_DUO,HANZI_YUN};//������
static const hanzi_t code7[]={HANZI_DA,HANZI_BU,HANZI_DUO,HANZI_YUN};//�󲿶���
static const hanzi_t code8[]={HANZI_DA,HANZI_BU,HANZI_DUO,HANZI_YUN};//�󲿶���
static const hanzi_t code9[]={HANZI_YIN};//��
static const hanzi_t code10[]={HANZI_ZHEN1,HANZI_YU};//����
static const hanzi_t code11[]={HANZI_LEI,HANZI_ZHEN1,HANZI_YU};//������
static const hanzi_t code12[]={HANZI_BING,HANZI_BAO1};//����
static const hanzi_t code13[]={HANZI_XIAO,HANZI_YU};//С��
static const hanzi_t code14[]={HANZI_ZHONG,HANZI_YU};//����
static const hanzi_t code15[]={HANZI_DA,HANZI_YU};//����
static const hanzi_t code16[]={HANZI_BAO,HANZI_YU};//����
static const hanzi_t code17[]={HANZI_DA,HANZI_BAO,HANZI_YU};//����
static const hanzi_t code18[]={HANZI_TE,HANZI_DA,HANZI_BAO,HANZI_YU};//�ش���
static const hanzi_t code19[]={HANZI_DONG,HANZI_YU};//����
static const hanzi_t code20[]={HANZI_YU,HANZI_JIA,HANZI_XUE};//���ѩ
static const hanzi_t code21[]={HANZI_ZHEN1,HANZI_XUE};//��ѩ
static const hanzi_t code22[]={HANZI_XIAO,HANZI_XUE};//Сѩ
static const hanzi_t code23[]={HANZI_ZHONG,HANZI_XUE};//��ѩ
static const hanzi_t code24[]={HANZI_DA,HANZI_XUE};//��ѩ
static const hanzi_t code25[]={HANZI_BAO,HANZI_XUE};//��ѩ
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
//�������������ȡ��ʾ������ʾ��Ϣ
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
//��ʾ����
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
	OLED_ShowHanzi(3*16,0,days[index],sizeof(days[index])/sizeof(days[index][0]));//����

	os_sprintf(buf,":%s-%s",weather[index].low,weather[index].high);//�¶�
	hanzi_t wendu[]={HANZI_WEN,HANZI_DU};
	OLED_ShowHanzi(0,2,wendu,sizeof(wendu)/sizeof(wendu[0]));
	x=sizeof(wendu)/sizeof(wendu[0])*16;
	OLED_ShowString(x,2,buf);
	x+=strlen(buf)*8;
	OLED_ShowOneHanzi(x,2,HANZI_SHESHIDU);

//		OLED_ShowString(0,4,weather[index].text_day);//����
	hanzi_t tianqi[]={HANZI_TIAN,HANZI_QI1};
	OLED_ShowHanzi(0,4,tianqi,sizeof(tianqi)/sizeof(tianqi[0]));
	x=sizeof(tianqi)/sizeof(tianqi[0])*16;
	OLED_ShowString(x,4,":");
	x+=8;
	get_info_by_code(weather[index].code_day,&hanzi,&out_len);
	OLED_ShowHanzi(x,4,hanzi,out_len);

	os_sprintf(buf,":%s%",weather[index].humidity);//ʪ��
	hanzi_t shidu[]={HANZI_SHI,HANZI_DU};
	OLED_ShowHanzi(0,6,shidu,sizeof(shidu)/sizeof(shidu[0]));
	x=sizeof(shidu)/sizeof(shidu[0])*16;
	OLED_ShowString(x,6,buf);
}
//��ʾʱ��
static void ICACHE_FLASH_ATTR display_date_time(void)
{
	mytime_t * real_time= get_real_time();
	if(strlen(real_time->time)==0)
	{
		return ;
	}
	const hanzi_t arr[]={HANZI_SHEN,HANZI_ZHEN};
//			OLED_ShowHanzi(3*16,0,arr,sizeof(arr)/sizeof(arr[0]));
	OLED_ShowHanzi_LineCenter(0,arr,sizeof(arr)/sizeof(arr[0]));//����
	OLED_ShowString_LineCenter(2, real_time->time);//ʱ��
	OLED_ShowString_LineCenter(4, real_time->date);//����
	s8 index=get_week_index(real_time->week);//����
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
//	os_timer_disarm(&display_weather_timer);			//ȡ�������ʱ����ʱ
//	os_timer_setfn(&display_weather_timer, display_weather, NULL);		//���������ʱ���Ļص������ͻص������Ĳ���
//	os_timer_arm(&display_weather_timer, 3000, 1);	//ʹ�ܺ��붨ʱ��
//}
//static void ICACHE_FLASH_ATTR display_weather_stop(void)
//{
//	os_timer_disarm(&display_weather_timer);			//ȡ�������ʱ����ʱ
//}
//static void ICACHE_FLASH_ATTR display_date_time_start(void)
//{
//	os_timer_disarm(&display_date_time_timer);			//ȡ�������ʱ����ʱ
//	os_timer_setfn(&display_date_time_timer, display_date_time, NULL);		//���������ʱ���Ļص������ͻص������Ĳ���
//	os_timer_arm(&display_date_time_timer, 1000, 1);	//ʹ�ܺ��붨ʱ��
//}
//static void ICACHE_FLASH_ATTR display_date_time_stop(void)
//{
//	os_timer_disarm(&display_date_time_timer);			//ȡ�������ʱ����ʱ
//}
/******************************************************************************
 * FunctionName : ui_display
 * Description  : ui��ʾ
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

		system_soft_wdt_feed();			// ι��(��ֹESP8266��λ)
	}
	else
	{
		display_date_time();
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
	os_timer_disarm(&display_weather_timer);			//ȡ�������ʱ����ʱ
	os_timer_disarm(&display_date_time_timer);			//ȡ�������ʱ����ʱ
}

