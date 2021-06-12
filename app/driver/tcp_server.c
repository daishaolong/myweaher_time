#include "driver/tcp_server.h"
#include "user_interface.h"
#include "osapi.h"

#include "ip_addr.h"			// ��"espconn.h"ʹ�á���"espconn.h"��ͷ#include"ip_addr.h"��#include"ip_addr.h"����"espconn.h"֮ǰ
#include "espconn.h"			// TCP/UDP�ӿ�
#include "ets_sys.h"			// �ص�����
#include "mem.h"




#define TCP_SERVER_PORT			8266//�������˿ں�
static struct espconn sta_netcon;//�������ӽṹ��
static esp_tcp	 esptcp;//tcpЭ��ṹ��

//tcp���������յ����ݵĻص�����
static void ICACHE_FLASH_ATTR tcp_server_recv_cb(void *arg, char *pdata, unsigned short len)
{
	os_printf("\nESP8266_WIFI_Recv_OK\n");
	espconn_send((struct espconn*)arg,pdata,len);//�ش�
}
//tcp���������ݷ��ͳɹ��Ļص�����
static void ICACHE_FLASH_ATTR tcp_server_sent_cb(void *arg)
{
	os_printf("\nESP8266_WIFI_Send_OK\n");
}
//tcp�����������Ͽ����ӵĻص�����
static void ICACHE_FLASH_ATTR tcp_server_disconnect_cb(void *arg)
{
	os_printf("\nESP8266_TCP_Disconnect_OK\n");
}
//tcp�쳣�Ͽ��Ļص�����
static void ICACHE_FLASH_ATTR tcp_break_cb(void *arg, sint8 err)
{
	os_printf("\nESP8266_TCP_Break\n");
}
//tcp�������ӵĻص�����
static void ICACHE_FLASH_ATTR tcp_connnect_cb(void *arg)
{
	struct espconn *pesp_conn=(struct espconn *)arg;				//�õ�espconn
	espconn_regist_sentcb(pesp_conn, tcp_server_sent_cb);			// ע���������ݷ��ͳɹ��Ļص�����
	espconn_regist_recvcb(pesp_conn, tcp_server_recv_cb);			// ע���������ݽ��ճɹ��Ļص�����
	espconn_regist_disconcb(pesp_conn,tcp_server_disconnect_cb);	// ע��ɹ��Ͽ�TCP���ӵĻص�����

	os_printf("\n--------------- ESP8266_TCP_Connect_OK ---------------\n");
	os_printf("client ip:%d,%d,%d,%d\r\n",pesp_conn->proto.tcp->remote_ip[0],//�ͻ���IP��ַ
			pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],pesp_conn->proto.tcp->remote_ip[3]);
	os_printf("client port:%d\r\n",pesp_conn->proto.tcp->remote_port);//�ͻ��˶˿ں�
}

void ICACHE_FLASH_ATTR tcp_server_init(void)
{
	//�ṹ�帳ֵ
	sta_netcon.type=ESPCONN_TCP;//����ΪTCPЭ��
	sta_netcon.state=ESPCONN_NONE;//״̬����ʼΪ��
//	sta_netcon.proto.tcp=(esp_tcp*)os_zalloc(sizeof(esp_tcp));// �����ڴ�--��̬������ʧ�ܵĿ���
	sta_netcon.proto.tcp=&esptcp;// �����ڴ�
	sta_netcon.proto.tcp->local_port = TCP_SERVER_PORT ;	// ���ñ��ض˿�

	//ע�����ӳɹ��ص�����/�쳣�Ͽ��ص�����
	espconn_regist_connectcb(&sta_netcon, tcp_connnect_cb);	// ע��TCP���ӳɹ������Ļص�����
	espconn_regist_reconcb(&sta_netcon, tcp_break_cb);		// ע��TCP�����쳣�Ͽ��Ļص�����
	// ����TCP_server����������
	//----------------------------------------------------------
	espconn_accept(&sta_netcon);	// ����TCP_server����������
	espconn_regist_time(&sta_netcon,120, 0); 	//���ó�ʱ�Ͽ�ʱ�䡣��λ=�룬���ֵ=7200
	os_printf("tcp server port:%d\r\n",TCP_SERVER_PORT);
}

