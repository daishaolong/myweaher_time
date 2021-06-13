#ifndef ESP8266_STATION_H
#define ESP8266_STATION_H

//#include "eagle_soc.h"
//#include "c_types.h"
#define MYSMART_LINK_ENABLE 1 //1使能智能配网  0禁止智能配网
typedef void (*station_connected_wifi_callback)(void);//STA成功连接wifi的回调函数--tcp/udp网络连接函数

void station_regist_connnetcd_cb(station_connected_wifi_callback connect_cb);
void  esp8266_station_init(void);

#endif //ESP8266_STATION_H
