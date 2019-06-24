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
#include <assert.h>

#include "mex.h"
#include "zmatlib.h"
#include "zlib.h"

void zmat_usage();

const char  *metadata[]={"type","size","status"};

extern char *zmat_err[];

/** @brief Mex function for the zmat - an interface to compress/decompress binary data
 *  This is the master function to interface for zipping and unzipping a char/int8 buffer
 */

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]){
  TZipMethod zipid=zmZlib;
  int iscompress=1;
#ifndef NO_LZMA
  const char *zipmethods[]={"zlib","gzip","base64","lzip","lzma",""};
#else
  const char *zipmethods[]={"zlib","gzip","base64",""};
#endif

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
	       int ret;
	       mwSize inputsize=mxGetNumberOfElements(prhs[0]);
	       mwSize buflen[2]={0};
	       unsigned char *outputbuf=NULL;
	       size_t outputsize=0;
	       unsigned char * inputstr=(mxIsChar(prhs[0])? (unsigned char *)mxArrayToString(prhs[0]) : (unsigned char *)mxGetData(prhs[0]));

    	       int errcode=zmat_run(inputsize, inputstr, &outputsize, &outputbuf, zipid, &ret, iscompress);
	       if(errcode<0)
	           mexErrMsgTxt(zmat_err[-errcode]);

	       if(outputbuf){
	            buflen[0]=1;
		    buflen[1]=outputsize;
		    plhs[0] = mxCreateNumericArray(2,buflen,mxUINT8_CLASS,mxREAL);
		    memcpy((unsigned char*)mxGetPr(plhs[0]),outputbuf,buflen[1]);
		    free(outputbuf);
	       }
	       if(nlhs>1){
	            mwSize inputdim[2]={1,0}, *dims=(mwSize *)mxGetDimensions(prhs[0]);
		    unsigned int *inputsize=NULL;
	            plhs[1]=mxCreateStructMatrix(1,1,3,metadata);
		    mxArray *val = mxCreateString(mxGetClassName(prhs[0]));
                    mxSetFieldByNumber(plhs[1],0,0, val);

		    inputdim[1]=mxGetNumberOfDimensions(prhs[0]);
		    inputsize=(unsigned int *)malloc(inputdim[1]*sizeof(unsigned int));
		    val = mxCreateNumericArray(2, inputdim, mxUINT32_CLASS, mxREAL);
		    for(int i=0;i<inputdim[1];i++)
		        inputsize[i]=dims[i];
		    memcpy(mxGetPr(val),inputsize,inputdim[1]*sizeof(unsigned int));
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
