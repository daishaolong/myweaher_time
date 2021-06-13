#include "driver/esp8266_station.h"
#include "user_interface.h"
#include "smartconfig.h"
#include "string.h"
#include "osapi.h"

#include "driver/esp8266_sntp.h"
#include "driver/oled.h"
#include "driver/ui.h"

static station_connected_wifi_callback station_connected_wifi_cb = NULL; //�ɹ�����wifi�Ļص�����--tcp/udp�������Ӻ���
static os_timer_t timer_wifi_check;			// ���������ʱ��

//ע��sta�ɹ�����wifi�Ļص�����
void station_regist_connnetcd_cb(station_connected_wifi_callback connect_cb) {
	station_connected_wifi_cb = connect_cb;
}
// �����ʱ�Ļص�����
//=========================================================================================================
static void ICACHE_FLASH_ATTR timer_wifi_check_cb(void) {
	struct ip_info ST_ESP8266_IP;	// ESP8266��IP��Ϣ
	u8 ESP8266_IP[4];				// ESP8266��IP��ַ
	u8 connect_status;				//wifi����״̬
	// �ɹ�����WIFI��STAģʽ�£��������DHCP(Ĭ��)����ESO8266��IP��ַ��WIFI·�����Զ����䡿
	connect_status = wifi_station_get_connect_status();
	//-------------------------------------------------------------------------------------
	if (STATION_GOT_IP == connect_status)	// �ж��Ƿ��ȡIP
			{
		wifi_get_ip_info(STATION_IF, &ST_ESP8266_IP);	// ��ȡSTA��IP��Ϣ
		ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;			// IP��ַ�߰�λ == addr�Ͱ�λ
		ESP8266_IP[1] = ST_ESP8266_IP.ip.addr >> 8;		// IP��ַ�θ߰�λ == addr�εͰ�λ
		ESP8266_IP[2] = ST_ESP8266_IP.ip.addr >> 16;	// IP��ַ�εͰ�λ == addr�θ߰�λ
		ESP8266_IP[3] = ST_ESP8266_IP.ip.addr >> 24;	// IP��ַ�Ͱ�λ == addr�߰�λ

		// ��ʾESP8266��IP��ַ
		//-----------------------------------------------------------------------------------------------
		os_printf("ESP8266_IP = %d.%d.%d.%d\n", ESP8266_IP[0], ESP8266_IP[1],
				ESP8266_IP[2], ESP8266_IP[3]);
		OLED_Clear();
		OLED_ShowString(0, 0, "wifi connected ");
		OLED_ShowString(0, 2, "ESP8266_IP:");
		OLED_ShowIP(0, 4, ESP8266_IP);
		OLED_ShowString(0, 6, "Please wait...");
		system_soft_wdt_feed();
		delay_ms(400);
		system_soft_wdt_feed();
		os_timer_disarm(&timer_wifi_check);	// �رն�ʱ��

		if (station_connected_wifi_cb != NULL) {
			station_connected_wifi_cb();	//ִ��wifi���ӻص�����--tcp/udp�������Ӻ���
		}
		esp8266_sntp_init();
		ui_display_start();
	} else if (STATION_WRONG_PASSWORD == connect_status
			|| STATION_NO_AP_FOUND == connect_status
			|| STATION_CONNECT_FAIL == connect_status) {
		os_printf("STATION CONNECT FAIL,reson:\r\n");
//		OLED_Clear();
		OLED_ShowString(0, 0, "Connected Fial:  ");
		if (STATION_CONNECT_FAIL == connect_status) {
			os_printf("STATION_CONNECT_FAIL\r\n");
			OLED_ShowString(0, 2, "STATION_CONNECT_FAIL");
		}
		if (STATION_WRONG_PASSWORD == connect_status) {
			os_printf("STATION_WRONG_PASSWORD\r\n");
			OLED_ShowString(0, 2, "STATION_WRONG_PASSWORD");
		}
		if (STATION_NO_AP_FOUND == connect_status) {
			os_printf("STATION_NO_AP_FOUND\r\n");
			OLED_ShowString(0, 2, "STATION_NO_AP_FOUND");
		}

	} else if (STATION_CONNECTING == connect_status) {
		static u8 cnt = 0;
		OLED_ShowString(0 + cnt * 8+strlen("connecting .")*8, 2, ".");
		cnt++;
	}
}
// �����ʱ����ʼ��(ms����)
//=====================================================================================
static void ICACHE_FLASH_ATTR timer_wifi_check_init(u32 time_ms,
		u8 time_repetitive) {
	struct	station_config	config;
	wifi_station_get_config_default(&config);
	OLED_ShowString(0, 0, "wifi:");
	OLED_ShowString(0+strlen("wifi:")*8, 0, config.ssid);
	OLED_ShowString(0, 2, "connecting .");
	os_timer_disarm(&timer_wifi_check);	// �رն�ʱ��
	os_timer_setfn(&timer_wifi_check, (os_timer_func_t *) timer_wifi_check_cb,
			NULL);	// ���ö�ʱ��
	os_timer_arm(&timer_wifi_check, time_ms, time_repetitive);  // ʹ�ܶ�ʱ��
}
#if MYSMART_LINK_ENABLE

//---------------------------���������궨��--------------------------------
#define SMARTLINK_KEY_PIN_NAME 		PERIPHS_IO_MUX_MTDI_U//���������������żĴ���
#define SMARTLINK_KEY_FUNC	 		FUNC_GPIO12//���������������Ź���
#define SMARTLINK_KEY_GPIO_ID_PIN   	12//������������IO��ID
#define SMARTLINK_KEY_READ	GPIO_INPUT_GET(GPIO_ID_PIN(SMARTLINK_KEY_GPIO_ID_PIN))//��ȡ������������״̬
#define SMARTLINK_KEY_PRESS 			0 //����������������


//----------------------------��̬����--------------------------------
//��������������ʼ��
static void ICACHE_FLASH_ATTR smartconfig_key_init(void) {
	PIN_FUNC_SELECT(SMARTLINK_KEY_PIN_NAME, SMARTLINK_KEY_FUNC); //�������������������ó�GPIO����
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(SMARTLINK_KEY_GPIO_ID_PIN));//����ģʽ
	PIN_PULLUP_EN(SMARTLINK_KEY_PIN_NAME);//ʹ������
}


//�����������ӹ��̻ص�����
static void ICACHE_FLASH_ATTR smart_link_cb(sc_status status, void *pdata) {
	switch (status) {
		case SC_STATUS_WAIT:		//����δ��ʼ�������ڴ˽׶ο�ʼ����
		os_printf("SC_STATUS_WAIT, Do not open app smartlink\r\n");
		break;
		case SC_STATUS_FIND_CHANNEL://���ڴ˽׶ο���APP�����������
		os_printf("SC_STATUS_FIND_CHANNEL,open app smartlink\r\n");
		OLED_ShowString(0,2,"use weixin config ...");
		break;
		case SC_STATUS_GETTING_SSID_PSWD://��ȡ��wifi��������
		os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
		OLED_ShowString(0,2,"getting SSID PSWD ....");
		sc_type *type = pdata;//��ӡ��ȡwifi��������ķ�ʽ
		if (*type == SC_TYPE_ESPTOUCH) {
			os_printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");		//ESPTOUCH�������
		} else {
			os_printf("SC_TYPE:SC_TYPE_AIRKISS\n");		//΢��С������
		}
		break;
		case SC_STATUS_LINK:		//��������·��
		os_printf("SC_STATUS_LINK\n");
		OLED_ShowString(0,2,"connecting to wifi ....");
		OLED_ShowString(0, 6, "please wait...");
		struct station_config *sta_conf = pdata;
		wifi_station_set_config(sta_conf);
		wifi_station_disconnect();
		wifi_station_connect();
		break;
		case SC_STATUS_LINK_OVER://��ȡ��IP������·�����
		os_printf("SC_STATUS_LINK_OVER\n");
		if (pdata != NULL) {
			uint8 phone_ip[4] = {0};
			memcpy(phone_ip, (uint8*) pdata, 4);
			os_printf("Phone	ip:	%d.%d.%d.%d\n", phone_ip[0], phone_ip[1],
					phone_ip[2], phone_ip[3]);
		}
		uint8 esp8266_ip[4];
		struct ip_info info;
		wifi_get_ip_info(STATION_IF,&info);
		os_memcpy(esp8266_ip,&info.ip,sizeof(esp8266_ip));
		os_printf("esp8266	ip:	%d.%d.%d.%d\n", esp8266_ip[0], esp8266_ip[1],
				esp8266_ip[2], esp8266_ip[3]);
		smartconfig_stop();		//�ر���������
		os_printf("smart link end!!!!!\r\n");
		OLED_ShowString(0, 0, "wifi connected  ");
		OLED_ShowString(0, 2, "ESP8266_IP:     ");
		OLED_ShowIP(0, 4, esp8266_ip);
		OLED_ShowString(0, 6, "Please wait...  ");
		system_soft_wdt_feed();
		delay_ms(400);
		system_soft_wdt_feed();
//		os_printf("system restart\r\n");
//		system_restart();//ϵͳ����
		if(station_connected_wifi_cb!=NULL)
		{
			station_connected_wifi_cb();		//ִ��wifi���ӻص�����--tcp/udp�������Ӻ���
		}
		esp8266_sntp_init();
		ui_display_start();
		break;
		default:
		break;
	}
}
//����������ʼ������
static void ICACHE_FLASH_ATTR mysmart_link_init(void) {
	smartconfig_set_type(SC_TYPE_AIRKISS);	//������������ģʽ:ESPTOUCH  SC_TYPE_AIRKISS
//	smartconfig_stop();//smartconfig_startδ���֮ǰ�����ظ�ִ��smartconfig_start,���ȵ���smartconfig_stop�������ο���
	smartconfig_start(smart_link_cb, 0);//��ʼ���������������ûص�������1����ʾ��ӡ�������ӹ��̵���Ϣ
	//smartconfig�����в�Ҫ��������API��������smartlink�����󣬵���smartconfig_stop�������ٵ�������API����
}

//--------------------------------ȫ�ֺ���-----------------------------
//��������������
static void ICACHE_FLASH_ATTR mysmart_link(void) {
	struct station_config config;
	smartconfig_key_init();	//������������������ʼ��
	wifi_set_opmode(STATION_MODE);//����WIFIģʽ:STAģʽ
	os_memset(&config,0,sizeof(config));
	wifi_station_get_config_default(&config);//��ȡ������flash�����ò���
	if (SMARTLINK_KEY_PRESS == SMARTLINK_KEY_READ || 0==os_strlen(config.ssid)||
			0==os_strlen(config.password))//���������������� ����wifi�� /����Ϊ��
	{
		os_printf("smart link start!!!!!\r\n");
		OLED_ShowString(0,0,"  wifi config ");
		OLED_ShowString(0, 2, "please wait...");
		mysmart_link_init();	//��ʼ����������
	} else {
		os_printf("no smart link,connnect Ap\r\n");
		os_printf("ssid:%s\r\n",config.ssid);
		os_printf("password:%s\r\n",config.password);
//		wifi_set_event_handler_cb(wifi_event_handler_cb);	//ע��wifi�¼��ص�����
		timer_wifi_check_init(1000, 1);	//��ʱ��ѯwifi����״̬
	}
}
void ICACHE_FLASH_ATTR esp8266_station_init(void)
{
	mysmart_link();
}

#else  //MYSMART_LINK_ENABLE

//---------------------������������ֱ�ӵ�STATIONģʽ����----------------------


#define		ESP8266_STA_SSID	"1403"					// �����WIFI��
#define		ESP8266_STA_PASS	"2009zhangyanyan"		// �����WIFI����
// ESP8266_STA��ʼ��
//==============================================================================
void ICACHE_FLASH_ATTR esp8266_station_init(void) {
	struct station_config STA_Config;	// STA�����ṹ��

	struct ip_info ST_ESP8266_IP;		// STA��Ϣ�ṹ��

	// ����ESP8266�Ĺ���ģʽ
	//------------------------------------------------------------------------
	wifi_set_opmode(STATION_MODE);				// ����ΪSTAģʽ�������浽Flash

	/*
	 // ����STAģʽ�µ�IP��ַ��ESP8266Ĭ�Ͽ���DHCP Client������WIFIʱ���Զ�����IP��ַ��
	 //--------------------------------------------------------------------------------
	 wifi_station_dhcpc_stop();						// �ر� DHCP Client
	 IP4_ADDR(&ST_ESP8266_IP.ip,192,168,8,88);		// ����IP��ַ
	 IP4_ADDR(&ST_ESP8266_IP.gw,192,168,8,1);		// �������ص�ַ
	 IP4_ADDR(&ST_ESP8266_IP.netmask,255,255,255,0);	// ������������
	 wifi_set_ip_info(STATION_IF,&ST_ESP8266_IP);	// ����STAģʽ�µ�IP��ַ
	 */

	// �ṹ�帳ֵ������STAģʽ����
	//-------------------------------------------------------------------------------
	os_memset(&STA_Config, 0, sizeof(struct station_config));	// STA�����ṹ�� = 0
	os_strcpy(STA_Config.ssid, ESP8266_STA_SSID);				// ����WIFI��
	os_strcpy(STA_Config.password, ESP8266_STA_PASS);			// ����WIFI����

	wifi_station_set_config(&STA_Config);	// ����STA�����������浽Flash

	// wifi_station_connect();		// ESP8266����WIFI  -��user_init �е��ò��ô˺���
	timer_wifi_check_init(1000, 1);	//��ʱ��ѯwifi����״̬
}

#endif //MYSMART_LINK_ENABLE

