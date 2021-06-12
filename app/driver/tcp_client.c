#include "driver/tcp_client.h"
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
//今天+明天+后天的http请求
//#define WEATHER_HTTP_REQUEST "GET https://api.seniverse.com/v3/weather/daily.json?key=SYN55AOrciorg-CB_&location=beijing&language=en&unit=c&start=0&days=3 HTTP/1.1\r\n"\
//							"Host: api.seniverse.com\r\n"\
//							"Accept-Language: zh-cn\r\n"\
//							"Connection: keep-alive\r\n\r\n"
static char weather_http_request[] =
		"GET https://api.seniverse.com/v3/weather/daily.json?key=SYN55AOrciorg-CB_&location=Shenzhen&language=en&unit=c&start=0&days=3 HTTP/1.1\r\n"
				"Host: api.seniverse.com\r\n"
				"Accept-Language: zh-cn\r\n"
				"Connection: keep-alive\r\n\r\n";
//static const char test_data[] =
//		"{\"results\":[{\"location\":{\"id\":\"WX4FBXXFKE4F\",\"name\":\"Beijing\",\"country\":\"CN\",\"path\":\"Beijing,Beijing,China\",\"timezone\":\"Asia/Shanghai\",\"timezone_offset\":\"+08:00\"},"
//				"\"daily\":["
//				"{\"date\":\"2021-05-30\",\"text_day\":\"Sunny\",\"code_day\":\"0\",\"text_night\":\"Cloudy\",\"code_night\":\"4\",\"high\":\"27\",\"low\":\"15\",\"rainfall\":\"0.0\",\"precip\":\"0\",\"wind_direction\":\"SE\",\"wind_direction_degree\":\"135\",\"wind_speed\":\"3.0\",\"wind_scale\":\"1\",\"humidity\":\"37\"},"
//				"{\"date\":\"2021-05-31\",\"text_day\":\"Thundershower\",\"code_day\":\"11\",\"text_night\":\"Cloudy\",\"code_night\":\"4\",\"high\":\"25\",\"low\":\"16\",\"rainfall\":\"2.4\",\"precip\":\"0.98\",\"wind_direction\":\"SE\",\"wind_direction_degree\":\"135\",\"wind_speed\":\"3.0\",\"wind_scale\":\"1\",\"humidity\":\"58\"},"
//				"{\"date\":\"2021-06-01\",\"text_day\":\"Overcast\",\"code_day\":\"9\",\"text_night\":\"Cloudy\",\"code_night\":\"4\",\"high\":\"29\",\"low\":\"18\",\"rainfall\":\"0.0\",\"precip\":\"0\",\"wind_direction\":\"N\",\"wind_direction_degree\":\"0\",\"wind_speed\":\"8.4\",\"wind_scale\":\"2\",\"humidity\":\"33\"}],\"last_update\":\"2021-05-30T08:00:00+08:00\"}]}";

//查找错误码
#define FOUND_VALUE_OK  0
#define FOUND_VALUE_ERR_PARA 1
#define FOUND_VALUE_ERR_NO_KEY 2
#define FOUND_VALUE_ERR_NO_BOUND 3

static weather_t weather[WEATHER_DAYS];
//根据key和边界字符，查找对应的value值，存入out_buf中,index从1开始
static uint8_t ICACHE_FLASH_ATTR find_value_by_key(const char *json_data,
		uint8_t index, const char*str_key, const char*str_bound, char *out_buf,
		uint16_t out_len) {
	if (NULL == json_data || NULL == str_key || NULL == str_bound
			|| NULL == out_buf || out_len < 1) {
		os_printf("para error:%s,%s,%d\r\n", __FILE__, __FUNCTION__, __LINE__);
		return FOUND_VALUE_ERR_PARA;
	}
	uint16_t i, offset, key_len;
	const char *str_start = json_data;
	const char *str_end;
	key_len = strlen(str_key);
	for (i = 0; i < index && str_start != NULL; i++) {
		offset = (0 == i) ? 0 : key_len;
		str_start = strstr(str_start + offset, str_key);
	}
//	os_printf("i:%d str_start addr:0x%x, ", i, (uint32_t) str_start);
	if (NULL == str_start) {
		os_printf("not found index:%d,key:%s to value\r\n", index, str_key);
		return FOUND_VALUE_ERR_NO_KEY;
	}
	str_end = strstr(str_start, str_bound);
	if (NULL == str_end) {
		os_printf("not found index:%d,key:%s to bound\r\n", index, str_key);
		return FOUND_VALUE_ERR_NO_BOUND;
	}
	for (i = key_len;
			str_start[i] != str_end[0] && (i - key_len) < (out_len - 1); i++) {
//		os_printf("%c", str_start[i]);
		out_buf[i - key_len] = str_start[i];
	}
	out_buf[i - key_len] = '\0';
//	os_printf(" \r\n");
	return FOUND_VALUE_OK;
}
//获取最高最低温度
static void ICACHE_FLASH_ATTR get_temperature(const char *json_data,
		uint8_t index, char *hi_buf, uint16_t hi_len, char *lo_buf,
		uint16_t lo_len) {
	const char *str_h = "\"high\":\"";
	const char *str_l = "\"low\":\"";
	const char *str_bound = "\",";
	if (FOUND_VALUE_OK
			== find_value_by_key(json_data, index, str_h, str_bound, hi_buf,
					hi_len))
		os_printf("index:%d,%s/", index, hi_buf);
	if (FOUND_VALUE_OK
			== find_value_by_key(json_data, index, str_l, str_bound, lo_buf,
					lo_len))
		os_printf("%s\r\n", lo_buf);
}
//获取白天和晚上的天气
static void ICACHE_FLASH_ATTR get_weather(const char *json_data, uint8_t index,
		char *day_buf, uint16_t day_len, char *night_buf, uint16_t night_len) {
	const char *str_day = "\"text_day\":\"";
	const char *str_night = "\"text_night\":\"";
	const char *str_bound = "\",";
	if (FOUND_VALUE_OK
			== find_value_by_key(json_data, index, str_day, str_bound, day_buf,
					day_len))
		os_printf("index:%d,%s/", index, day_buf);
	if (FOUND_VALUE_OK
			== find_value_by_key(json_data, index, str_night, str_bound,
					night_buf, night_len))
		os_printf("%s\r\n", night_buf);
}
//获取降雨量
static void ICACHE_FLASH_ATTR ge_rainfall(const char *json_data, uint8_t index,
		char *out_buf, uint16_t out_len) {
	const char *str_key = "\"rainfall\":\"";
	const char *str_bound = "\",";
	if (FOUND_VALUE_OK
			== find_value_by_key(json_data, index, str_key, str_bound, out_buf,
					out_len))
		os_printf("index:%d,rainfall:%s\r\n", index, out_buf);
}
//获取相对湿度
static void ICACHE_FLASH_ATTR ge_humidity(const char *json_data, uint8_t index,
		char *out_buf, uint16_t out_len) {
	const char *str_key = "\"humidity\":\"";
	const char *str_bound = "\"}";
	if (FOUND_VALUE_OK
			== find_value_by_key(json_data, index, str_key, str_bound, out_buf,
					out_len))
		os_printf("index:%d,humidity:%s%\r\n", index, out_buf);
}
static void ICACHE_FLASH_ATTR analysis_weather_json(const char *json_data) {
	uint8_t i;
	for (i = 0; i < WEATHER_DAYS; i++) {
		get_temperature(json_data, i + 1, weather[i].high,
				sizeof(weather[i].high), weather[i].low,
				sizeof(weather[i].low));
		get_weather(json_data, i + 1, weather[i].text_day,
				sizeof(weather[i].text_day), weather[i].text_night,
				sizeof(weather[i].text_night));
		ge_rainfall(json_data, i + 1, weather[i].rainfall,
				sizeof(weather[i].rainfall));
		ge_humidity(json_data, i + 1, weather[i].humidity,
				sizeof(weather[i].humidity));
	}
}

//发送天气http请求
static void ICACHE_FLASH_ATTR send_weather_http_request(void) {
	os_printf("ESP8266 WIFI Send Http:\r\n");
	os_printf(weather_http_request);
	espconn_send(&sta_netcon, weather_http_request,
			strlen(weather_http_request));
}
//tcp服务器接收到数据的回调函数
static void ICACHE_FLASH_ATTR tcp_server_recv_cb(void *arg, char *pdata,
		unsigned short len) {
//	espconn_send((struct espconn*)arg,pdata,len);//回传
	os_printf("\nESP8266_WIFI_Recv:\r\n");
	os_printf(pdata);
	os_printf(" \r\n");
	analysis_weather_json(pdata);
}
//tcp服务器数据发送成功的回调函数
static void ICACHE_FLASH_ATTR tcp_server_sent_cb(void *arg) {
	os_printf("\nESP8266_WIFI_Send_OK\n");
}
//tcp服务器正常断开连接的回调函数
static void ICACHE_FLASH_ATTR tcp_server_disconnect_cb(void *arg) {
	os_printf("\nESP8266_TCP_Disconnect_OK\n");
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

	send_weather_http_request();
//	analysis_weather_json(test_data);
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

	os_memset(&weather, 0, sizeof(weather));	//天气信息清零
//	espconn_regist_connectcb(&sta_netcon, tcp_connnect_cb);	// 注册TCP连接成功建立的回调函数
//	espconn_regist_reconcb(&sta_netcon, tcp_break_cb);		// 注册TCP连接异常断开的回调函数
//	espconn_connect(&sta_netcon);	// 连接服务器
}
//获取天气信息
weather_t * ICACHE_FLASH_ATTR get_weather_info(void) {
	return weather;
}
