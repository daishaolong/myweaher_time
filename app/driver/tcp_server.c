#include "driver/tcp_server.h"
#include "user_interface.h"
#include "osapi.h"

#include "ip_addr.h"			// 被"espconn.h"使用。在"espconn.h"开头#include"ip_addr.h"或#include"ip_addr.h"放在"espconn.h"之前
#include "espconn.h"			// TCP/UDP接口
#include "ets_sys.h"			// 回调函数
#include "mem.h"




#define TCP_SERVER_PORT			8266//服务器端口号
static struct espconn sta_netcon;//网络连接结构体
static esp_tcp	 esptcp;//tcp协议结构体

//tcp服务器接收到数据的回调函数
static void ICACHE_FLASH_ATTR tcp_server_recv_cb(void *arg, char *pdata, unsigned short len)
{
	os_printf("\nESP8266_WIFI_Recv_OK\n");
	espconn_send((struct espconn*)arg,pdata,len);//回传
}
//tcp服务器数据发送成功的回调函数
static void ICACHE_FLASH_ATTR tcp_server_sent_cb(void *arg)
{
	os_printf("\nESP8266_WIFI_Send_OK\n");
}
//tcp服务器正常断开连接的回调函数
static void ICACHE_FLASH_ATTR tcp_server_disconnect_cb(void *arg)
{
	os_printf("\nESP8266_TCP_Disconnect_OK\n");
}
//tcp异常断开的回调函数
static void ICACHE_FLASH_ATTR tcp_break_cb(void *arg, sint8 err)
{
	os_printf("\nESP8266_TCP_Break\n");
}
//tcp建立连接的回调函数
static void ICACHE_FLASH_ATTR tcp_connnect_cb(void *arg)
{
	struct espconn *pesp_conn=(struct espconn *)arg;				//得到espconn
	espconn_regist_sentcb(pesp_conn, tcp_server_sent_cb);			// 注册网络数据发送成功的回调函数
	espconn_regist_recvcb(pesp_conn, tcp_server_recv_cb);			// 注册网络数据接收成功的回调函数
	espconn_regist_disconcb(pesp_conn,tcp_server_disconnect_cb);	// 注册成功断开TCP连接的回调函数

	os_printf("\n--------------- ESP8266_TCP_Connect_OK ---------------\n");
	os_printf("client ip:%d,%d,%d,%d\r\n",pesp_conn->proto.tcp->remote_ip[0],//客户端IP地址
			pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],pesp_conn->proto.tcp->remote_ip[3]);
	os_printf("client port:%d\r\n",pesp_conn->proto.tcp->remote_port);//客户端端口号
}

void ICACHE_FLASH_ATTR tcp_server_init(void)
{
	//结构体赋值
	sta_netcon.type=ESPCONN_TCP;//设置为TCP协议
	sta_netcon.state=ESPCONN_NONE;//状态，初始为空
//	sta_netcon.proto.tcp=(esp_tcp*)os_zalloc(sizeof(esp_tcp));// 开辟内存--动态申请有失败的可能
	sta_netcon.proto.tcp=&esptcp;// 开辟内存
	sta_netcon.proto.tcp->local_port = TCP_SERVER_PORT ;	// 设置本地端口

	//注册连接成功回调函数/异常断开回调函数
	espconn_regist_connectcb(&sta_netcon, tcp_connnect_cb);	// 注册TCP连接成功建立的回调函数
	espconn_regist_reconcb(&sta_netcon, tcp_break_cb);		// 注册TCP连接异常断开的回调函数
	// 创建TCP_server，建立侦听
	//----------------------------------------------------------
	espconn_accept(&sta_netcon);	// 创建TCP_server，建立侦听
	espconn_regist_time(&sta_netcon,120, 0); 	//设置超时断开时间。单位=秒，最大值=7200
	os_printf("tcp server port:%d\r\n",TCP_SERVER_PORT);
}

