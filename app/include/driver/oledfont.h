#ifndef __OLEDFONT_H
#define __OLEDFONT_H 

//�������
typedef enum {
	HANZI_SHEN = 0, //��
	HANZI_ZHEN, 	//��
	HANZI_JIN, 		//��
	HANZI_MING, 	//��
	HANZI_HOU, 		//��
	HANZI_TIAN, 	//��
	HANZI_XING, 	//��
	HANZI_QI, 		//��
	HANZI_YI, 		//һ
	HANZI_ER, 		//��
	HANZI_SAN, 		//��
	HANZI_SI, 		//��
	HANZI_WU, 		//��
	HANZI_LIU, 		//��
	HANZI_RI, 		//��
	HANZI_SHI,		//ʪ
	HANZI_DU,		//��
	HANZI_WEN,		//��
	HANZI_SHESHIDU,	//��
	HANZI_QI1,		//��
	HANZI_SET_SIZE
} hanzi_t;

extern const unsigned char F8X16[]; //�ַ���
extern const unsigned short CHAR_SET_SIZE; //�ַ�����
extern const unsigned char F16X16_Hanzi[][32]; //���ֱ�

#endif	/* __OLEDFONT_H */
