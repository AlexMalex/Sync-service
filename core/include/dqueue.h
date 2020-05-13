#if !defined(DQUEUE_H)
#define DQUEUE_H

#include "sync.h"
#include "tstring.h"
#include "protobj.h"

class TBlock {
public:
	TBlock(PObject, bool=true);
	~TBlock();

	TBlock* next;
	TBlock* prev;

	PObject obj;

	bool free_object;
};

typedef TBlock* PBlock;
typedef TBlock& RBlock;

class DQueue: public TString, public TProtectedObject {
private:
	int count;
	bool syncronize;
public:
	PBlock head;
	PBlock tail;

	DQueue(char*, bool=false);
	virtual ~DQueue();

	bool addElement(PObject, bool=true);
	bool addElement_tail(PObject, bool=true);

	PObject removeElement(PBlock);

	bool isEmpty();

	void clear();

	int size();
};

inline bool DQueue::isEmpty()
{
	return (count==0);
}

inline int DQueue::size()
{
	return count;
}
typedef DQueue* PDQueue;
typedef DQueue& RDQueue;

#endif
