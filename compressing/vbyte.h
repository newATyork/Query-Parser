/* vbyte.h declares functions to read and write variable-byte encoded
 * integers to files
 */

#ifndef VBYTE_H
#define VBYTE_H

#ifdef __cplusplus
extern "C" {
#endif

	int vbyte_len(unsigned int n);
	int vbyte_compress(char *src, unsigned int n);
	int vbyte_decompress(unsigned char *src,unsigned int *n);

#ifdef __cplusplus
}
#endif

#endif

