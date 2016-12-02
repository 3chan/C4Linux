#ifndef	_TOKEN_H_
#define	_TOKEN_H_

/* 漢字コード判定 */
#ifndef	SJIS

#define	iseuckanji(c)	(0xa1<=(unsigned char)(c&0xff) && (unsigned char)(c&0xff)<=0xfe)
#define	iseuckana(c)	(0x8e==(unsigned char)(c&0xff))

#define	issjiskanji(c)	0
#define	issjiskana(c)	0

#else

#define	iseuckanji(c)	0
#define	iseuckana(c)	0

#define	issjiskanji(c)	((0x81<=(unsigned char)(c&0xff) && (unsigned char)(c&0xff)<=0x9f) \
	|| (0xe0<=(unsigned char)(c&0xff) && (unsigned char)(c&0xff)<=0xfc))
#define	issjiskana(c)	((0xa1<=(unsigned char)(c&0xff) && (unsigned char)(c&0xff)<=0xdf))

#endif


/* トークンデータ */

#define	TOKEN_ALLOC_SIZE	(128)

typedef	struct	{
	char	**token;
	int	size;
	int	no;
}TOKEN;

/* プロトタイプ */
int GetToken(char *buf,int len,TOKEN *token,char *token_separate,char *token_separate_point);
int FreeToken(TOKEN *token);
int CharSmall(char *buf);
int StrCmp(char *a,char *b);
int StrNCmp(char *a,char *b,int n);
int CutCrLf(char *str);

#endif
