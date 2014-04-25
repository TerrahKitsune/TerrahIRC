#pragma once
#include <windows.h>

template<class T> class TerrahStack
{

public:
	TerrahStack();
	~TerrahStack();

	bool Push(T data);
	T Pop(int nNth = 0);
	T Peek(int nNth = 0);
	size_t Size();
	bool IsEmpty(){ return root == NULL; }

	T QueueGetFirst(T DefaultData);
	T QueueGetNext();
	T QueuePopCurrent();

	CRITICAL_SECTION CriticalSection;

	template<class T> class TSNode{

	public:
		TSNode(){ memset(this, 0, sizeof(TSNode)); };
		~TSNode(){};

		T Data;
		TSNode * Next;
		TSNode * Prev;
	};

	TSNode<T> * root;
	TSNode<T> * trav;
	T DefVal;
};

template<class T> T TerrahStack<T>::QueuePopCurrent(){

	EnterCriticalSection(&this->CriticalSection);

	if (trav){

		if (root == trav){
			root = trav->Next;
		}

		if (trav->Next)
			trav->Next->Prev = trav->Prev;
		if (trav->Prev)
			trav->Prev->Next = trav->Next;

		TSNode<T> * tmp = trav->Prev;

		delete trav;

		trav = tmp;

		LeaveCriticalSection(&this->CriticalSection);
		if (tmp)
			return tmp->Data;
		else
			return DefVal;
	}

	LeaveCriticalSection(&this->CriticalSection);
	return DefVal;
}

template<class T> T TerrahStack<T>::QueueGetFirst(T DefaultData){

	TSNode<T> * node = root;
	TSNode<T> * trg = root;

	DefVal = DefaultData;

	EnterCriticalSection(&this->CriticalSection);

	while (node){
		trg = node;
		node = node->Next;
	}

	if (trg){
		trav = trg;
		LeaveCriticalSection(&this->CriticalSection);
		return trav->Data;
	}

	LeaveCriticalSection(&this->CriticalSection);
	trav = NULL;

	return DefVal;
}

template<class T> T TerrahStack<T>::QueueGetNext(){

	EnterCriticalSection(&this->CriticalSection);

	if (trav){

		trav = trav->Prev;
		LeaveCriticalSection(&this->CriticalSection);
		if (trav)
			return trav->Data;
		else return DefVal;
	}

	LeaveCriticalSection(&this->CriticalSection);
	return DefVal;
}

template<class T>TerrahStack<T>::TerrahStack(){

	memset(this, 0, sizeof(TerrahStack));
	InitializeCriticalSection(&this->CriticalSection);
}

template<class T>TerrahStack<T>::~TerrahStack(){

	TSNode<T> * node = root;
	TSNode<T> * temp;

	while (node){

		temp = node;
		node = node->Next;

		delete temp;
	}

	root = NULL;

	DeleteCriticalSection(&this->CriticalSection);
}

template<class T> size_t TerrahStack<T>::Size(){

	EnterCriticalSection(&this->CriticalSection);
	TSNode<T> * node = root;
	size_t len = 0;

	while (node){

		len++;
		node = node->Next;
	}
	LeaveCriticalSection(&this->CriticalSection);
	return len;
}

template<class T> bool TerrahStack<T>::Push(T data){

	EnterCriticalSection(&this->CriticalSection);
	TSNode<T> * node = new TSNode<T>;

	if (!node)
		return false;

	node->Data = data;
	node->Next = this->root;
	node->Prev = NULL;
	this->root = node;

	if (node->Next)
		node->Next->Prev = node;

	LeaveCriticalSection(&this->CriticalSection);
	return true;
}

template<class T> T TerrahStack<T>::Pop(int nNth = 0){

	EnterCriticalSection(&this->CriticalSection);
	TSNode<T> * node = root;
	TSNode<T> ** prev = &root;

	int n = 0;
	while (node){

		if (n++ == nNth)
			break;

		prev = &node->Next;
		node = node->Next;
	}

	if (node){

		if (node == trav)
			trav = root;

		*prev = node->Next;

		if (node->Next)
			node->Next->Prev = node->Prev;


		T ret = node->Data;

		delete node;

		LeaveCriticalSection(&this->CriticalSection);
		return ret;
	}

	LeaveCriticalSection(&this->CriticalSection);
	return 0;
}


template<class T> T TerrahStack<T>::Peek(int nNth = 0){

	EnterCriticalSection(&this->CriticalSection);
	TSNode<T> * node = root;

	int n = 0;
	while (node){

		if (n++ == nNth)
			break;

		node = node->Next;
	}

	if (node){

		LeaveCriticalSection(&this->CriticalSection);
		return node->Data;
	}

	LeaveCriticalSection(&this->CriticalSection);
	return 0;
}