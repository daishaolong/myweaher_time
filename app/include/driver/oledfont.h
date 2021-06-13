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
	HANZI_QING,		//晴
	HANZI_JIAN,		//间
	HANZI_DUO,		//多
	HANZI_YUN,		//云
	HANZI_YIN,		//阴
	HANZI_ZHEN1,	//阵
	HANZI_YU,		//雨
	HANZI_LEI,		//雷
	HANZI_XIAO,		//小
	HANZI_ZHONG,	//中
	HANZI_DA,		//大
	HANZI_BAO,		//暴
	HANZI_XUE,		//雪
	HANZI_JIA,		//夹
	HANZI_TE,		//特
	HANZI_BING,		//冰
	HANZI_BAO1,		//雹
	HANZI_BU,		//部
	HANZI_DONG,		//冻
	HANZI_SET_SIZE
} hanzi_t;

extern const unsigned char F8X16[]; //字符表
extern const unsigned short CHAR_SET_SIZE; //字符个数
extern const unsigned char F16X16_Hanzi[][32]; //汉字表

#endif	/* __OLEDFONT_H */
