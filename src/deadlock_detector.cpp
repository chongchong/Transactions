#include "deadlock_detector.h"
#include <vector>
#include <map>
#include <stack>
using namespace std;

#define maxT 50 //maxNumOfCurrentTransaction

bool abortT[maxT];

//POSSIBLE DATA STRUCTURE FOR WAIT-FOR GRAPH
bool waitFor[maxT][maxT];

vector<vector<Node *>> *cycles;
map<int,Node*> *nodeTable;
stack<Node *> *nodeStack;

void DeadlockDetector::BuildWaitForGraph()
{
	vector<int> *haveLock = new vector<int>(); // all transactions that have lock on this object
	for each(KeyValuePair<int, ReadWriteFIFOLock^> kvp in LockManager::lockTable)
	{
		int oid = kvp.Key;
		ReadWriteFIFOLock^ lock = kvp.Value;

		// Make sure Monitor::Exit(lock->_m) is called for every Monitor::Enter
		// (so watch out if you use "continue" or "break")
		Monitor::Enter(lock->_m);

		//BEGIN OF TODO
		
		//TRAVERSAL lockingList
		for each (int pid in lock->lockingList)
		{
			if (nodeTable->count(pid) == 0) {  // new graph node 
				Node *v = new Node(oid,pid,-1,-1,1);
				nodeTable->insert(pair<int,Node *>(pid,v)); // add to table
				haveLock->push_back(pid); // add to set of transactions that have this lock
				//cout << "table count is " << nodeTable->size() << "\n" << endl;
			} else { // existing transation, update lock count
				map<int,Node*>::iterator it;
				it = nodeTable->find(pid);
				Node *v = it->second;
				v->numLocks = v->numLocks + 1;
			}
		}

		//TRAVERSAL waitQ
		for each (Request^ req in lock->waitQ)
		{
			int pid = req->pid;
			if (nodeTable->count(pid) == 0) {  // new graph node
				Node *v = new Node(oid,pid,-1,-1,0);
				nodeTable->insert(pair<int,Node *>(pid,v)); // add to table
			}
			// build waitFor
			for (int i=0;i<haveLock->size();i++) {
				int tid = haveLock->at(i);
				if (pid != tid) {
					waitFor[pid][tid] = 1;
				}
			}
		}

		/*
		if (lock->waitQ->Count > 0)
		{
			//FIRST ELEMENT OF waitQ
			Request^ req = lock->waitQ->First->Value;
							
			if (lock->waitQ->Count > 1)
			{
				//SECOND ELEMENT OF waitQ
				Request^ r = lock->waitQ->First->Next->Value;
			}
		}*/

		haveLock->clear();

		//END OF TODO

		Monitor::Exit(lock->_m);


	}


	/*
			for (int i=0;i<maxT;i++){

				for (int j=0;j<maxT;j++){
					if (waitFor[i][j]==1){
					cout << i << " is waiting for " << j << "\n" << endl;
					}
				}
			}
		*/

	delete haveLock;
}

void DeadlockDetector::AnalyzeWaitForGraph()
{
		
	// Awesome algorithm starts here


	
}


void DeadlockDetector::DetectCycle() {
	int index = 0;
	//map<int,Node *>::iterator it = nodeTable->find(pid);
		//		Node *v = it->second;
	map<int, Node *>::iterator  it;
	
	//for(it = nodeTable->begin(); it != nodeTable->end(); it++) {
		//Node *v = it->second;
		/*
		if (v->index < 0){
			StrongConnect(v);
        }
    }*/
}

//template <typename T>
void DeadlockDetector::StrongConnect(Node *v){
	
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
		Console::WriteLine("DEADLOCKKKKKKKKKKKKKKKKK");
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

		//BEGIN OF TODO
		memset(waitFor, 0 ,sizeof(waitFor));
		//INITIALIZE ANY OTHER DATA STRUCTURES YOU DECLARE.

		cycles = new vector<vector<Node *>>;
		nodeTable = new map<int,Node *>();
		nodeStack = new stack<Node *>();

		//END OF TODO

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
