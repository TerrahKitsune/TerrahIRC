#pragma once
#include <windows.h>

template<class T> class TerrahStack
{
	
public:
	TerrahStack();
	~TerrahStack();

	bool Push(T data);
	T Pop(int nNth=0);
	T QueuePop();
	T Peek(int nNth=0);
	size_t Size();

protected:

	CRITICAL_SECTION CriticalSection;

	template<class T> class TSNode{

	public:
		TSNode(){ memset(this, 0, sizeof(TSNode)); };
		~TSNode(){};

		T Data;
		TSNode * Next;
	};

	TSNode<T> * root;
};

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
	this->root = node;
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

		*prev = node->Next;

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

template<class T> T TerrahStack<T>::QueuePop(){

	EnterCriticalSection(&this->CriticalSection);

	TSNode<T> * node = root;
	TSNode<T> ** prev = &root;

	while (node->Next){
		prev = &node->Next;
		node = node->Next;
	}

	if (node){

		*prev = NULL;

		T ret = node->Data;

		delete node;

		LeaveCriticalSection(&this->CriticalSection);
		return ret;
	}

	LeaveCriticalSection(&this->CriticalSection);
	return 0;
}