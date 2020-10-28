#include "crypto.h"
#include <corecrt_malloc.h>
/* Crypto.cpp
 * encryptCluter 密码学代码
 */
unsigned char* crypto_cluster_core(unsigned char* cluster, int bytesPerCluster, unsigned char* key, int mode)
{
	// 分配内存给加解密结果
	u8* result = (u8*)malloc(sizeof(u8) * bytesPerCluster);
	// 装载密钥
	u32 MasterKey[4] = { 0 };
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			MasterKey[i] += key[i * 4 + j] << (j * 8); // 小端
		}
	}

	//密钥扩展
	u32 K[36] = { 0 };
	u32* rk = keyExpansion(MasterKey, K);

	u32 msg_in[4] = { 0 };
	u32 msg_out[4] = { 0 };

	// 确定每个簇内的块数（每块128bits，16Bytes）,且除法结果一定为整数（扇区大小必为256bit倍数），所以无需考虑padding
	int blockPerCluster = bytesPerCluster / 16;

	for (int i = 0; i < blockPerCluster; i++)
	{
		bytes16_to_word4(cluster + i * 16, msg_in);
		if (mode == 0)
			SM4_block_enc(msg_in, msg_out, rk);
		else
			SM4_block_dec(msg_in, msg_out, rk);
		word4_to_bytes16(msg_out, result + i * 16);
	}
	return result;
}


unsigned char* enc_cluster(unsigned char* cluster, int bytesPerCluster, unsigned char* key)
{
	return crypto_cluster_core(cluster, bytesPerCluster, key, 0);
}

unsigned char* dec_cluster(unsigned char* cluster, int bytesPerCluster, unsigned char* key)
{
	return crypto_cluster_core(cluster, bytesPerCluster, key, 1);
}

// 循环左移 a是源操作数 b是左移位数
u32 cycleShiftLeft(u32 a, u32 b)
{
	b %= 32;
	u32 tmp = a >> (32 - b);
	return (a << b) ^ tmp;
}


// 非线性变换t
u32 nonlinear_transform(u32 a)
{
	u32 result = 0;
	u8 tmp = a >> 24;
	result += SboxTable[tmp / 16][tmp % 16];
	result <<= 8;
	tmp = a >> 16;
	result += SboxTable[tmp / 16][tmp % 16];
	result <<= 8;
	tmp = a >> 8;
	result += SboxTable[tmp / 16][tmp % 16];
	result <<= 8;
	tmp = a;
	result += SboxTable[tmp / 16][tmp % 16];
	return result;
}


// 线性变换L
u32 inline L(u32 a)
{
	return a ^ cycleShiftLeft(a, 2) ^ cycleShiftLeft(a, 10) ^ cycleShiftLeft(a, 18) ^ cycleShiftLeft(a, 24);
}


//合成变换 T
u32 inline T(u32 a)
{
	return L(nonlinear_transform(a));
}

// 线性变换L-密钥扩展
u32 inline L_key(u32 a)
{
	return a ^ cycleShiftLeft(a, 13) ^ cycleShiftLeft(a, 23);
}

//合成变换 T-密钥扩展
u32 inline T_key(u32 a)
{
	return L_key(nonlinear_transform(a));
}

// 输入master Key 4*32bits ， K 36 * 32bits， 输出rk的基地址（其实就是K+4）
u32* keyExpansion(u32* masterKey, u32* K)
{
	K[0] = masterKey[0] ^ FK[0];
	K[1] = masterKey[1] ^ FK[1];
	K[2] = masterKey[2] ^ FK[2];
	K[3] = masterKey[3] ^ FK[3];
	for (int i = 0; i < 32; i++)
	{
		K[i + 4] = K[i] ^ T_key(K[i + 1] ^ K[i + 2] ^ K[i + 3] ^ CK[i]);
	}
	return K + 4;
}


// 输入in 4个word rk 32个word 输出 out4个wod
void SM4_block_enc(u32* in, u32* out, u32* rk)
{
	u32 X[36] = { 0 };
	X[0] = in[0];
	X[1] = in[1];
	X[2] = in[2];
	X[3] = in[3];
	for (int i = 0; i < 32; i++)
	{
		X[i + 4] = X[i] ^ T(X[i + 1] ^ X[i + 2] ^ X[i + 3] ^ rk[i]);
	}
	out[0] = X[35];
	out[1] = X[34];
	out[2] = X[33];
	out[3] = X[32];
}


// 输入in 4个word rk 32个word 输出 out4个wod
void SM4_block_dec(u32* in, u32* out, u32* rk)
{
	u32 X[36] = { 0 };
	X[0] = in[0];
	X[1] = in[1];
	X[2] = in[2];
	X[3] = in[3];
	for (int i = 0; i < 32; i++)
	{
		X[i + 4] = X[i] ^ T(X[i + 1] ^ X[i + 2] ^ X[i + 3] ^ rk[31 - i]);
	}
	out[0] = X[35];
	out[1] = X[34];
	out[2] = X[33];
	out[3] = X[32];
}

unsigned char* RC4_for_MTR_data(unsigned char* in, int bytesToProcess, unsigned char* key)
{
	unsigned char S[256] = "";
	unsigned char j = 0;
	//为S盒赋初始值
	for (int i = 0; i < 256; i++) {
		S[i] = i;
	}
	//打乱S盒
	for (int i = 0; i < 256; i++) {
		j = (j + S[j] + key[i % 16]) % 256; // key 长度写死 16字节
		swap_ch(S + i, S + j);
	}
	unsigned char* result = (unsigned char*)malloc(sizeof(unsigned char) * bytesToProcess);
	unsigned i = 0;
	for (int k = 0; k < bytesToProcess; k++)
	{
		i = (i + 1) % 256;
		j = (j + S[i]) % 256;
		swap_ch(S + i, S + j);
		result[k] = in[k] ^ S[(S[i] + S[j]) % 256];
	}
	return result;
}


// 将16个字节转成4个word
void bytes16_to_word4(u8* in, u32* out)
{
	out[0] = (u32)in[0] << 24 ^ (u32)in[1] << 16 ^ (u32)in[2] << 8 ^ (u32)in[3];
	out[1] = (u32)in[4] << 24 ^ (u32)in[5] << 16 ^ (u32)in[6] << 8 ^ (u32)in[7];
	out[2] = (u32)in[8] << 24 ^ (u32)in[9] << 16 ^ (u32)in[10] << 8 ^ (u32)in[11];
	out[3] = (u32)in[12] << 24 ^ (u32)in[13] << 16 ^ (u32)in[14] << 8 ^ (u32)in[15];
	return;
}

// 将4个word转成16个字节
void word4_to_bytes16(u32* in, u8* out)
{
	out[0] = in[0] >> 24;
	out[1] = in[0] >> 16;
	out[2] = in[0] >> 8;
	out[3] = in[0];
	out[4] = in[1] >> 24;
	out[5] = in[1] >> 16;
	out[6] = in[1] >> 8;
	out[7] = in[1];
	out[8] = in[2] >> 24;
	out[9] = in[2] >> 16;
	out[10] = in[2] >> 8;
	out[11] = in[2];
	out[12] = in[3] >> 24;
	out[13] = in[3] >> 16;
	out[14] = in[3] >> 8;
	out[15] = in[3];
	return;
}

void swap_ch(unsigned char* a, unsigned char* b) {
	unsigned char c = *a;
	*a = *b;
	*b = c;
	return;
}