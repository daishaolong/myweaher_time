
#include "driver/myhttp.h"
#include "user_interface.h"
#include "osapi.h"

//wifi接收缓冲区
#define WIFI_RECV_MAX_SIZE  (1024*2)
static uint8_t wifi_recv_buf[WIFI_RECV_MAX_SIZE];
static uint16_t wifi_recv_size=0;
static os_timer_t wifi_recv_handle_timer;
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
//获取白天天气码
static void ICACHE_FLASH_ATTR get_code_day(const char *json_data, uint8_t index,
		char *out_buf, uint16_t out_len) {
	const char *str_key = "\"code_day\":\"";
	const char *str_bound = "\"}";
	if (FOUND_VALUE_OK
			== find_value_by_key(json_data, index, str_key, str_bound, out_buf,
					out_len))
		os_printf("index:%d,humidity:%s%\r\n", index, out_buf);
}
//获取晚上天气码
static void ICACHE_FLASH_ATTR get_code_night(const char *json_data, uint8_t index,
		char *out_buf, uint16_t out_len) {
	const char *str_key = "\"code_night\":\"";
	const char *str_bound = "\"}";
	if (FOUND_VALUE_OK
			== find_value_by_key(json_data, index, str_key, str_bound, out_buf,
					out_len))
		os_printf("index:%d,humidity:%s%\r\n", index, out_buf);
}
static void ICACHE_FLASH_ATTR analysis_weather_json(void *arg) {
	uint8_t i;
	const char *json_data=(const char *)arg;
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
		get_code_day(json_data, i + 1, weather[i].code_day,
				sizeof(weather[i].code_day));
		get_code_night(json_data, i + 1, weather[i].code_night,
				sizeof(weather[i].code_night));
	}
}


//发送天气http请求
void ICACHE_FLASH_ATTR send_weather_http_request(struct espconn *espconn) {
	os_printf("ESP8266 WIFI Send Http:\r\n");
	os_printf(weather_http_request);
	wifi_recv_size=0;

	os_timer_disarm(&wifi_recv_handle_timer);
	os_timer_setfn(&wifi_recv_handle_timer,analysis_weather_json,wifi_recv_buf);
	os_timer_arm(&wifi_recv_handle_timer,100,0);

	espconn_send(espconn, weather_http_request,
			strlen(weather_http_request));
}
//接收天气数据
void ICACHE_FLASH_ATTR recvice_weather_data(char *in_data,unsigned short in_len)
{
	uint16_t i;
	for(i=0;i<in_len &&wifi_recv_size<(WIFI_RECV_MAX_SIZE-1) ;i++)
	{
		wifi_recv_buf[wifi_recv_size]=in_data[i];
		wifi_recv_size++;
	}
	wifi_recv_buf[wifi_recv_size]='\0';
	os_timer_disarm(&wifi_recv_handle_timer);
	os_timer_arm(&wifi_recv_handle_timer,100,0);
}
//获取天气信息
weather_t * ICACHE_FLASH_ATTR get_weather_info(void) {
	return weather;
}
//http初始化
void ICACHE_FLASH_ATTR  myhttp_init(void)
{
	os_memset(&weather, 0, sizeof(weather));	//天气信息清零
}
