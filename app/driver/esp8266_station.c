#include "driver/esp8266_station.h"
#include "user_interface.h"
#include "smartconfig.h"
#include "string.h"
#include "osapi.h"

#include "driver/esp8266_sntp.h"
#include "driver/oled.h"
#include "driver/ui.h"

static station_connected_wifi_callback station_connected_wifi_cb = NULL; //成功连接wifi的回调函数--tcp/udp网络连接函数
static os_timer_t timer_wifi_check;			// 定义软件定时器

//注册sta成功连接wifi的回调函数
void station_regist_connnetcd_cb(station_connected_wifi_callback connect_cb) {
	station_connected_wifi_cb = connect_cb;
}
// 软件定时的回调函数
//=========================================================================================================
static void ICACHE_FLASH_ATTR timer_wifi_check_cb(void) {
	struct ip_info ST_ESP8266_IP;	// ESP8266的IP信息
	u8 ESP8266_IP[4];				// ESP8266的IP地址
	u8 connect_status;				//wifi连接状态
	// 成功接入WIFI【STA模式下，如果开启DHCP(默认)，则ESO8266的IP地址由WIFI路由器自动分配】
	connect_status = wifi_station_get_connect_status();
	//-------------------------------------------------------------------------------------
	if (STATION_GOT_IP == connect_status)	// 判断是否获取IP
			{
		wifi_get_ip_info(STATION_IF, &ST_ESP8266_IP);	// 获取STA的IP信息
		ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;			// IP地址高八位 == addr低八位
		ESP8266_IP[1] = ST_ESP8266_IP.ip.addr >> 8;		// IP地址次高八位 == addr次低八位
		ESP8266_IP[2] = ST_ESP8266_IP.ip.addr >> 16;	// IP地址次低八位 == addr次高八位
		ESP8266_IP[3] = ST_ESP8266_IP.ip.addr >> 24;	// IP地址低八位 == addr高八位

		// 显示ESP8266的IP地址
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
		os_timer_disarm(&timer_wifi_check);	// 关闭定时器

		if (station_connected_wifi_cb != NULL) {
			station_connected_wifi_cb();	//执行wifi连接回调函数--tcp/udp网络连接函数
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
// 软件定时器初始化(ms毫秒)
//=====================================================================================
static void ICACHE_FLASH_ATTR timer_wifi_check_init(u32 time_ms,
		u8 time_repetitive) {
	struct	station_config	config;
	wifi_station_get_config_default(&config);
	OLED_ShowString(0, 0, "wifi:");
	OLED_ShowString(0+strlen("wifi:")*8, 0, config.ssid);
	OLED_ShowString(0, 2, "connecting .");
	os_timer_disarm(&timer_wifi_check);	// 关闭定时器
	os_timer_setfn(&timer_wifi_check, (os_timer_func_t *) timer_wifi_check_cb,
			NULL);	// 设置定时器
	os_timer_arm(&timer_wifi_check, time_ms, time_repetitive);  // 使能定时器
}
#if MYSMART_LINK_ENABLE

//---------------------------智能配网宏定义--------------------------------
#define SMARTLINK_KEY_PIN_NAME 		PERIPHS_IO_MUX_MTDI_U//智能配网按键引脚寄存器
#define SMARTLINK_KEY_FUNC	 		FUNC_GPIO12//智能配网按键引脚功能
#define SMARTLINK_KEY_GPIO_ID_PIN   	12//智能配网按键IO口ID
#define SMARTLINK_KEY_READ	GPIO_INPUT_GET(GPIO_ID_PIN(SMARTLINK_KEY_GPIO_ID_PIN))//读取智能配网按键状态
#define SMARTLINK_KEY_PRESS 			0 //智能配网按键按下


//----------------------------静态函数--------------------------------
//智能配网按键初始化
static void ICACHE_FLASH_ATTR smartconfig_key_init(void) {
	PIN_FUNC_SELECT(SMARTLINK_KEY_PIN_NAME, SMARTLINK_KEY_FUNC); //智能配网按键引脚配置成GPIO功能
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(SMARTLINK_KEY_GPIO_ID_PIN));//输入模式
	PIN_PULLUP_EN(SMARTLINK_KEY_PIN_NAME);//使能上拉
}


//智能配网连接过程回调函数
static void ICACHE_FLASH_ATTR smart_link_cb(sc_status status, void *pdata) {
	switch (status) {
		case SC_STATUS_WAIT:		//连接未开始，请勿在此阶段开始连接
		os_printf("SC_STATUS_WAIT, Do not open app smartlink\r\n");
		break;
		case SC_STATUS_FIND_CHANNEL://请在此阶段开启APP进行配对连接
		os_printf("SC_STATUS_FIND_CHANNEL,open app smartlink\r\n");
		OLED_ShowString(0,2,"use weixin config ...");
		break;
		case SC_STATUS_GETTING_SSID_PSWD://获取到wifi名和密码
		os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
		OLED_ShowString(0,2,"getting SSID PSWD ....");
		sc_type *type = pdata;//打印获取wifi名和密码的方式
		if (*type == SC_TYPE_ESPTOUCH) {
			os_printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");		//ESPTOUCH软件发送
		} else {
			os_printf("SC_TYPE:SC_TYPE_AIRKISS\n");		//微信小程序发送
		}
		break;
		case SC_STATUS_LINK:		//尝试连接路由
		os_printf("SC_STATUS_LINK\n");
		OLED_ShowString(0,2,"connecting to wifi ....");
		OLED_ShowString(0, 6, "please wait...");
		struct station_config *sta_conf = pdata;
		wifi_station_set_config(sta_conf);
		wifi_station_disconnect();
		wifi_station_connect();
		break;
		case SC_STATUS_LINK_OVER://获取到IP，连接路由完成
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
		smartconfig_stop();		//关闭智能配网
		os_printf("smart link end!!!!!\r\n");
		OLED_ShowString(0, 0, "wifi connected  ");
		OLED_ShowString(0, 2, "ESP8266_IP:     ");
		OLED_ShowIP(0, 4, esp8266_ip);
		OLED_ShowString(0, 6, "Please wait...  ");
		system_soft_wdt_feed();
		delay_ms(400);
		system_soft_wdt_feed();
//		os_printf("system restart\r\n");
//		system_restart();//系统重启
		if(station_connected_wifi_cb!=NULL)
		{
			station_connected_wifi_cb();		//执行wifi连接回调函数--tcp/udp网络连接函数
		}
		esp8266_sntp_init();
		ui_display_start();
		break;
		default:
		break;
	}
}
//智能配网初始化函数
static void ICACHE_FLASH_ATTR mysmart_link_init(void) {
	smartconfig_set_type(SC_TYPE_AIRKISS);	//设置智能配网模式:ESPTOUCH  SC_TYPE_AIRKISS
//	smartconfig_stop();//smartconfig_start未完成之前不可重复执行smartconfig_start,请先调用smartconfig_stop结束本次快连
	smartconfig_start(smart_link_cb, 0);//开始智能配网，并设置回调函数；1：表示打印配网连接过程的信息
	//smartconfig过程中不要调用其他API函数，当smartlink结束后，调用smartconfig_stop函数后再调用其他API函数
}

//--------------------------------全局函数-----------------------------
//智能配网处理函数
static void ICACHE_FLASH_ATTR mysmart_link(void) {
	struct station_config config;
	smartconfig_key_init();	//智能配网触发按键初始化
	wifi_set_opmode(STATION_MODE);//设置WIFI模式:STA模式
	os_memset(&config,0,sizeof(config));
	wifi_station_get_config_default(&config);//获取保存在flash的配置参数
	if (SMARTLINK_KEY_PRESS == SMARTLINK_KEY_READ || 0==os_strlen(config.ssid)||
			0==os_strlen(config.password))//智能配网按键按下 或者wifi名 /密码为空
	{
		os_printf("smart link start!!!!!\r\n");
		OLED_ShowString(0,0,"  wifi config ");
		OLED_ShowString(0, 2, "please wait...");
		mysmart_link_init();	//初始化智能连接
	} else {
		os_printf("no smart link,connnect Ap\r\n");
		os_printf("ssid:%s\r\n",config.ssid);
		os_printf("password:%s\r\n",config.password);
//		wifi_set_event_handler_cb(wifi_event_handler_cb);	//注册wifi事件回调函数
		timer_wifi_check_init(1000, 1);	//定时查询wifi连接状态
	}
}
void ICACHE_FLASH_ATTR esp8266_station_init(void)
{
	mysmart_link();
}

#else  //MYSMART_LINK_ENABLE

//---------------------不用智能配网直接的STATION模式配置----------------------


#define		ESP8266_STA_SSID	"1403"					// 接入的WIFI名
#define		ESP8266_STA_PASS	"2009zhangyanyan"		// 接入的WIFI密码
// ESP8266_STA初始化
//==============================================================================
void ICACHE_FLASH_ATTR esp8266_station_init(void) {
	struct station_config STA_Config;	// STA参数结构体

	struct ip_info ST_ESP8266_IP;		// STA信息结构体

	// 设置ESP8266的工作模式
	//------------------------------------------------------------------------
	wifi_set_opmode(STATION_MODE);				// 设置为STA模式，并保存到Flash

	/*
	 // 设置STA模式下的IP地址【ESP8266默认开启DHCP Client，接入WIFI时会自动分配IP地址】
	 //--------------------------------------------------------------------------------
	 wifi_station_dhcpc_stop();						// 关闭 DHCP Client
	 IP4_ADDR(&ST_ESP8266_IP.ip,192,168,8,88);		// 配置IP地址
	 IP4_ADDR(&ST_ESP8266_IP.gw,192,168,8,1);		// 配置网关地址
	 IP4_ADDR(&ST_ESP8266_IP.netmask,255,255,255,0);	// 配置子网掩码
	 wifi_set_ip_info(STATION_IF,&ST_ESP8266_IP);	// 设置STA模式下的IP地址
	 */

	// 结构体赋值，配置STA模式参数
	//-------------------------------------------------------------------------------
	os_memset(&STA_Config, 0, sizeof(struct station_config));	// STA参数结构体 = 0
	os_strcpy(STA_Config.ssid, ESP8266_STA_SSID);				// 设置WIFI名
	os_strcpy(STA_Config.password, ESP8266_STA_PASS);			// 设置WIFI密码

	wifi_station_set_config(&STA_Config);	// 设置STA参数，并保存到Flash

	// wifi_station_connect();		// ESP8266连接WIFI  -在user_init 中调用不用此函数
	timer_wifi_check_init(1000, 1);	//定时查询wifi连接状态
}

#endif //MYSMART_LINK_ENABLE

