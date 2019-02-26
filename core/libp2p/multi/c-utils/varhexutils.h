#ifndef VARHEXUTILS
#define VARHEXUTILS

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libp2p/multi/c-utils/endian.h"
#include "libp2p/multi/c-utils/varint.h"

int8_t Var_Bytes_Count(uint8_t *countbytesofthis);

uint8_t *Num_To_Varint_64(uint64_t TOV64INPUT);  // UINT64_T TO VARINT

uint8_t *Num_To_Varint_32(uint32_t TOV32INPUT);  // UINT32_T TO VARINT

uint64_t *Varint_To_Num_64(uint8_t TON64INPUT[60]);  // VARINT TO UINT64_t

uint32_t *Varint_To_Num_32(uint8_t TON32INPUT[60]);  // VARINT TO UINT32_t

//
char *Int_To_Hex(uint64_t int2hex);  // VAR[binformat] TO HEX

uint64_t Hex_To_Int(char *hax);

/**
 * Convert binary array to array of hex values
 * @param incoming the binary array
 * @param incoming_size the size of the incoming array
 * @returns the allocated array
 */
unsigned char *Var_To_Hex(const char *incoming, int incoming_size);

/**
 * Turn a hex string into a byte array
 * @param incoming a string of hex values
 * @param num_bytes the size of the result
 * @returns a pointer to the converted value
 */
unsigned char *Hex_To_Var(char *Hexstr, size_t *num_bytes);

//
void convert(char *convert_result,
             uint8_t *buf);  // Both of them read them properly.

char *Num_To_HexVar_64(uint64_t TOHVINPUT);  // UINT64 TO HEXIFIED VAR

void convert2(char *convert_result2, uint8_t *bufhx);

char *Num_To_HexVar_32(uint32_t TOHVINPUT);  // UINT32 TO HEXIFIED VAR

uint64_t HexVar_To_Num_64(char *theHEXstring);  // HEXIFIED VAR TO UINT64_T

uint32_t HexVar_To_Num_32(char theHEXstring[]);  // HEXIFIED VAR TO UINT32_T

#endif
