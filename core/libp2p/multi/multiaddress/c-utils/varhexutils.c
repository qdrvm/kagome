#ifndef VARHEXUTILS
#define VARHEXUTILS

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <inttypes.h>
#include "libp2p/multi/multiaddress/c-utils/varint.h"
#include "libp2p/multi/multiaddress/c-utils/varhexutils.h"

int8_t Var_Bytes_Count(uint8_t * countbytesofthis)
{
	static int8_t xrzk_bytescnt = 0;
	for(int8_t i=0; i<10; i++)
	{
		if(countbytesofthis[i] != 0)
		{
			xrzk_bytescnt++;
		}
	}
	return xrzk_bytescnt;
}
uint8_t * Num_To_Varint_64(uint64_t TOV64INPUT) //UINT64_T TO VARINT
{
	static uint8_t buffy_001[60] = {0};
	uvarint_encode64(TOV64INPUT, buffy_001, 60);
	return buffy_001;
}
uint8_t * Num_To_Varint_32(uint32_t TOV32INPUT) // UINT32_T TO VARINT
{
	static uint8_t buffy_032[60] = {0};
	uvarint_encode32(TOV32INPUT, buffy_032, 60);
	return buffy_032;
}
uint64_t * Varint_To_Num_64(uint8_t TON64INPUT[60]) //VARINT TO UINT64_t
{
	static uint64_t varintdecode_001 = 0;
	uvarint_decode64(TON64INPUT, 60, &varintdecode_001);
	return &varintdecode_001;
}
uint32_t * Varint_To_Num_32(uint8_t TON32INPUT[60]) //VARINT TO UINT32_t
{
	static uint32_t varintdecode_032 = 0;
	uvarint_decode32(TON32INPUT, 60, &varintdecode_032);
	return &varintdecode_032;
}
/**
 * Converts a 64 bit integer into a hex string
 * @param int2hex the 64 bit integer
 * @returns a hex representation as a string (leading zero if necessary)
 */
char * Int_To_Hex(uint64_t int2hex) //VAR[binformat] TO HEX
{
	static char result[50];
	memset(result, 0, 50);
	sprintf(result, "%02lX", int2hex);
	int slen = strlen(result);
	if (slen % 2 != 0) {
		for(int i = slen; i >= 0; --i) {
			result[i+1] = result[i];
		}
		result[0] = '0';
	}
	return result;
}

uint64_t Hex_To_Int(char * hax)
{
	char * hex = NULL;
	hex=hax;
    uint64_t val = 0;
    while (*hex)
	{
		// get current character then increment
        uint8_t byte = *hex++;
		// transform hex character to the 4bit equivalent number, using the ascii table indexes
		if (byte >= '0' && byte <= '9') byte = byte - '0';
		else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
		else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;
		// shift 4 to make space for new digit, and add the 4 bits of the new digit
		val = (val << 4) | (byte & 0xF);
	}
	return val;
}

/**
 * Convert a byte array into a hex byte array
 * @param in the incoming byte array
 * @param in_size the size of in
 * @param out the resultant array of hex bytes
 */
void vthconvert(const unsigned char* in, int in_size, unsigned char** out)
{
	*out = (unsigned char*)malloc( (in_size * 2) + 1);
	memset(*out, 0, (in_size * 2) + 1);
	unsigned char *ptr = *out;

	for (int i = 0; i < in_size; i++) {
		sprintf((char*)&ptr[i * 2], "%02x", in[i]);
	}
}

/**
 * Convert binary array to array of hex values
 * @param incoming the binary array
 * @param incoming_size the size of the incoming array
 * @returns the allocated array
 */
unsigned char * Var_To_Hex(const unsigned char *incoming, int incoming_size)
{
	if(incoming != NULL)
	{
		unsigned char* retVal = NULL;
		// this does the real work
		vthconvert(incoming, incoming_size, &retVal);

		// we can't return an array that will be deallocated!
		return retVal;
	}
	return NULL;
}

/**
 * Turn a hex string into a byte array
 * @param incoming a string of hex values
 * @param num_bytes the size of the result
 * @returns a pointer to the converted value
 */
unsigned char* Hex_To_Var(const char* incoming, size_t* num_bytes) //HEX TO VAR[BINFORMAT]
{
	// the return value
	unsigned char* retVal = NULL;
	int incoming_size = strlen(incoming);
	*num_bytes = incoming_size / 2;
	retVal = (unsigned char*)malloc(*num_bytes);

	char code[3];
	code[2]='\0';
	int pos = 0;
	for(int i=0; i < incoming_size; i += 2)
	{
		code[0] = incoming[i];
		code[1] = incoming[i+1];
    	uint64_t lu = strtol(code, NULL, 16);
		retVal[pos] = (unsigned int)lu;
		pos++;
	}
	return retVal;
}
//
void convert(char * convert_result, uint8_t * buf)					//Both of them read them properly.
{
	char conv_proc[800]="\0";
	bzero(conv_proc,800);
	int i;
	for(i=0; i < 10; i++)
	{
		sprintf (conv_proc, "%02X", buf[i]);
		//printf("%d:%d\n",i, buf[i]);
		strcat(convert_result, conv_proc);
	}
}
char * Num_To_HexVar_64(uint64_t TOHVINPUT) //UINT64 TO HEXIFIED VAR
{											//Code to varint - py
	static char convert_result[800]="\0";//Note that the hex resulted from this will differ from py
	bzero(convert_result,800);
	memset(convert_result,0,sizeof(convert_result));//But if you make sure the string is always 20 chars in size
	uint8_t buf[400] = {0};
	bzero(buf,400);
	uvarint_encode64(TOHVINPUT, buf, 800);
	convert(convert_result,buf);
	return convert_result;
}
void convert2(char * convert_result2, uint8_t * bufhx)
{
	uint8_t * buf = NULL;
	buf = bufhx;
	char conv_proc[4]="\0";
	conv_proc[3] = '\0';
	bzero(conv_proc, 3);
	int i;
	for(i=0; i == 0; i++)
	{
		sprintf (conv_proc, "%02X", buf[i]);
		//printf("aaaaaaaaaaah%d:%d\n",i, buf[i]);
		strcat(convert_result2, conv_proc);
	}
	buf = NULL;
}
char * Num_To_HexVar_32(uint32_t TOHVINPUT) //UINT32 TO HEXIFIED VAR
{											//Code to varint - py
	static char convert_result2[3]="\0";
	bzero(convert_result2,3);
	convert_result2[2] = '\0';
	memset(convert_result2,0,sizeof(convert_result2));
	uint8_t buf[1] = {0};
	bzero(buf,1);
	uvarint_encode32(TOHVINPUT, buf, 1);
	convert2(convert_result2,buf);
	return convert_result2;
}

uint64_t HexVar_To_Num_64(char * theHEXstring) //HEXIFIED VAR TO UINT64_T
{											   //Varint to code - py
	uint8_t buffy[400] = {0};
	char codo[800] = "\0";
	bzero(codo,800);
	strcpy(codo, theHEXstring);
	char code[3] = "\0";
	int x = 0;
	for(int i= 0;i<399;i++)
	{
		strncpy(&code[0],&codo[i],1);
		strncpy(&code[1],&codo[i+1],1);
    	char *ck = NULL;
    	uint64_t lu = 0;
    	lu=strtoul(code, &ck, 16);
		buffy[x] = lu;
		i++;
		x++;
	}
	static uint64_t decoded;
	uvarint_decode64 (buffy, 400, &decoded);
	return decoded;
}
uint32_t HexVar_To_Num_32(char theHEXstring[]) //HEXIFIED VAR TO UINT32_T
{												//Varint to code py
	uint8_t buffy[400] = {0};
	bzero(buffy,400);
	char codo[800] = "\0";
	bzero(codo,800);
	strcpy(codo, theHEXstring);
	char code[4] = "\0";
	bzero(code,3);
	code[3] = '\0';
	int x = 0;
	for(int i= 0;i<399;i++)
	{
		strncpy(&code[0],&codo[i],1);
		strncpy(&code[1],&codo[i+1],1);
    	char *ck = NULL;
    	uint32_t lu = {0};
    	lu=strtoul(code, &ck, 16);
		buffy[x] = lu;
		i++;
		x++;
	}
	static uint32_t decoded;
	uvarint_decode32 (buffy, 10, &decoded);
	return decoded;
}
#endif
