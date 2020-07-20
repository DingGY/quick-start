#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/des.h>
#include <openssl/md5.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#define DEBUG
#ifdef DEBUG
#define encry_debug(a,b) outPutStr(a,b)
#else
#define encry_debug(a,b)
#endif
void outPutStr(unsigned char *str, int len)
{
    int i=0;
    for(i = 0; i < len; i++){
        printf("0x%x ", str[i]);
    }
    printf("\n");
}
void outPutFileStr(unsigned char *str, int len)
{
	int i = 0;
	unsigned char str_out[500];
	char cmd[600];
	for(i=0;i<len;i++){
		sprintf(str_out + i*2, "%02x", str[i]);
	}
	sprintf(cmd, "echo %s > ./encrypt_data", str_out);
	system(cmd);
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
	memset(str, 0, 500);
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
int set_pkcs7_padding(char *input, char *str)
{
	int len = strlen(input);
	int p = 0;
	int i;
	if(len != 0)
	{
		p = 8 - len%8;
	}
	memcpy(str, input, len);
	for(i = 0; i < p; i++)
	{
		str[len] = p;
		len++;
	}
	printf("----------------------pkcs7---------------\n");
	encry_debug(str, len);
	return len/8;
}
int create_encrypt_data(char *random, char *timenow, char *username, char *str)
{
	int i = 0;
	int encrypt_count = 0;
    char encrypt_key[500];
    char pkcs7_str[500];
    DES_key_schedule ks[3];
	memset(str, 0, 500);
	create_encrypt_key(random, timenow, encrypt_key);
	for(i = 0; i < 3; i++)
	{
    	DES_set_key_unchecked(encrypt_key + i*8, &ks[i]);
	}
	encrypt_count = set_pkcs7_padding(username, pkcs7_str);
	for(i = 0; i < encrypt_count;i++)
	{
    	DES_ecb3_encrypt(pkcs7_str + i*8, str + i*8, &ks[0], &ks[1], &ks[2], DES_ENCRYPT);
	}
	return encrypt_count * 8;
}
int  create_encrypt_message(char *username, char *password, char *str)
{
	char random[8], timenow[8], onu_key[16], encry_data[500];
	int len;
	//the str more than 256 byte
	memset(str,0,500);
	// encrypt type default 1
	str[0] = 1;
	strnrand(random,8);
	encry_debug(random, 8);
	byte_time(timenow);
	encry_debug(timenow, 8);
	memcpy(str+1, random, 8);
	memcpy(str+9, timenow, 8);
	create_onu_key(random, timenow, password, onu_key);
	encry_debug(onu_key, 16);
	memcpy(str+17, onu_key, 16);
	len = create_encrypt_data(random, timenow, username, encry_data);
	encry_debug(encry_data, len);
	memcpy(str+33, encry_data, len);
	len += 33;
	encry_debug(str, len);
	return len;
	
}
int create_send_message(char *username, char *password, char *serialnum, int mode, char *str)
{
	char sn[24], encry_data[500];
	int len = 0;
	memset(str, 0, 256);
	memset(sn, 0, 24);
	printf("---------------------begin--------------------\n");
	if(mode == 2)
	{
		if(strlen(username))
		{
			len = create_encrypt_message(username, password, encry_data);
			memcpy(str, encry_data, len);
		}
		else
		{
			if(strlen(serialnum) > 24)
				memcpy(sn, serialnum, 24);
			else
				memcpy(sn, serialnum, strlen(serialnum));
			memcpy(str, sn, 24);
			strcpy(str+24, "admin/admin");
			len = 35;
		}
	}
	else if(mode == 3)
	{
		len = create_encrypt_message(username, password, encry_data);
		memcpy(str, encry_data, len);
	}
	outPutFileStr(str, len);
	return len;
}
int main()
{
	int len = 0;
	int fd;
	int size;
	char str[500];
	char encrypt_data[500];
	len = create_send_message("+dgy666hello@js.comxxx.cn@voip", "123123", "000000000000000000000000", 2, str);
	encry_debug(str, len);
	fd = open("./iencrypt_data", O_RDONLY);
	if(fd == -1)
	{
		printf("open filed!!\n");
		return 0;
	}
	size = read(fd,encrypt_data,sizeof(encrypt_data));
	close(fd);
	size--;
	encrypt_data[size] = '\0';
	printf("--------------------------open file------------------------\n");
	printf("len: %d\n last: 0x%02x\ndata: %s\n", size, encrypt_data[size], encrypt_data);
    return 0;
}
