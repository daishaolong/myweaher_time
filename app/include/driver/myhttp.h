#ifndef MY_HTTP_H
#define MY_HTTP_H

#include "ip_addr.h"			// ��"espconn.h"ʹ�á���"espconn.h"��ͷ#include"ip_addr.h"��#include"ip_addr.h"����"espconn.h"֮ǰ
#include "espconn.h"			// TCP/UDP�ӿ�

#define WEATHER_CHAR_LEN 16//�����ַ�����
#define WEATHER_DAYS	3//3������

typedef struct {
	char high[WEATHER_CHAR_LEN];
	char low[WEATHER_CHAR_LEN];
	char humidity[WEATHER_CHAR_LEN];
	char code_day[WEATHER_CHAR_LEN];
	char text_day[WEATHER_CHAR_LEN];
	char code_night[WEATHER_CHAR_LEN];
	char text_night[WEATHER_CHAR_LEN];
	char rainfall[WEATHER_CHAR_LEN];
} weather_t;


void  send_weather_http_request(struct espconn *espconn);
void  recvice_weather_data(char *in_data,unsigned short in_len);
weather_t* get_weather_info(void);
void ICACHE_FLASH_ATTR  myhttp_init(void);

#endif //MY_HTTP_H
