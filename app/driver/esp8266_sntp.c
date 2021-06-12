#include "driver/esp8266_sntp.h"
#include "sntp.h"
#include "user_interface.h"
#include "osapi.h"

LOCAL os_timer_t sntp_timer; //sntp软件定时器

static mytime_t myrealtime;
//获取月份：1-12月
//str:月字符串
static uint8_t ICACHE_FLASH_ATTR get_month_number(char *str) {
	uint8_t i;
	const char *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
			"Aug", "Sep", "Oct", "Nov", "Dec" };
	const uint8_t size = sizeof(month) / sizeof(month[0]);
	for (i = 0; i < size; i++) {
		if (0 == strcmp(str, month[i])) {
			break;
		}
	}
	return i + 1;
}
//src最后一个字符为换行符
static void ICACHE_FLASH_ATTR copy_str(uint8_t from, uint8_t to,
		const char *src, char *des, uint8_t len) {
	uint8_t i, cnt, left = 0;
	for (i = 0, cnt = 0; src[i] != '\0' && src[i] != '\n'; i++) {
		if (from == cnt && (i - left) < len) {
			des[i - left] = src[i];
		}
		if (' ' == src[i]) {
			cnt++;
			if (from == cnt) {
				left = i + 1;
			}
			if (to == cnt)
				break;
		}
	}
	if ((to == cnt || '\0' == src[i] || '\n' == src[i]) && (i - left) < len) {
		des[i - left] = '\0';
	}
}
//获取时间hh:mm:ss
static void ICACHE_FLASH_ATTR get_time(char *str, char * time, uint8 len) {
	if (NULL == time)
		return;
	time[0] = '\0';
	if (len < 9 || NULL == str)
		return;
	copy_str(3, 4, str, time, len);
}
//获取星期
static void ICACHE_FLASH_ATTR get_week(char *str, char * week, uint8 len) {
	if (NULL == week)
		return;
	week[0] = '\0';
	if (len < 4 || NULL == str)
		return;
	copy_str(0, 1, str, week, len);
}
//获取日期yyyy-mm-dd
static void ICACHE_FLASH_ATTR get_date(char *str, char *date, uint8 len) {
	char year[5] = { 0 }, month[4] = { 0 }, data[3] = { 0 };
	uint8_t num = 0;
	if (NULL == date)
		return;
	date[0] = '\0';
	if (NULL == str || len < 11)
		return;
	copy_str(1, 2, str, month, sizeof(month));
	num = get_month_number(month);
	month[0] = num / 10 + '0';
	month[1] = num % 10 + '0';
	month[2] = '\0';
	copy_str(2, 3, str, data, sizeof(data));
	copy_str(4, 5, str, year, sizeof(year));
	strcat(date, year);
	strcat(date, "-");
	strcat(date, month);
	strcat(date, "-");
	strcat(date, data);
}

LOCAL void ICACHE_FLASH_ATTR user_check_sntp_stamp(void *arg) {
	uint32 current_stamp; //时间戳，距离基准时间的秒数
	char *str_realtime; //实际时间的字符串-最后一个字符为换行符
	//查询当前距离基准时间 (1970.01.01 00： 00： 00 GMT + 8) 的时间戳，单位：秒
	current_stamp = sntp_get_current_timestamp();
	if (current_stamp != 0) {
		//查询实际时间 (GMT + 8)
		str_realtime = sntp_get_real_time(current_stamp);
//		os_printf("\r\nsntp:%d,%s\r\n", current_stamp, str_realtime);
//		static u8 clear_flag = 0;
//		if (0 == clear_flag) {
//			OLED_Clear();
//			clear_flag = 1;
//		}
//		char time[9] = { 0 }, date[11] = { 0 }, week[4] = { 0 };
		get_time(str_realtime, myrealtime.time, sizeof(myrealtime.time));
		get_date(str_realtime, myrealtime.date, sizeof(myrealtime.date));
		get_week(str_realtime, myrealtime.week, sizeof(myrealtime.week));
//		OLED_ShowString(0, 0, "Shenzhen");
//		OLED_ShowString(0, 2, myrealtime.time);
//		OLED_ShowString(0, 4, myrealtime.date);
//		OLED_ShowString(0, 6, myrealtime.week);

	}
}

void ICACHE_FLASH_ATTR esp8266_sntp_init(void) {
	ip_addr_t addr;
	sntp_setservername(0, "us.pool.ntp.org");//	set	server	0	by	domain	name
	sntp_setservername(1, "ntp.sjtu.edu.cn");//	set	server	1	by	domain	name
	ipaddr_aton("210.72.145.44", &addr);
	sntp_setserver(2, &addr);	//	set	server	2	by	IP	address
	sntp_init();

	//设置软件定时器，定时查询时间
	os_timer_disarm(&sntp_timer);
	os_timer_setfn(&sntp_timer, (os_timer_func_t *) user_check_sntp_stamp,
			NULL);
	os_timer_arm(&sntp_timer, 1000, 1);
	os_memset(&myrealtime,0,sizeof(myrealtime));
}

//获取实时时间
mytime_t * ICACHE_FLASH_ATTR get_real_time(void)
{
	return &myrealtime;
}
