#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#define WEATHER_CHAR_LEN 10//天气字符长度
#define WEATHER_DAYS	3//3天天气

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



void tcp_client_init(void);
weather_t* get_weather_info(void);





#endif //TCP_CLIENT_H
