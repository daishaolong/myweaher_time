#ifndef __OLEDFONT_H
#define __OLEDFONT_H 

//ºº×ÖÐòºÅ
typedef enum
{
	HANZI_SHEN=0,
	HANZI_ZHEN,
	HANZI_JIN,
	HANZI_MING,
	HANZI_HOU,
	HANZI_TIAN,
	HANZI_SET_SIZE
}hanzi_t;

extern const unsigned char  F8X16[];//×Ö·û±í
extern const unsigned short CHAR_SET_SIZE;//×Ö·û¸öÊý
extern const unsigned char 	F16X16_Hanzi[][32];//ºº×Ö±í

#endif	/* __OLEDFONT_H */
