/*variable byte encoding and decoding*/

#include "vbyte.h"
#include <stdbool.h>

int vbyte_len(unsigned int n) {
    unsigned int ret = 1;
    while (n >= 128) {
        n >>= 7;
        ret++;
    }
    return ret;
}


/* compress n into src,
 * if violate the bound,return 0,
 * else return compressed size 
 */

int vbyte_compress(char *src,unsigned int n)   
{
	int len = 0;

	while(n >= 128)
	{
		*(src+len) = (unsigned char)((n & 0xff)|0x80);   // The 8th bit 
		n >>= 7;
		len++;
	}

	*(src+len) = (unsigned char)n;
	
	len++;
	
	*(src+len) = 0;

	return len;
}


int vbyte_decompress(unsigned char *src,unsigned int *n)
{
	int len = 0;

	*n = 0;

	while( *(src+len) & 0x80 )
	{
		*n += ( *(src+len) & 0x7f ) << (7*len);
		len++;
	}


	*n += (*(src+len)&0x7f)<<(7*len);
	len++;
	return len;
}





