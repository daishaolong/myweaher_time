// ͷ�ļ�����
//==============================================================================
#include "driver/oled.h"
//#include "driver/oledfont.h"	// �ַ���
#include "driver/oledimage.h"	// ͼƬ��
#include "osapi.h"
//==============================================================================

// ������ʱ����
//===========================================
void ICACHE_FLASH_ATTR delay_ms(u32 C_time) {
	for (; C_time > 0; C_time--)
		os_delay_us(1000);
}
// OLED��غ���
//==============================================================================
// IIC��ʼ��( SCL==IO14��SDA==IO2 )
//-----------------------------------------------------------
void ICACHE_FLASH_ATTR IIC_Init_JX(void) {
	i2c_master_gpio_init();		// ��ʼ��SCL(IO14)��SDA(IO2)
}
//-----------------------------------------------------------

// ��OLEDд��ָ���ֽ�
//----------------------------------------------------------------------------
u8 ICACHE_FLASH_ATTR OLED_Write_Command(u8 OLED_Byte) {
	i2c_master_start();					// ������ʼ�ź�

	i2c_master_writeByte(0x78);			// ѡ�������ϵ�OLED[0111 100X B]
	if (i2c_master_checkAck() == false) {
		i2c_master_stop();
		return 0;
	}

	i2c_master_writeByte(0x00);			// [0x00]��ʾ��һ�ֽ�д�����[ָ��]
	if (i2c_master_checkAck() == false) {
		i2c_master_stop();
		return 0;
	}

	i2c_master_writeByte(OLED_Byte);	// [ָ�����]
	if (i2c_master_checkAck() == false) {
		i2c_master_stop();
		return 0;
	}

	i2c_master_stop();					// ����ֹͣ�ź�

	return true;						// ����д��ɹ�
}
//----------------------------------------------------------------------------

// ��OLEDд�������ֽ�
//----------------------------------------------------------------------------
u8 ICACHE_FLASH_ATTR OLED_Write_Data(u8 OLED_Byte) {
	i2c_master_start();					// ������ʼ�ź�

	i2c_master_writeByte(0x78);			// ѡ�������ϵ�OLED[0111 100X B]
	if (i2c_master_checkAck() == false) {
		i2c_master_stop();
		return 0;
	}

	i2c_master_writeByte(0x40);			// [0x40]��ʾ��һ�ֽ�д�����[����]
	if (i2c_master_checkAck() == false) {
		i2c_master_stop();
		return 0;
	}

	i2c_master_writeByte(OLED_Byte);	// [��������]
	if (i2c_master_checkAck() == false) {
		i2c_master_stop();
		return 0;
	}

	i2c_master_stop();					// ����ֹͣ�ź�

	return true;						// ����д��ɹ�
}
//----------------------------------------------------------------------------

// ��OLEDд��һ�ֽ�����/ָ��
//---------------------------------------------------------------
void ICACHE_FLASH_ATTR OLED_WR_Byte(u8 OLED_Byte, u8 OLED_Type) {
	if (OLED_Type)
		OLED_Write_Data(OLED_Byte);		// д������
	else
		OLED_Write_Command(OLED_Byte);	// д��ָ��
}
//---------------------------------------------------------------

// ����д��ĳֵ
//------------------------------------------------------------------------
void ICACHE_FLASH_ATTR OLED_Clear(void) {
	u8 N_Page, N_row;

	for (N_Page = 0; N_Page < 8; N_Page++) {
		OLED_WR_Byte(0xb0 + N_Page, OLED_CMD);	// ��0��7ҳ����д��
		OLED_WR_Byte(0x00, OLED_CMD);      	// �е͵�ַ
		OLED_WR_Byte(0x10, OLED_CMD);      	// �иߵ�ַ

		for (N_row = 0; N_row < 128; N_row++)
			OLED_WR_Byte(0x00, OLED_DATA);
	}
}
//------------------------------------------------------------------------

// ��������д�����ʼ�С���
//------------------------------------------------------------------------
void ICACHE_FLASH_ATTR OLED_Set_Pos(u8 x, u8 y) {
	OLED_WR_Byte(0xb0 + y, OLED_CMD);				// д��ҳ��ַ

	OLED_WR_Byte((x & 0x0f), OLED_CMD);  			// д���еĵ�ַ(�Ͱ��ֽ�)

	OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD);	// д���еĵ�ַ(�߰��ֽ�)
}
//------------------------------------------------------------------------

void ICACHE_FLASH_ATTR OLED_ShowImage(void);
// ��ʼ��OLED
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR OLED_Init(void) {
	IIC_Init_JX();					// ��ʼ��IIC

	system_soft_wdt_feed();			// ι��(��ֹESP8266��λ)

	delay_ms(100);					// �ӳ�(����Ҫ��)

	OLED_WR_Byte(0xAE, OLED_CMD);	// �ر���ʾ

	OLED_WR_Byte(0x00, OLED_CMD);	// ���õ��е�ַ
	OLED_WR_Byte(0x10, OLED_CMD);	// ���ø��е�ַ
	OLED_WR_Byte(0x40, OLED_CMD);	// ������ʼ�е�ַ
	OLED_WR_Byte(0xB0, OLED_CMD);	// ����ҳ��ַ

	OLED_WR_Byte(0x81, OLED_CMD); 	// �Աȶ����ã�����������
	OLED_WR_Byte(0xFF, OLED_CMD);	// 265

	OLED_WR_Byte(0xA1, OLED_CMD);	// ���ö�(SEG)����ʼӳ���ַ
	OLED_WR_Byte(0xA6, OLED_CMD);	// ������ʾ��0xa7����ʾ

	OLED_WR_Byte(0xA8, OLED_CMD);	// ��������·����16~64��
	OLED_WR_Byte(0x3F, OLED_CMD);	// 64duty

	OLED_WR_Byte(0xC8, OLED_CMD);	// ��ӳ��ģʽ��COM[N-1]~COM0ɨ��

	OLED_WR_Byte(0xD3, OLED_CMD);	// ������ʾƫ��
	OLED_WR_Byte(0x00, OLED_CMD);	// ��ƫ��

	OLED_WR_Byte(0xD5, OLED_CMD);	// ����������Ƶ
	OLED_WR_Byte(0x80, OLED_CMD);	// ʹ��Ĭ��ֵ

	OLED_WR_Byte(0xD9, OLED_CMD);	// ���� Pre-Charge Period
	OLED_WR_Byte(0xF1, OLED_CMD);	// ʹ�ùٷ��Ƽ�ֵ

	OLED_WR_Byte(0xDA, OLED_CMD);	// ���� com pin configuartion
	OLED_WR_Byte(0x12, OLED_CMD);	// ʹ��Ĭ��ֵ

	OLED_WR_Byte(0xDB, OLED_CMD);	// ���� Vcomh���ɵ������ȣ�Ĭ�ϣ�
	OLED_WR_Byte(0x40, OLED_CMD);	// ʹ�ùٷ��Ƽ�ֵ

	OLED_WR_Byte(0x8D, OLED_CMD);	// ����OLED��ɱ�
	OLED_WR_Byte(0x14, OLED_CMD);	// ����ʾ

	OLED_WR_Byte(0xAF, OLED_CMD);	// ����OLED�����ʾ

	OLED_Clear();        			// ����

	OLED_Set_Pos(0, 0); 				// ��������д�����ʼ�С���
	OLED_ShowImage();					// ����ͼƬ
	OLED_Clear();        			// ����
}

// ��ָ�����괦��ʾһ���ַ�
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR OLED_ShowChar(u8 x, u8 y, u8 Show_char) {
	u8 c = 0, i = 0;
	c = Show_char - ' '; 				// ��ȡ�ַ���ƫ����
	if (c >= CHAR_SET_SIZE)
		return;
	if ((x + 8) > Max_Column) {
		x = 0;
		y = y + 2;
	}	// ������������Χ��������2ҳ
	if (y > 6)
		return;
	if (SIZE == 16) 					// �ַ���СΪ[8*16]��һ���ַ�����ҳ
			{
		// ����һҳ
		//-------------------------------------------------------
		OLED_Set_Pos(x, y);						// ���û�����ʼ��
		for (i = 0; i < 8; i++)  						// ѭ��8��(8��)
			OLED_WR_Byte(F8X16[c * 16 + i], OLED_DATA); 	// �ҵ���ģ

		// ���ڶ�ҳ
		//-------------------------------------------------------
		OLED_Set_Pos(x, y + 1); 					// ҳ����1
		for (i = 0; i < 8; i++)  						// ѭ��8��
			OLED_WR_Byte(F8X16[c * 16 + i + 8], OLED_DATA);  		// �ѵڶ�ҳ����
	}
}
//-----------------------------------------------------------------------------

// ��ָ��������ʼ����ʾ�ַ���
//-------------------------------------------------------------------
void ICACHE_FLASH_ATTR OLED_ShowString(u8 x, u8 y, u8 * Show_char) {
	u8 N_Char = 0;		// �ַ����

	while (Show_char[N_Char] != '\0') 	// ����������һ���ַ�
	{
		OLED_ShowChar(x, y, Show_char[N_Char]); 	// ��ʾһ���ַ�

		x += 8;					// ������8��һ���ַ�ռ8��

		if (x >= 128) {
			x = 0;
			y += 2;
		} 	// ��x>=128������һҳ

		N_Char++; 				// ָ����һ���ַ�
	}
}
// ��ָ�����м�λ����ʾ�ַ���������λ���Զ����
//-------------------------------------------------------------------
void ICACHE_FLASH_ATTR OLED_ShowString_LineCenter(u8 y, u8 * Show_char) {
	u8 x, i;		// �ַ����
	u16 char_len = strlen(Show_char);
	u16 max_char_len = Max_Column / 8;
	char_len = (char_len >= max_char_len) ? max_char_len : char_len;
	u16 space_len = max_char_len - char_len;
	for (i = 0, x = 0; i < space_len / 2; i++) {
		OLED_ShowChar(x, y, ' '); 	// ��ʾһ���ո��ַ�
		x += 8;
	}
	for (i = 0; i < char_len; i++) {
		OLED_ShowChar(x, y, Show_char[i]); 	// ��ʾһ���ַ�
		x += 8;
	}
	for (i = 0; i < space_len / 2; i++) {
		OLED_ShowChar(x, y, ' '); 	// ��ʾһ���ո��ַ�
		x += 8;
	}
}
//-------------------------------------------------------------------

// ��ʾͼƬ
//-------------------------------------------------------------------
void ICACHE_FLASH_ATTR OLED_ShowImage(void) {
	uint8 x, y, z;
	uint8 start_x = (Max_Column - IMG_WIDTH) / 2;
	for (y = 0; y < IMG_HEIGTH / 8; y++) {
		OLED_Set_Pos(start_x, y);
		for (x = 0; x < IMG_WIDTH; x++) {
			OLED_WR_Byte(gImage_bilibili[y * IMG_WIDTH + x], OLED_DATA);
		}
	}
	delay_ms(1000);
}
//-------------------------------------------------------------------

// ��ָ��λ����ʾIP��ַ(���ʮ����)
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR OLED_ShowIP(u8 x, u8 y, u8*Array_IP) {
	u8 N_IP_Byte = 0;		// IP�ֽ����

	// ѭ����ʾ4��IP�ֽ�(�ɸߵ����ֽ���ʾ)
	//----------------------------------------------------------
	for (; N_IP_Byte < 4; N_IP_Byte++) {
		// ��ʾ��λ/ʮλ
		//------------------------------------------------------
		if (Array_IP[N_IP_Byte] / 100)		// �жϰ�λ?=0
				{
			OLED_ShowChar(x, y, 48 + Array_IP[N_IP_Byte] / 100);
			x += 8;

			// ��ʾʮλ����λ!=0��
			//---------------------------------------------------------
			//if(Array_IP[N_IP_Byte]%100/10)
			{
				OLED_ShowChar(x, y, 48 + Array_IP[N_IP_Byte] % 100 / 10);
				x += 8;
			}

		}

		// ��ʾʮλ����λ==0��
		//---------------------------------------------------------
		else if (Array_IP[N_IP_Byte] % 100 / 10)		// �ж�ʮλ?=0
				{
			OLED_ShowChar(x, y, 48 + Array_IP[N_IP_Byte] % 100 / 10);
			x += 8;
		}

		// ��ʾ��λ
		//---------------------------------------------------------
		//if(Array_IP[C_IP_Byte]%100%10)
		{
			OLED_ShowChar(x, y, 48 + Array_IP[N_IP_Byte] % 100 % 10);
			x += 8;
		}

		// ��ʾ��.��
		if (N_IP_Byte < 3) {
			OLED_ShowChar(x, y, '.');
			x += 8;
		}
	}
}
//-----------------------------------------------------------------------------

// ��ָ�����괦��ʾһ������
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR OLED_ShowOneHanzi(u8 x, u8 y, hanzi_t hanzi_num) {
	u8 i = 0;
	if (hanzi_num >= HANZI_SET_SIZE)
		return;
	if ((x + 16) >= Max_Column) {
		x = 0;
		y = y + 2;
	}	// ������������Χ��������2ҳ
	if (y > 6)
		return;

	// ����һҳ
	//-------------------------------------------------------
	OLED_Set_Pos(x, y);						// ���û�����ʼ��
	for (i = 0; i < 16; i++)  						// ѭ��16��(16��)
		OLED_WR_Byte(F16X16_Hanzi[hanzi_num][0 + i], OLED_DATA); 	// �ҵ���ģ

	// ���ڶ�ҳ
	//-------------------------------------------------------
	OLED_Set_Pos(x, y + 1); 					// ҳ����1
	for (i = 0; i < 16; i++)  						// ѭ��16��
		OLED_WR_Byte(F16X16_Hanzi[hanzi_num][16 + i], OLED_DATA);  	// �ѵڶ�ҳ����
}

// ��ָ�����괦��ʾһ�麺��
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR OLED_ShowHanzi(u8 x, u8 y, const hanzi_t *arr, u8 len) {
	u8 i = 0;
	for (i = 0; i < len; i++) {
		OLED_ShowOneHanzi(x, y, arr[i]);
		x += 16;  	// ������16��һ������ռ16��
		if (x >= 128) {
			x = 0;
			y += 2;
		} 	// ��x>=128������һҳ
	}
}
// ��ָ�����м�λ����ʾ���֣�����λ���Զ����
//-------------------------------------------------------------------
void ICACHE_FLASH_ATTR OLED_ShowHanzi_LineCenter(u8 y, const hanzi_t *arr,
		u8 len) {
	u8 x, i;		// �ַ����
	u16 max_len = Max_Column / 16;
	len = (len >= max_len) ? max_len : len;
	u16 space_len = (max_len - len) * 2;
	for (i = 0, x = 0; i < space_len / 2; i++) {
		OLED_ShowChar(x, y, ' '); 	// ��ʾһ���ո��ַ�
		x += 8;
	}
	for (i = 0; i < len; i++) {
		OLED_ShowOneHanzi(x, y, arr[i]); 	// ��ʾһ������
		x += 16;
	}
	for (i = 0; i < space_len / 2; i++) {
		OLED_ShowChar(x, y, ' '); 	// ��ʾһ���ո��ַ�
		x += 8;
	}
}
//-----------------------------------------------------------------------------

//==============================================================================
