#pragma once
#ifndef __VERSION_H
#define __VERSION_H

#define DXGLMAJOR 1
#define DXGLMINOR 1
#define DXGLPOINT 1
#define DXGLBUILD 1
//#define DXGLBETA

#define DXGLVERNUMBER DXGLMAJOR,DXGLMINOR,DXGLPOINT,DXGLBUILD
#define DXGLVERQWORD (((unsigned __int64)DXGLMAJOR<<48)+((unsigned __int64)DXGLMINOR<<32)+((unsigned __int64)DXGLPOINT<<16)+(unsigned __int64)DXGLBUILD)
#define DXGLVERSTRING ""

#define COPYYEAR 2018
#define COPYYEARSTRING "2018"

#define SHADER2DVERSION 1
#define SHADER3DVERSION 1

#endif //__VERSION_H
