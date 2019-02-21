#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <inttypes.h>
#include <ctype.h>
#include "libp2p/multi/multiaddress/c-utils/base58.h"
#include "libp2p/multi/multiaddress/c-utils/varhexutils.h"
#include "libp2p/multi/multiaddress/c-utils/protocols.h"
#include "libp2p/multi/multiaddress/c-utils/protoutils.h"

extern char *strtok_r(char *, const char *, char **);

//////////////////////////////////////////////////////////
char ASCII2bits(char ch) {
   if (ch >= '0' && ch <= '9') {
      return (ch - '0');
   } else if (ch >= 'a' && ch <= 'z') {
      return (ch - 'a') + 10;
   } else if (ch >= 'A' && ch <= 'Z') {
      return (ch - 'A') + 10;
   }
   return 0; // fail
}

void hex2bin (char *dst, char *src, int len)
{
    while (len--) {
        *dst    = ASCII2bits(*src++) << 4; // higher bits
        *dst++ |= ASCII2bits(*src++);      // lower  bits
    }
}

char bits2ASCII(char b) {
   if (b >= 0 && b < 10) {
      return (b + '0');
   } else if (b >= 10 && b <= 15) {
      return (b - 10 + 'a');
   }
   return 0; // fail
}

void bin2hex (char *dst, char *src, int len)
{
    while (len--) {
        *dst++ = bits2ASCII((*src >> 4) & 0xf);    // higher bits
        *dst++ = bits2ASCII(*src++ & 0xf); // lower  bits
    }
    *dst = '\0';
}
//////////////////////////////////////////////////////////
//IPv4 VALIDATOR
#define DELIM "."

/* return 1 if string contain only digits, else return 0 */
int valid_digit(char *ip_str)
{
	int err = 0;
    while (*ip_str) {
        if (*ip_str >= '0' && *ip_str <= '9')
            ++ip_str;
        else
            return 0;
    }
    return 1;
}

/* return 1 if IP string is valid, else return 0 */
int is_valid_ipv4(char *ip_str)
{
    int i, num, dots = 0;
    char *ptr;
	int err=0;
    if (ip_str == NULL)
        err = 1;

    // See following link for strtok()
    // http://pubs.opengroup.org/onlinepubs/009695399/functions/strtok_r.html
    ptr = strtok(ip_str, DELIM);

    if (ptr == NULL)
        err = 1;

    while (ptr)
	{

        /* after parsing string, it must contain only digits */
        if (!valid_digit(ptr))
            err = 1;

        num = atoi(ptr);

        /* check for valid IP */
        if (num >= 0 && num <= 255) {
            /* parse remaining string */
            ptr = strtok(NULL, DELIM);
            if (ptr != NULL)
                ++dots;
        } else
            err = 1;
    }

    /* valid IP string must contain 3 dots */
    if (dots != 3)
	{
        err = 1;
	}
	if(err == 0)
	{
    return 1;
	}
	else
	{
		return 0;
	}
}


//////////////IPv6 Validator
#define MAX_HEX_NUMBER_COUNT 8

int ishexdigit(char ch)
{
   if((ch>='0'&&ch<='9')||(ch>='a'&&ch<='f')||(ch>='A'&&ch<='F'))
      return(1);
   return(0);
}

int is_valid_ipv6(char *str)
{
   int hdcount=0;
   int hncount=0;
   int err=0;
   int packed=0;

   if(*str==':')
   {
      str++;
      if(*str!=':')
         return(0);
      else
      {
         packed=1;
         hncount=1;
         str++;

         if(*str==0)
            return(1);
      }
   }

   if(ishexdigit(*str)==0)
   {
      return(0);
   }

   hdcount=1;
   hncount=1;
   str++;

   while(err==0&&*str!=0)
   {
      if(*str==':')
      {
         str++;
         if(*str==':')
         {
           if(packed==1)
              err=1;
           else
           {
              str++;

          if(ishexdigit(*str) || (*str==0 && hncount < MAX_HEX_NUMBER_COUNT ))
          {
             packed=1;
             hncount++;

             if(ishexdigit(*str))
             {
                if(hncount==MAX_HEX_NUMBER_COUNT)
                {
                   err=1;
                } else
                {
                   hdcount=1;
                   hncount++;
                   str++;
                }
             }
          } else
          {
             err=1;
          }
       }
    }
	else
    {
           if(!ishexdigit(*str))
           {
              err=1;
           } else
           {
              if(hncount==MAX_HEX_NUMBER_COUNT)
              {
                 err=1;
              } else
              {
                  hdcount=1;
                  hncount++;
                  str++;
              }
           }
        }
     }
	 else
     {
        if(ishexdigit(*str))
        {
           if(hdcount==4)
              err=1;
           else
           {
              hdcount++;
              str++;
           }
         } else
            err=1;
     }
   }

   if(hncount<MAX_HEX_NUMBER_COUNT&&packed==0)
      err=1;

    return(err==0);
}
uint64_t ip2int(const char * ipconvertint)
{
	uint64_t final_result =0;
	char * iproc;
	int ipat1=0;
	int ipat2=0;
	int ipat3=0;
	int ipat4=0;
	char ip[16];
	strcpy(ip, ipconvertint);
	iproc = strtok (ip,".");
	for(int i=0; i<4;i++)
	{
		switch(i)
		{
			case 0:
			{
				ipat1 = atoi(iproc);
				break;
			}
			case 1:
			{
				ipat2 = atoi(iproc);
				break;
			}
			case 2:
			{
				ipat3 = atoi(iproc);
				break;
			}
			case 3:
			{
				ipat4 = atoi(iproc);
				break;
			}
			default:
			{
				printf("Somebody misplaced an int\n");
				break;
			}
		}
		iproc = strtok (NULL,".");
	}
	final_result =  ((ipat1*pow(2,24))+(ipat2*pow(2,16))+(ipat3*pow(2,8))+ipat4*1);
	return final_result;
}
char * int2ip(int inputintip)
{
	uint32_t ipint = inputintip;
	static char xxx_int2ip_result[16] = "\0";
	bzero(xxx_int2ip_result,16);
	uint32_t ipint0 = (ipint >> 8*3) % 256;
	uint32_t ipint1 = (ipint >> 8*2) % 256;
	uint32_t ipint2 = (ipint >> 8*1) % 256;
	uint32_t ipint3 = (ipint >> 8*0) % 256;
	sprintf(xxx_int2ip_result, "%d.%d.%d.%d", ipint0,ipint1,ipint2,ipint3);
	return xxx_int2ip_result;
}

/**
 * Unserialize the bytes into a string
 * @param results where to put the resultant string
 * @param in_bytes the bytes to unserialize
 * @param in_bytes_size the length of the bytes array
 * @returns 0 on error, otherwise 1
 */
char* bytes_to_string(char** buffer, const uint8_t* in_bytes, int in_bytes_size)
{
	uint8_t * bytes = NULL;
	char *results = NULL;
	int size = in_bytes_size;
	struct ProtocolListItem* head = NULL;
	char hex[(in_bytes_size*2)+1];
	//Positioning for memory jump:
	int lastpos = 0;
	char pid[3];

	// set up variables
	load_protocols(&head);
	memset(hex, 0, (in_bytes_size * 2) + 1);
	char* tmp = (char*)Var_To_Hex((char*)in_bytes, size);
	memcpy(hex, tmp, in_bytes_size * 2);
	free(tmp);
	pid[2] = 0;

	// allocate memory for results
	*buffer = malloc(800);
	results = *buffer;
	memset(results, 0, 800);


	//Process Hex String
	NAX:
	//Stage 1 ID:
	pid[0] = hex[lastpos];
	pid[1] = hex[lastpos+1];

	int protocol_int = Hex_To_Int(pid);
	struct Protocol* protocol = proto_with_deccode(head, protocol_int);
	if(protocol != NULL)
	{
		//////////Stage 2: Address
		if(strcmp(protocol->name,"ipfs")!=0)
		{
			lastpos = lastpos+2;
			char address[(protocol->size/4)+1];
			memset(address, 0, (protocol->size / 4) + 1);
			memcpy(address, &hex[lastpos], protocol->size / 4);
			//////////Stage 3 Process it back to string
			lastpos= lastpos+(protocol->size/4);

			//////////Address:
			//Keeping Valgrind happy
			char name[30];
			bzero(name,30);
			strcpy(name, protocol->name);
			//
			strcat(results, "/");
			strcat(results, name);
			strcat(results, "/");
			if(strcmp(name, "ip4")==0)
			{
				strcat(results,int2ip(Hex_To_Int(address)));
			}
			else if(strcmp(name, "tcp")==0)
			{
				char a[5];
					sprintf(a,"%lu",Hex_To_Int(address));
					strcat(results,a);
			}
			else if(strcmp(name, "udp")==0)
			{
				char a[5];
				sprintf(a,"%lu",Hex_To_Int(address));
				strcat(results,a);
			}
			/////////////Done processing this, move to next if there is more.
			if(lastpos<size*2)
			{
				goto NAX;
			}
		}
		else//IPFS CASE
		{
			lastpos = lastpos + 4;
			//fetch the size of the address based on the varint prefix
			char prefixedvarint[3];
			memset(prefixedvarint, 0, 3);
			memcpy(prefixedvarint, &hex[lastpos-2], 2);
			int addrsize = HexVar_To_Num_32(prefixedvarint);

			// get the ipfs address as hex values
			unsigned char IPFS_ADDR[addrsize+1];
			memset(IPFS_ADDR, 0, addrsize + 1);
			memcpy(IPFS_ADDR, &hex[lastpos], addrsize);
			// convert the address from hex values to a binary array
			size_t num_bytes = 0;
			unsigned char* addrbuf = Hex_To_Var((char*)IPFS_ADDR, &num_bytes);
			size_t b58_size = strlen((char*)IPFS_ADDR);
			unsigned char b58[b58_size];
			memset(b58, 0, b58_size);
			unsigned char *ptr_b58 = b58;
			int returnstatus = multiaddr_encoding_base58_encode(addrbuf, num_bytes, &ptr_b58, &b58_size);
			free(addrbuf);
			if(returnstatus == 0)
			{
                char *error = (char*) malloc(sizeof(char) * (38 + addrsize + 1));
                strcpy(error, "unable to base58 encode MultiAddress: ");
                strcat(error, IPFS_ADDR);
                return error;
			}
			strcat(results, "/");
			strcat(results, protocol->name);
			strcat(results, "/");
			strcat(results, (char*)b58);
		}
	}
	strcat(results, "/");
	unload_protocols(head);
	return NULL;
}
//

/**
 * Convert an address string to a byte representation
 * @param protocol the protocol to use
 * @param incoming the byte array
 * @param incoming_size the size of the byte array
 * @param results the results
 * @param results_size the size of the results
 * @returns the results array
 */
char* address_string_to_bytes(struct Protocol * protocol, const char *incoming, size_t incoming_size, char** results, int* results_size)
{
	static char astb__stringy[800] = "\0";
	memset(astb__stringy, 0, 800);

	int code = 0;
	code = protocol->deccode;

	switch(code)
	{
		case 4://IPv4
		{
			char testip[16] = "\0";
			bzero(testip,16);
			strcpy(testip,incoming);
			if(is_valid_ipv4(testip)==1)
			{
				uint64_t iip = ip2int(incoming);
				strcpy(astb__stringy,Int_To_Hex(iip));
				protocol = NULL;
				*results = malloc(strlen(astb__stringy));
				memcpy(*results, astb__stringy, strlen(astb__stringy));
				*results_size = strlen(astb__stringy);
				return *results;
			}
			else
			{
				return "ERR";
			}
			break;
		}
		case 41://IPv6 Must be done
		{
			return "ERR";
			break;
		}
		case 6: //Tcp
		{
			if(atoi(incoming)<65536&&atoi(incoming)>0)
			{
				static char himm_woot[5] = "\0";
				bzero(himm_woot, 5);
				strcpy(himm_woot, Int_To_Hex(atoi(incoming)));
				if(himm_woot[2] == '\0')
				{//manual switch
					char swap0='0';
					char swap1='0';
					char swap2=himm_woot[0];
					char swap3=himm_woot[1];
					himm_woot[0] = swap0;
					himm_woot[1] = swap1;
					himm_woot[2] = swap2;
					himm_woot[3] = swap3;
				}
				else if(himm_woot[3] == '\0')
				{
					char swap0='0';
					char swap1=himm_woot[0];
					char swap2=himm_woot[1];
					char swap3=himm_woot[2];
					himm_woot[0] = swap0;
					himm_woot[1] = swap1;
					himm_woot[2] = swap2;
					himm_woot[3] = swap3;
				}
				himm_woot[4]='\0';
				*results = malloc(5);
				*results_size = 5;
				memcpy(*results, himm_woot, 5);
				return *results;
			}
			else
			{
				return "ERR";
			}
			break;
		}
		case 17: //Udp
		{
			if(atoi(incoming)<65536&&atoi(incoming)>0)
			{
				static char himm_woot2[5] = "\0";
				bzero(himm_woot2, 5);
				strcpy(himm_woot2, Int_To_Hex(atoi(incoming)));
				if(himm_woot2[2] == '\0')
				{//Manual Switch2be
					char swap0='0';
					char swap1='0';
					char swap2=himm_woot2[0];
					char swap3=himm_woot2[1];
					himm_woot2[0] = swap0;
					himm_woot2[1] = swap1;
					himm_woot2[2] = swap2;
					himm_woot2[3] = swap3;
				}
				else if(himm_woot2[3] == '\0')
				{//Manual switch
					char swap0='0';
					char swap1=himm_woot2[0];
					char swap2=himm_woot2[1];
					char swap3=himm_woot2[2];
					himm_woot2[0] = swap0;
					himm_woot2[1] = swap1;
					himm_woot2[2] = swap2;
					himm_woot2[3] = swap3;
				}
				himm_woot2[4]='\0';
				*results = malloc(5);
				*results_size = 5;
				memcpy(*results, himm_woot2, 5);
				return *results;
			}
			else
			{
				return "ERR";
			}
			break;
		}
		case 33://dccp
		{
			return "ERR";
		}
		case 132://sctp
		{
			return "ERR";
		}
		case 301://udt
		{
			return "ERR";
		}
		case 302://utp
		{
			return "ERR";
		}
		case 42://IPFS - !!!
		{
			// decode the base58 to bytes
			char * incoming_copy = NULL;
			incoming_copy = (char*)incoming;
			size_t incoming_copy_size = strlen(incoming_copy);
			size_t result_buffer_length = multiaddr_encoding_base58_decode_max_size((unsigned char*)incoming_copy);
			unsigned char result_buffer[result_buffer_length];
			unsigned char* ptr_to_result = result_buffer;
			memset(result_buffer, 0, result_buffer_length);
			// now get the decoded address
			int return_value = multiaddr_encoding_base58_decode(incoming_copy, incoming_copy_size, &ptr_to_result, &result_buffer_length);
			if (return_value == 0)
			{
				return "ERR";
			}

			// throw everything in a hex string so we can debug the results
			char addr_encoded[300];
			memset(addr_encoded, 0, 300);
			int ilen = 0;
			for(int i = 0; i < result_buffer_length; i++)
			{
				// get the char so we can see it in the debugger
				char miu[3];
				sprintf(miu,"%02x", ptr_to_result[i]);
				strcat(addr_encoded, miu);
			}
			ilen = strlen(addr_encoded);
			char prefixed[3];
			memset(prefixed, 0, 3);
			strcpy(prefixed,Num_To_HexVar_32(ilen));
			*results_size = ilen + 3;
			*results = malloc(*results_size);
			memset(*results, 0, *results_size);
			strcat(*results, prefixed); // 2 bytes
			strcat(*results, addr_encoded); // ilen bytes + null terminator
			return *results;
		}
		case 480://http
		{
			return "ERR";
		}
		case 443://https
		{
			return "ERR";
		}
		case 477://ws
		{
			return "ERR";
		}
		case 444://onion
		{
			return "ERR";
		}
		case 275://libp2p-webrtc-star
		{
			return "ERR";
		}
		default:
		{
			printf("NO SUCH PROTOCOL!\n");
			return "ERR";
		}
	}
}

/**
 * convert a string address into bytes
 * @param finalbytes the destination
 * @param realbbsize the ultimate size of the destination
 * @param strx the incoming string
 * @param strsize the string length
 * @return empty string in case of success, error otherwise
 */
char* string_to_bytes(uint8_t** finalbytes, size_t* realbbsize, const char* strx, size_t strsize)
{
	if(strx[0] != '/')
	{
		char *error = (char*) malloc(sizeof(char) * (45 + strsize));
		strcpy(error, "address must start with '/', passed address: ");
		strcat(error, strx);
		return error;
	}

	//Initializing variables to store our processed HEX in:
	char processed[800];//HEX CONTAINER
	bzero(processed,800);

	//Now Setting up variables for calculating which is the first
	//and second word:
	int firstorsecond = 1; //1=Protocol && 2 = Address

	// copy input so as to not harm it
	char pstring[strsize + 1];
	strcpy(pstring,strx);

	// load up the list of protocols
	struct ProtocolListItem* head = NULL;
	load_protocols(&head);

	//Starting to extract words and process them:
	char * wp;
	char * end;
	wp=strtok_r(pstring,"/",&end);
	struct Protocol * protx;
	while(wp)
	{
		if(firstorsecond==1)//This is the Protocol
		{
			protx = proto_with_name(head, wp);
			if(protx != NULL)
			{
				strcat(processed, Int_To_Hex(protx->deccode));
				firstorsecond=2;//Since the next word will be an address
			}
			else
			{
				char *error = (char*) malloc(sizeof(char) * (34 + strsize));
				strcpy(error, "no such protocol, passed address: ");
				strcat(error, strx);
				return error;
			}
		}
		else//This is the address
		{
			char* s_to_b = NULL;
			int s_to_b_size = 0;
			if( strcmp(address_string_to_bytes(protx, wp,strlen(wp), &s_to_b, &s_to_b_size), "ERR") == 0)
			{
				char *error = (char*) malloc(sizeof(char) * (36 + strsize));
				strcpy(error, "address is invalid, passed address: ");
				strcat(error, strx);
				return error;
			}
			else
			{
				int temp_size = strlen(processed);
				strncat(processed, s_to_b, s_to_b_size);
				processed[temp_size + s_to_b_size] = 0;
				free(s_to_b);
			}
			protx=NULL;//Since right now it doesn't need that assignment anymore.
			firstorsecond=1;//Since the next word will be an protocol
		}
		wp=strtok_r(NULL,"/",&end);
	}
	protx=NULL;
	unload_protocols(head);

	*finalbytes = Hex_To_Var(processed, realbbsize);
	return NULL;
}
