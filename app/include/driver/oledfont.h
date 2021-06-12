#ifndef __OLEDFONT_H
#define __OLEDFONT_H 

//汉字序号
typedef enum {
	HANZI_SHEN = 0, //深
	HANZI_ZHEN, 	//圳
	HANZI_JIN, 		//今
	HANZI_MING, 	//明
	HANZI_HOU, 		//后
	HANZI_TIAN, 	//天
	HANZI_XING, 	//星
	HANZI_QI, 		//期
	HANZI_YI, 		//一
	HANZI_ER, 		//二
	HANZI_SAN, 		//三
	HANZI_SI, 		//四
	HANZI_WU, 		//五
	HANZI_LIU, 		//六
	HANZI_RI, 		//日
	HANZI_SHI,		//湿
	HANZI_DU,		//度
	HANZI_WEN,		//温
	HANZI_SHESHIDU,	//℃
	HANZI_QI1,		//气
	HANZI_SET_SIZE
} hanzi_t;

extern const unsigned char F8X16[]; //字符表
extern const unsigned short CHAR_SET_SIZE; //字符个数
extern const unsigned char F16X16_Hanzi[][32]; //汉字表

#endif	/* __OLEDFONT_H */
