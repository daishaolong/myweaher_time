

#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "driver/led.h"

//��̬ȫ�ֱ���
static os_timer_t sys_run_led_timer;	//ϵͳָʾ�����ж�ʱ��
static uint8 sys_led_status;			//SYS LED�Ƶ�ǰ״̬

//�궨��
#define SYS_RUN_LED_TIMER_PERIOD_MS  	1000//ϵͳ����ָʾ�ƶ�ʱ������ʱ��
#define SYS_RUN_LED_TIMER_REPEAT		1//ϵͳ����ָʾ�ƶ�ʱ����������
#define SYS_LED_ON() 					GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0)//GPIO4���ʹ�ܣ�������͵�ƽ
#define SYS_LED_OFF() 					GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1)//GPIO4���ʹ�ܣ�������ߵ�ƽ
#define SYS_LED_PIN_NAME 				PERIPHS_IO_MUX_GPIO4_U//SYS LED���żĴ���
#define SYS_LED_FUNC	 				FUNC_GPIO4//SYS LED���Ź���
#define LED_ON   1						//LED ��״̬
#define LED_OFF  0						//LED ��״̬
/******************************************************************************
 * FunctionName : sys_run_led
 * Description  : ϵͳ����ָʾ�����У�����sys_led_status״̬����led����
 * Parameters   : parg  sys_led_status������ַ
 * Returns      : none
*******************************************************************************/
static void ICACHE_FLASH_ATTR
sys_run_led(void *parg) {
	uint8 *led_status = (uint8*) parg;
	(LED_ON == (*led_status)) ? SYS_LED_OFF() : SYS_LED_ON();
	(*led_status) = (LED_ON == (*led_status)) ? LED_OFF : LED_ON;
//	os_printf("\r\n-sys running!!!--\r\n");
}
/******************************************************************************
 * FunctionName : sys_run_led_init
 * Description  : ϵͳ����ָʾ�Ƴ�ʼ��
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
sys_run_led_init(void) {
	PIN_FUNC_SELECT(SYS_LED_PIN_NAME, SYS_LED_FUNC);		//LED IO���ó�GPIO����
	SYS_LED_OFF();			//�ر�LED��
	sys_led_status = LED_OFF;			//��״̬Ϊ�ر�

	os_timer_disarm(&sys_run_led_timer);			//ȡ�������ʱ����ʱ
	os_timer_setfn(&sys_run_led_timer, sys_run_led, &sys_led_status);//���������ʱ���Ļص������ͻص������Ĳ���
	os_timer_arm(&sys_run_led_timer, SYS_RUN_LED_TIMER_PERIOD_MS,
			SYS_RUN_LED_TIMER_REPEAT);			//ʹ�ܺ��붨ʱ��

}

