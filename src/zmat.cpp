/***************************************************************************//**
**  \mainpage ZMAT: a data compression function for MATLAB/octave
**
**  \author Qianqian Fang <q.fang at neu.edu>
**  \copyright Qianqian Fang, 2019
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

enum TZipMethod {zmZlib, zmGzip};

void zmat_usage();
int  zmat_keylookup(char *origkey, const char *table[]);

const char  *metadata[]={"ArrayType","ArraySize"};

/** @brief Mex function for the zmat - an interface to compress/decompress binary data
 *  This is the master function to interface for zipping and unzipping a char/int8 buffer
 */

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]){
  TZipMethod zipid=zmZlib;
  int iscompress=1;
  const char *zipmethods[]={"zlib","gzip"};

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
      int len=mxGetNumberOfElements(prhs[1]);
      if(!mxIsChar(prhs[1]) || len==0)
             mexErrMsgTxt("the 'method' field must be a non-empty string");
      if((zipid=(TZipMethod)zmat_keylookup((char *)mxGetChars(plhs[1]), zipmethods))<0)
             mexErrMsgTxt("the specified compression method is not supported");
  }

  try{
	  if(mxIsChar(prhs[0]) || mxIsUint8(prhs[0])){
	       z_stream zs;
	       int ret;
	       dimtype inputsize=mxGetNumberOfElements(prhs[0]);
	       dimtype buflen[2]={0};
	       unsigned char *temp=NULL;

    	       zs.zalloc = Z_NULL;
    	       zs.zfree = Z_NULL;
    	       zs.opaque = Z_NULL;

	       if(inputsize==0)
		    mexErrMsgTxt("input can not be empty");
	       if(iscompress){
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
		    zs.next_in = (Bytef *)(mxGetChars(prhs[0])); // input char array
		    zs.avail_out = buflen[0]; // size of output

		    zs.next_out =  (Bytef *)(temp); //(Bytef *)(); // output char array
    
		    ret=deflate(&zs, Z_FINISH);
		    if(ret!=Z_STREAM_END && ret!=Z_OK)
		        mexErrMsgTxt("not all input data is compressed");
		    deflateEnd(&zs);
	       }else{
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

                    ret=inflate(&zs, Z_FINISH);
		    if(ret!=Z_STREAM_END && ret!=Z_OK)
		        mexErrMsgTxt("not all input data is decompressed");
		    inflateEnd(&zs);
	       }
	       if(temp){
	            buflen[0]=1;
		    buflen[1]=zs.total_out;
		    plhs[0] = mxCreateNumericArray(2,buflen,mxUINT8_CLASS,mxREAL);
		    memcpy((unsigned char*)mxGetPr(plhs[0]),temp,buflen[1]);
		    free(temp);
	       }
	       if(nlhs>1){
	            int inputdim[2]={1,0};
	            plhs[1]=mxCreateStructMatrix(1,1,2,metadata);
		    mxArray *val = mxCreateString(mxGetClassName(prhs[0]));
                    mxSetFieldByNumber(plhs[1],0,0, val);

		    inputdim[1]=mxGetNumberOfDimensions(prhs[0]);
		    val = mxCreateNumericArray(2, inputdim, mxUINT32_CLASS, mxREAL);
		    memcpy(mxGetPr(val),mxGetDimensions(prhs[0]),inputdim[1]*sizeof(dimtype));
                    mxSetFieldByNumber(plhs[1],0,1, val);
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
