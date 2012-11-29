#ifndef _DEADLOCK_H
#define _DEADLOCK_H

#include "lock.h"

struct Node {
	int oid; // object id
	int tid; // transaction id
	LockType type; // type of lock this trans is holding
	//vector<Node*> *waitFor; // a vector of transactions this is waiting for
	int numLocks; // number of locks this transaction holds
	int index;  // used for Tarjan's Algorithm
	int lowLink; // used for Tarjan's Algorithm
	
	Node(int o, int t, int i, int l, int n){
		oid = o; tid = t; index = i; lowLink = l; numLocks = n;
	}
};

public ref class DeadlockDetector
{
public:
	// the interval between consecutive runs of deadlock detection.
	int timeInterval;

	DeadlockDetector()
	{
		this->timeInterval = 1000;
	}

	DeadlockDetector(int time)
	{
		this->timeInterval = time;
	}

	void run();

	//--------------------------------------------------------------------
	// DeadlockDetector::BuildWaitForGraph
	//
	// Input    : None.
	// Purpose  : Build a wait for graph based on information in lockTable.
	// Return   : None.
	//--------------------------------------------------------------------
	void BuildWaitForGraph();
	
	//--------------------------------------------------------------------
	// DeadlockDetector::AnalyzeWaitForGraph
	//
	// Input    : None.
	// Purpose  : Analyze the wait for graph. Detect deadlock.
	//			  Decide on victims in case of deadlock
	// Return   : None.
	//--------------------------------------------------------------------
	void AnalyzeWaitForGraph();

	//--------------------------------------------------------------------
	// DeadlockDetector::AbortTransactions
	//
	// Input    : None.
	// Purpose  : Abort the victim transactions as decided by analysis.
	// Return   : None.
	//--------------------------------------------------------------------
	void AbortTransactions();

private:
	void DetectCycle();
	void StrongConnect(Node *v);
};

#endif //_DEADLOCK_H