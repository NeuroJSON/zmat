/***************************************************************************//**
**  \mainpage ZMAT: a data compression function for MATLAB/octave
**
**  \author Qianqian Fang <q.fang at neu.edu>
**  \copyright Qianqian Fang, 2019
**
**  Functions: base64_encode()/base64_decode()
**  \copyright 2005-2011, Jouni Malinen <j@w1.fi>
**
**  \section slicense License
**          GPL v3, see LICENSE.txt for details
*******************************************************************************/


/***************************************************************************//**
\file    zmat.cpp

@brief   mex function for ZMAT
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <exception>
#include <ctype.h>

#include "mex.h"
#include "zlib.h"

#if (! defined MX_API_VER) || (MX_API_VER < 0x07300000)
      typedef int dimtype;                              /**<  MATLAB before 2017 uses int as the dimension array */
#else
      typedef size_t dimtype;                           /**<  MATLAB after 2017 uses size_t as the dimension array */
#endif


void zmat_usage();
int  zmat_keylookup(char *origkey, const char *table[]);
unsigned char * base64_encode(const unsigned char *src, size_t len,
			      size_t *out_len);
unsigned char * base64_decode(const unsigned char *src, size_t len,
			      size_t *out_len);


enum TZipMethod {zmZlib, zmGzip, zmBase64};
const char  *metadata[]={"type","size","status"};

/** @brief Mex function for the zmat - an interface to compress/decompress binary data
 *  This is the master function to interface for zipping and unzipping a char/int8 buffer
 */

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]){
  TZipMethod zipid=zmZlib;
  int iscompress=1;
  const char *zipmethods[]={"zlib","gzip","base64",""};

  /**
   * If no input is given for this function, it prints help information and return.
   */
  if (nrhs==0){
     zmat_usage();
     return;
  }

  if(nrhs>=2){
      double *val=mxGetPr(prhs[1]);
      iscompress=val[0];
  }
  if(nrhs>=3){
      int len=mxGetNumberOfElements(prhs[2]);
      if(!mxIsChar(prhs[2]) || len==0)
             mexErrMsgTxt("the 'method' field must be a non-empty string");
      if((zipid=(TZipMethod)zmat_keylookup((char *)mxArrayToString(prhs[2]), zipmethods))<0)
             mexErrMsgTxt("the specified compression method is not supported");
  }

  try{
	  if(mxIsChar(prhs[0]) || mxIsUint8(prhs[0]) || mxIsInt8(prhs[0])){
	       z_stream zs;
	       int ret;
	       dimtype inputsize=mxGetNumberOfElements(prhs[0]);
	       dimtype buflen[2]={0};
	       unsigned char *temp=NULL;
	       size_t outputsize=0;
	       char * inputstr=(mxIsChar(prhs[0])? mxArrayToString(prhs[0]) : (char *)mxGetData(prhs[0]));

    	       zs.zalloc = Z_NULL;
    	       zs.zfree = Z_NULL;
    	       zs.opaque = Z_NULL;

	       if(inputsize==0)
		    mexErrMsgTxt("input can not be empty");

	       if(iscompress){
                    if(zipid==zmBase64){

		        temp=base64_encode((const unsigned char*)inputstr, inputsize, &outputsize);
	            }else{
			if(zipid==zmZlib){
		            if(deflateInit(&zs, Z_DEFAULT_COMPRESSION) != Z_OK)
		        	mexErrMsgTxt("failed to initialize zlib");
			}else{
		            if(deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15|16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK)
		        	mexErrMsgTxt("failed to initialize zlib");
			}
			buflen[0] =deflateBound(&zs,inputsize);
			temp=(unsigned char *)malloc(buflen[0]);
			zs.avail_in = inputsize; // size of input, string + terminator
			zs.next_in = (Bytef *)inputstr; // input char array
			zs.avail_out = buflen[0]; // size of output

			zs.next_out =  (Bytef *)(temp); //(Bytef *)(); // output char array

			ret=deflate(&zs, Z_FINISH);
			outputsize=zs.total_out;
			if(ret!=Z_STREAM_END && ret!=Z_OK)
		            mexErrMsgTxt("zlib error, see info.status for error flag");
			deflateEnd(&zs);
		    }
	       }else{
                    if(zipid==zmBase64){
		        temp=base64_decode((const unsigned char*)inputstr, inputsize, &outputsize);
	            }else{
		        int count=1;
	        	if(zipid==zmZlib){
		            if(inflateInit(&zs) != Z_OK)
		               mexErrMsgTxt("failed to initialize zlib");
			}else{
		            if(inflateInit2(&zs, 15|32) != Z_OK)
		               mexErrMsgTxt("failed to initialize zlib");
			}
			buflen[0] =inputsize*20;
			temp=(unsigned char *)malloc(buflen[0]);

			zs.avail_in = inputsize; // size of input, string + terminator
			zs.next_in =(Bytef *)(mxGetData(prhs[0])); // input char array
			zs.avail_out = buflen[0]; // size of output

			zs.next_out =  (Bytef *)(temp); //(Bytef *)(); // output char array

                	while((ret=inflate(&zs, Z_SYNC_FLUSH))!=Z_STREAM_END && count<=10){
			    temp=(unsigned char *)realloc(temp, (buflen[0]<<count));
			    zs.next_out =  (Bytef *)(temp+(buflen[0]<<(count-1))); //(Bytef *)(); // output char array
			    zs.avail_out = (buflen[0]<<(count-1)); // size of output
			    count++;
			}
			outputsize=zs.total_out;

			if(ret!=Z_STREAM_END && ret!=Z_OK)
		            mexErrMsgTxt("zlib error, see info.status for error flag");
			inflateEnd(&zs);
		    }
	       }
	       if(temp){
	            buflen[0]=1;
		    buflen[1]=outputsize;
		    plhs[0] = mxCreateNumericArray(2,buflen,mxUINT8_CLASS,mxREAL);
		    memcpy((unsigned char*)mxGetPr(plhs[0]),temp,buflen[1]);
		    free(temp);
	       }
	       if(nlhs>1){
	            dimtype inputdim[2]={1,0};
	            plhs[1]=mxCreateStructMatrix(1,1,3,metadata);
		    mxArray *val = mxCreateString(mxGetClassName(prhs[0]));
                    mxSetFieldByNumber(plhs[1],0,0, val);

		    inputdim[1]=mxGetNumberOfDimensions(prhs[0]);
		    val = mxCreateNumericArray(2, inputdim, mxUINT32_CLASS, mxREAL);
		    memcpy(mxGetPr(val),mxGetDimensions(prhs[0]),inputdim[1]*sizeof(dimtype));
                    mxSetFieldByNumber(plhs[1],0,1, val);

                    val = mxCreateDoubleMatrix(1,1,mxREAL);
                    *mxGetPr(val) = ret;
                    mxSetFieldByNumber(plhs[1],0,2, val);
	       }
	  }else{
	      mexErrMsgTxt("the input must be in char or int8/uint8 format");
	  }
  }catch(const char *err){
      mexPrintf("Error: %s\n",err);
  }catch(const std::exception &err){
      mexPrintf("C++ Error: %s\n",err.what());
  }catch(...){
      mexPrintf("Unknown Exception");
  }
  return;
}


/**
 * @brief Print a brief help information if nothing is provided
 */

void zmat_usage(){
     printf("Usage:\n    [output,info]=zmat(input,iscompress,method);\n\nPlease run 'help zmat' for more details.\n");
}

/**
 * @brief Look up a string in a string list and return the index
 *
 * @param[in] origkey: string to be looked up
 * @param[out] table: the dictionary where the string is searched
 * @return if found, return the index of the string in the dictionary, otherwise -1.
 */

int zmat_keylookup(char *origkey, const char *table[]){
    int i=0;
    char *key=(char *)malloc(strlen(origkey)+1);
    memcpy(key,origkey,strlen(origkey)+1);
    while(key[i]){
        key[i]=tolower(key[i]);
	i++;
    }
    i=0;
    while(table[i] && table[i][0]!='\0'){
	if(strcmp(key,table[i])==0){
	        free(key);
		return i;
	}
	i++;
    }
    free(key);
    return -1;
}


/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

static const unsigned char base64_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable, or %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned buffer is
 * nul terminated to make it easier to use as a C string. The nul terminator is
 * not included in out_len.
 */
unsigned char * base64_encode(const unsigned char *src, size_t len,
			      size_t *out_len)
{
	unsigned char *out, *pos;
	const unsigned char *end, *in;
	size_t olen;
	int line_len;

	olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
	olen += olen / 72; /* line feeds */
	olen++; /* nul termination */
	if (olen < len)
		return NULL; /* integer overflow */
	out = (unsigned char *)malloc(olen);
	if (out == NULL)
		return NULL;

	end = src + len;
	in = src;
	pos = out;
	line_len = 0;
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
		line_len += 4;
		if (line_len >= 72) {
			*pos++ = '\n';
			line_len = 0;
		}
	}

	if (end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else {
			*pos++ = base64_table[((in[0] & 0x03) << 4) |
					      (in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
		line_len += 4;
	}

	if (line_len)
		*pos++ = '\n';

	*pos = '\0';
	if (out_len)
		*out_len = pos - out;
	return out;
}


/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
unsigned char * base64_decode(const unsigned char *src, size_t len,
			      size_t *out_len)
{
	unsigned char dtable[256], *out, *pos, block[4], tmp;
	size_t i, count, olen;
	int pad = 0;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[base64_table[i]] = (unsigned char) i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[src[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return NULL;

	olen = count / 4 * 3;
	pos = out = (unsigned char *)malloc(olen);
	if (out == NULL)
		return NULL;

	count = 0;
	for (i = 0; i < len; i++) {
		tmp = dtable[src[i]];
		if (tmp == 0x80)
			continue;

		if (src[i] == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					/* Invalid padding */
					free(out);
					return NULL;
				}
				break;
			}
		}
	}

	*out_len = pos - out;
	return out;
}
