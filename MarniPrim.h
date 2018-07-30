#pragma once
class CMarniPrim
{
public:
	CMarniPrim();
	~CMarniPrim();
};

typedef struct tagOTag
{
	u32 d0,
		d1,
		d2;
} OTag;

class CMarniPAlloc
{
public:
	CMarniPAlloc();
	~CMarniPAlloc();

	DWORD field0,
		field4,
		field8,
		count;
	OTag *pData;

	void Init(int size);
	int  InitOTag();
};

