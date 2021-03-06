#include "stdafx.h"
#include "MarniPrim.h"

CMarniPAlloc::CMarniPAlloc()
{
	Init(4096);
}

CMarniPAlloc::~CMarniPAlloc()
{
	if (this->pData)
	{
		delete[] this->pData;
		this->pData = NULL;
	}
}

void CMarniPAlloc::Init(int size)
{
	this->pData = new OTag[size];
	this->count = size;
	this->field0 = 3;
	InitOTag();
}

int CMarniPAlloc::InitOTag()
{
	OTag *p = this->pData;
	for (DWORD i = 0; i < this->count; i++)
	{
		p->d0 = NULL;
		p++;
	}
	this->field8 = 0;
	return 1;
}