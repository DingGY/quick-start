#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/des.h>
#include <openssl/md5.h>
#include <time.h>
#define DEBUG
#ifdef DEBUG
#define encry_debug(a,b) outPutStr(a,b)
#else
#define encry_debug(a,b)
#endif
typedef struct
{
	char type;
	char random[8];
	char timestamp[8];
	char onu_key[16];
	char encry_data[16];
} js_op60_encry_msg_t;

typedef struct 
{
	char sn[24];
	js_op60_encry_msg_t encry_msg;
} js_op60_send_msg_t;

void outPutStr(unsigned char *str, int len)
{
    int i=0;
    for(i = 0; i < len; i++){
        printf("0x%x ", str[i]);
    }
    printf("\n");
}
void strnrand(char * str, int len)
{
	int i = 0;
	for(i = 0; i < len; i++)
	{
		str[i] = (char)rand(); 
	}
}
void byte_time(char *str)
{
	memset(str, 0, 8);
	time((time_t *)str);
}
void create_encrypt_key(char *random, char *timenow, char *str)
{
	// the str has 24 byte for 3des key
	memset(str, 0, 24);
	memcpy(str, random, 8);
	memcpy(str+8, timenow, 8);
}
void create_onu_key(char *random, char *timenow, char* password, char *str)
{
	//the str has 16 byte for hash
	MD5_CTX ctx;
	char data[256];
	int pwdlen = strlen(password);
	memset(data, 0, 256);
	memcpy(data, random, 8);
	memcpy(data+8, password, pwdlen);
	memcpy(data+8+pwdlen, timenow, 8);
	MD5_Init(&ctx);
	MD5_Update(&ctx, data, 16 + pwdlen); 
	MD5_Final(str, &ctx);
}
void set_pkcs7_padding(char *input, char *str)
{
	int len = strlen(input);
	int p = 16 - len%16;
	int i;
	if(len > 15)
	{
		len = 15;
		p = 1;
	}
	memcpy(str, input, len);
	for(i = len; i<16; i++)
	{
		str[i] = p;
	}
}
void create_encrypt_data(char *random, char *timenow, char *username, char *data)
{
	int i = 0;
    char encrypt_key[24];
    char pkcs7_str[16];
    DES_key_schedule ks[3];
	memset(data, 0, 16);
	create_encrypt_key(random, timenow, encrypt_key);
	for(i = 0; i < 3; i++)
	{
    	DES_set_key_unchecked(encrypt_key + i*8, &ks[i]);
	}
	set_pkcs7_padding(username, pkcs7_str);
	for(i = 0; i < 2;i++)
	{
    	DES_ecb3_encrypt(pkcs7_str + i*8, data + i*8, &ks[0], &ks[1], &ks[2], DES_ENCRYPT);
	}
}
void create_encrypt_message(char *username, char *password, js_op60_encry_msg_t msg)
{
	
	char random[8], timenow[8], onu_key[16], encry_data[16];
	//the str more than 256 byte
	memset(msg,0,sizeof(js_op60_encry_msg_t));
	// encrypt type default 1
	msg.type = 1;
	strnrand(random,8);
	encry_debug(msg.random, 8);
	byte_time(msg.timestamp);
	encry_debug(msg.timestamp, 8);
	create_onu_key(msg.random, msg.timestamp, password, msg.onu_key);
	encry_debug(msg.onu_key, 16);
	create_encrypt_data(msg.random,  msg.timestamp, username, msg.encry_data);
	encry_debug(msg.encry_data, 16);
	encry_debug(msg, 49);
	
}
int create_send_message(char *username, char *password, char *serialnum, int mode, char *msg)
{
	js_op60_send_msg_t send_msg;
	char sn[24], encry_data[49];
	int len = 0;
	memset(send_msg, 0, sizeof(send_msg));
	memset(sn, 0, 24);
	if(mode == 2)
	{
		if(strlen(serialnum) > 24)
			memcpy(send_msg.sn, serialnum, 24);
		else
			memcpy(send_msg.sn, serialnum, strlen(serialnum));
		memcpy(msg, sn, 24);
		if(strlen(username))
		{
			create_encrypt_message(password, username, send_msg.encry_data);
			memcpy(str+24, send_msg.encry_data, 49);
			len =  73;
		}
		else
		{
			strcpy(str+24, "admin/admin");
			len = 35;
		}
	}
	else if(mode == 3)
	{
		create_encrypt_message(password, username, send_msg.encry_data);
		memcpy(str, send_msg.encry_data, 49);
		len = 49;
	}
	return len;
}
int main()
{
	char str[256];
	create_send_message("dgy666", "123123", "000000000000000000000000", 2, str);
	encry_debug(str, 100);
    return 0;
}
