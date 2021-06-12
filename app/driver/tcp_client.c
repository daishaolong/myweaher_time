#include "driver/tcp_client.h"
#include "user_interface.h"
#include "osapi.h"

#include "ip_addr.h"			// ��"espconn.h"ʹ�á���"espconn.h"��ͷ#include"ip_addr.h"��#include"ip_addr.h"����"espconn.h"֮ǰ
#include "espconn.h"			// TCP/UDP�ӿ�
#include "ets_sys.h"			// �ص�����
#include "mem.h"
#include "stdio.h"

//static const uint8 server_ip[4]={192,168,3,5};//������IP
//#define SERVER_PORT			80// �������˿ں�
static struct espconn sta_netcon; //�������ӽṹ��
static esp_tcp esptcp; //tcpЭ��ṹ��

#define SERVER_NAME   "api.seniverse.com"//��֪����
#define SERVER_PORT			80//HTTP�˿ں�
static ip_addr_t server_ip; //������ip��ַ
#define CITY_STR	"Shenzhen"//�����ַ���
#define MY_KEY_STR "SYN55AOrciorg-CB_"//˽Կ
//����+����+�����http����
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

//���Ҵ�����
#define FOUND_VALUE_OK  0
#define FOUND_VALUE_ERR_PARA 1
#define FOUND_VALUE_ERR_NO_KEY 2
#define FOUND_VALUE_ERR_NO_BOUND 3

static weather_t weather[WEATHER_DAYS];
//����key�ͱ߽��ַ������Ҷ�Ӧ��valueֵ������out_buf��,index��1��ʼ
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
//��ȡ�������¶�
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
//��ȡ��������ϵ�����
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
//��ȡ������
static void ICACHE_FLASH_ATTR ge_rainfall(const char *json_data, uint8_t index,
		char *out_buf, uint16_t out_len) {
	const char *str_key = "\"rainfall\":\"";
	const char *str_bound = "\",";
	if (FOUND_VALUE_OK
			== find_value_by_key(json_data, index, str_key, str_bound, out_buf,
					out_len))
		os_printf("index:%d,rainfall:%s\r\n", index, out_buf);
}
//��ȡ���ʪ��
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

//��������http����
static void ICACHE_FLASH_ATTR send_weather_http_request(void) {
	os_printf("ESP8266 WIFI Send Http:\r\n");
	os_printf(weather_http_request);
	espconn_send(&sta_netcon, weather_http_request,
			strlen(weather_http_request));
}
//tcp���������յ����ݵĻص�����
static void ICACHE_FLASH_ATTR tcp_server_recv_cb(void *arg, char *pdata,
		unsigned short len) {
//	espconn_send((struct espconn*)arg,pdata,len);//�ش�
	os_printf("\nESP8266_WIFI_Recv:\r\n");
	os_printf(pdata);
	os_printf(" \r\n");
	analysis_weather_json(pdata);
}
//tcp���������ݷ��ͳɹ��Ļص�����
static void ICACHE_FLASH_ATTR tcp_server_sent_cb(void *arg) {
	os_printf("\nESP8266_WIFI_Send_OK\n");
}
//tcp�����������Ͽ����ӵĻص�����
static void ICACHE_FLASH_ATTR tcp_server_disconnect_cb(void *arg) {
	os_printf("\nESP8266_TCP_Disconnect_OK\n");
}
// TCP�����쳣�Ͽ�ʱ�Ļص�������ESP8266��ΪTCP_Client������TCP_Serverʧ�ܣ�Ҳ�����˺�����
//tcp�쳣�Ͽ��Ļص�����
static void ICACHE_FLASH_ATTR tcp_break_cb(void *arg, sint8 err) {
	os_printf("\nESP8266_TCP_Break,err code:%d\n", err);
	espconn_connect(&sta_netcon);	// ���ӷ�����
}
//tcp�������ӵĻص�����
static void ICACHE_FLASH_ATTR tcp_connnect_cb(void *arg) {
	struct espconn *pesp_conn = (struct espconn *) arg;				//�õ�espconn
	espconn_regist_sentcb(pesp_conn, tcp_server_sent_cb);	// ע���������ݷ��ͳɹ��Ļص�����
	espconn_regist_recvcb(pesp_conn, tcp_server_recv_cb);	// ע���������ݽ��ճɹ��Ļص�����
	espconn_regist_disconcb(pesp_conn, tcp_server_disconnect_cb);// ע��ɹ��Ͽ�TCP���ӵĻص�����

	os_printf("\n--------------- ESP8266_TCP_Connect_OK ---------------\n");
	os_printf("server ip:%d.%d.%d.%d\r\n",
			pesp_conn->proto.tcp->remote_ip[0],	//������IP��ַ
			pesp_conn->proto.tcp->remote_ip[1],
			pesp_conn->proto.tcp->remote_ip[2],
			pesp_conn->proto.tcp->remote_ip[3]);
	os_printf("server port:%d\r\n", pesp_conn->proto.tcp->remote_port);	//�������˿ں�

	send_weather_http_request();
//	analysis_weather_json(test_data);
}

//���������ص�����
static void ICACHE_FLASH_ATTR dns_found_cb(const char *name, ip_addr_t *ipaddr,
		void *arg) {
	struct espconn *pespconn = (struct espconn *) arg;
	if (NULL == ipaddr)	//��������ʧ��
			{
		os_printf("\r\n----DomainName Analyse Failed----\r\n");
	} else if (ipaddr != NULL && ipaddr->addr != 0)	//���������ɹ�
			{
		os_printf("\r\n----DomainName Analyse Succeed----\r\n");
		os_printf("domain name :%s\r\n", name);
		os_printf("domain ip:%d.%d.%d.%d\r\n", ipaddr->addr & 0xff,
				(ipaddr->addr >> 8) & 0xff, (ipaddr->addr >> 16) & 0xff,
				(ipaddr->addr >> 24) & 0xff);
		server_ip.addr = ipaddr->addr;	//����ip��ַ
		os_memcpy(pespconn->proto.tcp->remote_ip, &server_ip.addr,
				sizeof(server_ip.addr));	//����ip��ַ
		espconn_regist_connectcb(pespconn, tcp_connnect_cb);// ע��TCP���ӳɹ������Ļص�����
		espconn_regist_reconcb(pespconn, tcp_break_cb);		// ע��TCP�����쳣�Ͽ��Ļص�����
		espconn_connect(pespconn);	// ���ӷ�����
	}

}
void ICACHE_FLASH_ATTR tcp_client_init(void) {
	//�ṹ�帳ֵ
	sta_netcon.type = ESPCONN_TCP;	//����ΪTCPЭ��
	sta_netcon.state = ESPCONN_NONE;	//״̬����ʼΪ��
	//sta_netcon.proto.tcp=(esp_tcp*)os_zalloc(sizeof(esp_tcp));// �����ڴ�--��̬������ʧ�ܵĿ���
	sta_netcon.proto.tcp = &esptcp;	// �����ڴ�

	sta_netcon.proto.tcp->remote_port = SERVER_PORT;	//��������ṹ��Զ�˶˿ں�
	sta_netcon.proto.tcp->local_port = espconn_port();	////�Զ���ȡ���ض˿�
//	os_memcpy(sta_netcon.proto.tcp->remote_ip,server_ip,sizeof(server_ip));//��������ṹ��Զ��IP
	espconn_gethostbyname(&sta_netcon, SERVER_NAME, &server_ip, dns_found_cb);//��������

	os_memset(&weather, 0, sizeof(weather));	//������Ϣ����
//	espconn_regist_connectcb(&sta_netcon, tcp_connnect_cb);	// ע��TCP���ӳɹ������Ļص�����
//	espconn_regist_reconcb(&sta_netcon, tcp_break_cb);		// ע��TCP�����쳣�Ͽ��Ļص�����
//	espconn_connect(&sta_netcon);	// ���ӷ�����
}
//��ȡ������Ϣ
weather_t * ICACHE_FLASH_ATTR get_weather_info(void) {
	return weather;
}
