#ifndef ESP8266_STATION_H
#define ESP8266_STATION_H

//#include "eagle_soc.h"
//#include "c_types.h"
#define MYSMART_LINK_ENABLE 1 //1ʹ����������  0��ֹ��������
typedef void (*station_connected_wifi_callback)(void);//STA�ɹ�����wifi�Ļص�����--tcp/udp�������Ӻ���

void station_regist_connnetcd_cb(station_connected_wifi_callback connect_cb);
void  esp8266_station_init(void);

#endif //ESP8266_STATION_H
