#include "driver/tcp_client.h"
#include "driver/myhttp.h"
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


//tcp���������յ����ݵĻص�����
static void ICACHE_FLASH_ATTR tcp_server_recv_cb(void *arg, char *pdata,
		unsigned short len) {
	os_printf("\nESP8266_WIFI_Recv:\r\n");
//	os_printf(pdata);
//	os_printf(" \r\n");
	recvice_weather_data(pdata, len);
}
//tcp���������ݷ��ͳɹ��Ļص�����
static void ICACHE_FLASH_ATTR tcp_server_sent_cb(void *arg) {
	os_printf("\nESP8266_WIFI_Send_OK\n");
}
//tcp�����������Ͽ����ӵĻص�����
static void ICACHE_FLASH_ATTR tcp_server_disconnect_cb(void *arg) {
	os_printf("\nESP8266_TCP_Disconnect_OK\n");
	espconn_connect(&sta_netcon);	// ���ӷ�����
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

	send_weather_http_request(pesp_conn);
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

	myhttp_init();
//	espconn_regist_connectcb(&sta_netcon, tcp_connnect_cb);	// ע��TCP���ӳɹ������Ļص�����
//	espconn_regist_reconcb(&sta_netcon, tcp_break_cb);		// ע��TCP�����쳣�Ͽ��Ļص�����
//	espconn_connect(&sta_netcon);	// ���ӷ�����
}

