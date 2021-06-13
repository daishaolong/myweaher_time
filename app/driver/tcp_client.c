#include "driver/tcp_client.h"
#include "driver/myhttp.h"
#include "user_interface.h"
#include "osapi.h"

#include "ip_addr.h"			// 被"espconn.h"使用。在"espconn.h"开头#include"ip_addr.h"或#include"ip_addr.h"放在"espconn.h"之前
#include "espconn.h"			// TCP/UDP接口
#include "ets_sys.h"			// 回调函数
#include "mem.h"
#include "stdio.h"

//static const uint8 server_ip[4]={192,168,3,5};//服务器IP
//#define SERVER_PORT			80// 服务器端口号
static struct espconn sta_netcon; //网络连接结构体
static esp_tcp esptcp; //tcp协议结构体

#define SERVER_NAME   "api.seniverse.com"//心知天气
#define SERVER_PORT			80//HTTP端口号
static ip_addr_t server_ip; //服务器ip地址
#define CITY_STR	"Shenzhen"//城市字符串
#define MY_KEY_STR "SYN55AOrciorg-CB_"//私钥


//tcp服务器接收到数据的回调函数
static void ICACHE_FLASH_ATTR tcp_server_recv_cb(void *arg, char *pdata,
		unsigned short len) {
	os_printf("\nESP8266_WIFI_Recv:\r\n");
//	os_printf(pdata);
//	os_printf(" \r\n");
	recvice_weather_data(pdata, len);
}
//tcp服务器数据发送成功的回调函数
static void ICACHE_FLASH_ATTR tcp_server_sent_cb(void *arg) {
	os_printf("\nESP8266_WIFI_Send_OK\n");
}
//tcp服务器正常断开连接的回调函数
static void ICACHE_FLASH_ATTR tcp_server_disconnect_cb(void *arg) {
	os_printf("\nESP8266_TCP_Disconnect_OK\n");
	espconn_connect(&sta_netcon);	// 连接服务器
}
// TCP连接异常断开时的回调函数【ESP8266作为TCP_Client，连接TCP_Server失败，也会进入此函数】
//tcp异常断开的回调函数
static void ICACHE_FLASH_ATTR tcp_break_cb(void *arg, sint8 err) {
	os_printf("\nESP8266_TCP_Break,err code:%d\n", err);
	espconn_connect(&sta_netcon);	// 连接服务器
}
//tcp建立连接的回调函数
static void ICACHE_FLASH_ATTR tcp_connnect_cb(void *arg) {
	struct espconn *pesp_conn = (struct espconn *) arg;				//得到espconn
	espconn_regist_sentcb(pesp_conn, tcp_server_sent_cb);	// 注册网络数据发送成功的回调函数
	espconn_regist_recvcb(pesp_conn, tcp_server_recv_cb);	// 注册网络数据接收成功的回调函数
	espconn_regist_disconcb(pesp_conn, tcp_server_disconnect_cb);// 注册成功断开TCP连接的回调函数

	os_printf("\n--------------- ESP8266_TCP_Connect_OK ---------------\n");
	os_printf("server ip:%d.%d.%d.%d\r\n",
			pesp_conn->proto.tcp->remote_ip[0],	//服务器IP地址
			pesp_conn->proto.tcp->remote_ip[1],
			pesp_conn->proto.tcp->remote_ip[2],
			pesp_conn->proto.tcp->remote_ip[3]);
	os_printf("server port:%d\r\n", pesp_conn->proto.tcp->remote_port);	//服务器端口号

	send_weather_http_request(pesp_conn);
}

//域名解析回调函数
static void ICACHE_FLASH_ATTR dns_found_cb(const char *name, ip_addr_t *ipaddr,
		void *arg) {
	struct espconn *pespconn = (struct espconn *) arg;
	if (NULL == ipaddr)	//域名解析失败
			{
		os_printf("\r\n----DomainName Analyse Failed----\r\n");
	} else if (ipaddr != NULL && ipaddr->addr != 0)	//域名解析成功
			{
		os_printf("\r\n----DomainName Analyse Succeed----\r\n");
		os_printf("domain name :%s\r\n", name);
		os_printf("domain ip:%d.%d.%d.%d\r\n", ipaddr->addr & 0xff,
				(ipaddr->addr >> 8) & 0xff, (ipaddr->addr >> 16) & 0xff,
				(ipaddr->addr >> 24) & 0xff);
		server_ip.addr = ipaddr->addr;	//保存ip地址
		os_memcpy(pespconn->proto.tcp->remote_ip, &server_ip.addr,
				sizeof(server_ip.addr));	//设置ip地址
		espconn_regist_connectcb(pespconn, tcp_connnect_cb);// 注册TCP连接成功建立的回调函数
		espconn_regist_reconcb(pespconn, tcp_break_cb);		// 注册TCP连接异常断开的回调函数
		espconn_connect(pespconn);	// 连接服务器
	}

}
void ICACHE_FLASH_ATTR tcp_client_init(void) {
	//结构体赋值
	sta_netcon.type = ESPCONN_TCP;	//设置为TCP协议
	sta_netcon.state = ESPCONN_NONE;	//状态，初始为空
	//sta_netcon.proto.tcp=(esp_tcp*)os_zalloc(sizeof(esp_tcp));// 开辟内存--动态开辟有失败的可能
	sta_netcon.proto.tcp = &esptcp;	// 开辟内存

	sta_netcon.proto.tcp->remote_port = SERVER_PORT;	//设置网络结构体远端端口号
	sta_netcon.proto.tcp->local_port = espconn_port();	////自动获取本地端口
//	os_memcpy(sta_netcon.proto.tcp->remote_ip,server_ip,sizeof(server_ip));//设置网络结构体远端IP
	espconn_gethostbyname(&sta_netcon, SERVER_NAME, &server_ip, dns_found_cb);//域名解析

	myhttp_init();
//	espconn_regist_connectcb(&sta_netcon, tcp_connnect_cb);	// 注册TCP连接成功建立的回调函数
//	espconn_regist_reconcb(&sta_netcon, tcp_break_cb);		// 注册TCP连接异常断开的回调函数
//	espconn_connect(&sta_netcon);	// 连接服务器
}

