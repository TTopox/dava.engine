/*
	DAVA SDK
	Fast Names
	Author: Sergey Zdanevich
*/

#ifndef __DAVAENGINE_FAST_NAME__
#define __DAVAENGINE_FAST_NAME__

#include "HashMap.h"
#include "Debug/DVAssert.h"
#include "Base/StaticSingleton.h"

namespace DAVA
{

struct FastNameDB : public StaticSingleton<FastNameDB>
{
	FastNameDB()
		// namesHash init. size will be 4096 and default values for int will be -1
		: namesHash(HashMap<const char *, int>(4096, -1))
	{};

	Vector<const char *> namesTable;
	Vector<int> namesRefCounts;
	Vector<int> namesEmptyIndexes;
	HashMap<const char *, int> namesHash;
};

class FastName
{
private:

public:
	FastName();
	FastName(const char *name);
	FastName(const FastName &_name);
	~FastName();

	const char* operator*() const;
	FastName& operator=(const FastName &_name);
	bool operator==(const FastName &_name) const;
	bool operator!=(const FastName &_name) const;
	int Index() const;

private:
	int index;

};

};

#endif // __DAVAENGINE_FAST_NAME__
