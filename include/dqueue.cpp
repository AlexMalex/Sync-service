#include "windows.h"
#include "dqueue.h"

TBlock::TBlock(PObject o, bool free):obj(o), next(NULL), prev(NULL), free_object(free)
{
}

TBlock::~TBlock()
{
	if (free_object) {
		if (obj!=NULL) delete obj;

		obj=NULL;
	};
}

DQueue::DQueue(char* n, bool sync):TString(n), TProtectedObject(), count(0), head(NULL), tail(NULL), syncronize(sync)
{
}

DQueue::~DQueue()
{
	clear();
}

bool DQueue::addElement(PObject o, bool free_object)  // from head
{
	PBlock block=new TBlock(o, free_object);

	if (block==NULL) return false;

	if (syncronize) enter();

	try {
		if (head!=NULL) {
			head->prev=block;
			block->next=head;
		};

		head=block;

		if (count==0) tail=block;

		count++;
	}
	catch (...) {
	};

	if (syncronize) leave();

	return true;
}

bool DQueue::addElement_tail(PObject o, bool free_object)  // from tail
{
	PBlock block=new TBlock(o, free_object);

	if (block==NULL) return false;

	if (syncronize) enter();

	try {
		if (tail!=NULL) {
			tail->next=block;
			block->prev=tail;
		};

		tail=block;

		if (count==0) head=block;

		count++;
	}
	catch (...) {
	};

	if (syncronize) leave();

	return true;
}

PObject DQueue::removeElement(PBlock block)
{
	PObject o, ret_value;
	
	if (block==NULL) return NULL;

	if (isEmpty()) return NULL;

	if (syncronize) enter();

	try {
		o=block->obj;

		if (block->free_object) ret_value=NULL;
		else ret_value=o;

		if (head!=block) {
			block->prev->next=block->next;
		}
		else {
			head=block->next;
			if (head!=NULL) head->prev=NULL;
		};

		if (tail!=block) {
			block->next->prev=block->prev;
		}
		else {
			tail=block->prev;
			if (tail!=NULL) tail->next=NULL;
		};

		delete block;

		count--;
	}
	catch (...) {
	};

	if (syncronize) leave();

	return ret_value;
}

void DQueue::clear()
{
	if (syncronize) enter();

	try {
		while (count!=0) {
			removeElement(head);
		};
	}
	catch (...) {
	};

	if (syncronize) leave();
}
