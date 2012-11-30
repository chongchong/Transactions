#ifndef _DEADLOCK_H
#define _DEADLOCK_H

#include "lock.h"

// A Node represents a transaction
// It's used to analyze the wait-for graph
struct Node {
	int tid; // transaction id
	int numLocks; // number of locks this transaction holds
	int index;  // used for Tarjan's Algorithm
	int lowLink; // used for Tarjan's Algorithm
	bool inStack; // used for Tarjan's Algorithm
	
	Node(int t, int i, int l, int n, bool s){
		tid = t; numLocks = n; index = i; lowLink = l; inStack = s;
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
	//--------------------------------------------------------------------
	// DeadlockDetector::DetectCycle
	//
	// Input    : None.
	// Purpose  : Part of Tarjan's Algorithm the detects cycles.
	// Return   : None.
	//--------------------------------------------------------------------
	void DetectCycle();

	//--------------------------------------------------------------------
	// DeadlockDetector::StrongConnect
	//
	// Input    : A Node struct of transaction
	// Purpose  : Determine which strong connected partition it belongs to
	// Return   : None.
	//--------------------------------------------------------------------
	void StrongConnect(Node *v);
};

#endif //_DEADLOCK_H