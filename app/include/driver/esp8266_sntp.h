#ifndef ESP8266_SNTP_H
#define ESP8266_SNTP_H

typedef struct
{
	char time[9];
	char date[11];
	char week[4];
}mytime_t;

void esp8266_sntp_init(void);
//获取实时时间
mytime_t *  get_real_time(void);

#endif //ESP8266_SNTP_H
