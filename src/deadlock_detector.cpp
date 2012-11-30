#include "deadlock_detector.h"
#include <vector>
#include <map>
#include <stack>
using namespace std;

#define maxT 50 //maxNumOfCurrentTransaction

bool abortT[maxT];

//POSSIBLE DATA STRUCTURE FOR WAIT-FOR GRAPH
bool waitFor[maxT][maxT];

vector<vector<Node *>*> *cycles; // a list whose elements are lists of strongly connected components in the graph
map<int,Node*> *nodeTable; // a map of transaction ids to transaction nodes
stack<Node *> *nodeStack; // used for Tarjan's algorithm

int index = 0; // used for Tarjan's algorithm to group strongly connected components

void DeadlockDetector::BuildWaitForGraph()
{
	vector<int> *haveLock = new vector<int>(); // a list of all transactions that have a lock on an object
	for each(KeyValuePair<int, ReadWriteFIFOLock^> kvp in LockManager::lockTable)
	{
		int oid = kvp.Key;
		ReadWriteFIFOLock^ lock = kvp.Value;

		// Make sure Monitor::Exit(lock->_m) is called for every Monitor::Enter
		// (so watch out if you use "continue" or "break")
		Monitor::Enter(lock->_m);
		
		//TRAVERSAL lockingList
		for each (int pid in lock->lockingList)
		{
			if (nodeTable->count(pid) == 0) {  // new transaction node encountered
				Node *v = new Node(pid,-1,-1,1,false); // create node
				nodeTable->insert(pair<int,Node *>(pid,v)); // add to table
			} else { // existing transation, update lock count
				map<int,Node*>::iterator it;
				it = nodeTable->find(pid);
				Node *v = it->second;
				v->numLocks = v->numLocks + 1;
			}
			haveLock->push_back(pid); // add it to the set of transactions that "have lock"
		}

		//TRAVERSAL waitQ
		for each (Request^ req in lock->waitQ)
		{
			int pid = req->pid;
			if (nodeTable->count(pid) == 0) {  // new transaction node encountered
				Node *v = new Node(pid,-1,-1,0,false);
				nodeTable->insert(pair<int,Node *>(pid,v)); // add to table
			}
			
			// build waitFor
			for (int i=0;i<haveLock->size();i++) {
				int tid = haveLock->at(i);
				if (pid != tid) {
					waitFor[pid][tid] = 1; // pid is waiting for tid
				}
			}
		}

		haveLock->clear();

		Monitor::Exit(lock->_m);


	}

	delete haveLock;
}

void DeadlockDetector::AnalyzeWaitForGraph()
{
	// Awesome algorithm starts here
	DetectCycle();
	// Determine abort
	int numLocks = 0; Node *abortTarget; bool found = false;
	for (int i=0;i<cycles->size();i++) {
		vector<Node *> *list = cycles->at(i);
		if (list->size()>1) { // if there is a cycle
			for (int j=0;j<list->size();j++) {
				Node *v = list->at(j);
				if (v->numLocks > numLocks) { // the find transaction that holds the most locks
					numLocks = v->numLocks;
					abortTarget = v;
					found = true;
				}
			}
		}
	}
	if (found) abortT[abortTarget->tid] = true;

	// Reset graph and such
	for (int i=0;i<maxT;i++){
		for (int j=0;j<maxT;j++){
			waitFor[i][j] = 0;
		}
	}
	index = 0;
	for (int i=0;i<cycles->size();i++) {
		delete cycles->at(i);
	}
	cycles->clear();
	while (!nodeStack->empty()) {
		nodeStack->pop();
	}
	nodeTable->clear();
}

void DeadlockDetector::DetectCycle() {
	// Tarjan's algorithm
	map<int, Node *>::iterator  it;
	
	for(it = nodeTable->begin(); it != nodeTable->end(); it++) {
		Node *v = it->second;
		if (v->index < 0){
			StrongConnect(v);
        }
    }
}

void DeadlockDetector::StrongConnect(Node *v){
	// Tarjan's algorithm
	v->index = index;
    v->lowLink = index;
    index++;
    nodeStack->push(v);
	v->inStack = true;

    for (int i=0;i<maxT;i++) {
		if (waitFor[v->tid][i] == 1){ // v is waiting for transaction i
			map<int,Node*>::iterator it;
			it = nodeTable->find(i);
			Node *w = it->second; // find w
			if (w->index < 0){
				StrongConnect(w);
				v->lowLink = min(v->lowLink, w->lowLink);
			} else if (w->inStack){
				v->lowLink = min(v->lowLink, w->index);
			}
		}
	}

	if (v->lowLink == v->index){ // v is the root of a cycle
		vector<Node *> *list = new vector<Node *>();
		Node *w;
		do {
			w = nodeStack->top();
			nodeStack->pop();
			list->push_back(w);
		} while (v != w);
	
		cycles->push_back(list);
	}
}

void DeadlockDetector::AbortTransactions()
{
	//DO NOT CHANGE ANY CODE HERE
	bool deadlock = false;

	for (int i = 0; i < maxT; ++i)
		if (abortT[i])
		{
			deadlock = true;
			break;
		}

	if (!deadlock) {
		Console::WriteLine("no deadlock found");
	}
	else {
		for (int i = 0; i < maxT; ++i)
			if (abortT[i])
			{
				Console::WriteLine("DD: Abort Transaction: " + i);
			}

		for each(KeyValuePair<int, ReadWriteFIFOLock^> kvp in LockManager::lockTable)
		{
			int oid = kvp.Key;
			ReadWriteFIFOLock^ lock = kvp.Value;

			Monitor::Enter(lock->_m);

			bool pall = false;

			for each(Request^ req in lock->waitQ)
				if (abortT[req->pid])
				{
					lock->abortList->Add(req->pid);
					lock->wakeUpList->Add(req->pid);
					Console::WriteLine("DD: Transaction " + req->pid + " cancel object " + oid);
					pall = true;
				}

			if (pall)
			{
				Monitor::PulseAll(lock->_m);
			}

			Monitor::Exit(lock->_m);
		}
	}
}

void DeadlockDetector::run()
{
	while (true)
	{
		Thread::Sleep(timeInterval);

		memset(waitFor, 0 ,sizeof(waitFor));
		//INITIALIZE ANY OTHER DATA STRUCTURES YOU DECLARE.

		cycles = new vector<vector<Node *>*>();
		nodeTable = new map<int,Node *>();
		nodeStack = new stack<Node *>();

		memset(abortT, 0 ,sizeof(abortT));

		Monitor::Enter(LockManager::lockTable);

		BuildWaitForGraph();

		AnalyzeWaitForGraph();

		AbortTransactions();

		delete nodeTable;
		delete cycles;
		delete nodeStack;

		Monitor::Exit(LockManager::lockTable);
	}
}
