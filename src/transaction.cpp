#include "transaction.h"
#include "lock.h"
#include <vector>
#include <algorithm>
using namespace std;

Transaction::Transaction()
{
	this->status = NEW;
}

Status Transaction::AbortTransaction()
{
	this->status = ABORTED;
	ReleaseAllLocks();
	cout << "Transaction " << this->tid << " Aborted" << endl;
	return OK;
}

void Transaction::InsertLock(int oid, bool shared)
{   
	pair<int,bool> lock(oid,shared);
	LockList.push_back(lock);
}

/** a helper function for ReleaseAllLocks()
 ** use to compare two objects according to their IDs 
 ** if two objects have the same IDs, then exclusive locks will proceed shared locks
 **/
bool CompareLockId(pair<int,bool> a, pair<int,bool> b){
	if (a.first != b.first) return a.first<b.first;
	else return a.second <= b.second;
}

void Transaction::ReleaseAllLocks()
{
	int oid;
	bool shared;
	sort(LockList.begin(),LockList.end(),CompareLockId);
	while( !LockList.empty()){
		oid=LockList.back().first;
		shared=LockList.back().second;
		LockList.pop_back();
		while (!LockList.empty() && LockList.back().first==oid){ // this while loop ensures that we only release the same lock once
			oid=LockList.back().first;
			shared=LockList.back().second;
			LockList.pop_back();
		}
		if (shared){
			LockManager::ReleaseSharedLock(this->tid,oid);
		}
		else {
			LockManager::ReleaseExclusiveLock(this->tid,oid);
		}
		
	}
	
}

Status Transaction::Read(KeyType key, DataType &value) 
{
	if (LockManager::AcquireSharedLock(this ->tid, key )==false){
		this -> AbortTransaction();
		return FAIL;
	}
	InsertLock(key, true);
	return TSHI->GetValue(key, value);
}

Status Transaction::AddWritePair(KeyType key, DataType value, OpType op)
{
	try {
		KVP kvp;
		kvp.key=key;
		kvp.value=value;
		kvp.op=op;
		writeList.push_back(kvp);
	}
	catch (...) {return FAIL;}
	return OK;
}

Status Transaction::GroupWrite() 
{
	while(! writeList.empty()){
		KVP kvp=writeList.back();
		if (LockManager::AcquireExclusiveLock(this->tid, kvp.key)==false){
			this -> AbortTransaction();
			return FAIL;
		}
		this ->InsertLock(kvp.key, false);
		Status opStatus=FAIL;
		switch(kvp.op)
		{
			case INSERT:
				opStatus= TSHI ->InsertKeyValue(kvp.key,kvp.value); 
				break;
			case DELETE:
				opStatus= TSHI ->DeleteKey(kvp.key);
				break;
			case UPDATE:
				opStatus= TSHI ->UpdateValue(kvp.key,kvp.value);
				break;
			default:
				return FAIL;
		}
		if (opStatus==FAIL) return FAIL;
		writeList.pop_back();
	}
	this->status=GROUPWRITE;
	return OK;
}

Status Transaction::StartTranscation()
{
	if (this->status != NEW)
	{
		return FAIL;
	}

	this->status = RUNNING;

	cout << "Transaction " << this->tid << " Started" << endl;
	return OK;
}

Status Transaction::EndTransaction()
{
	if (this->status != ABORTED) {
		ReleaseAllLocks();
	}
	return OK;
}

Status Transaction::CommitTransaction()
{
	if ((this->status != RUNNING) && (this->status != GROUPWRITE))
	{
		return FAIL;
	}

	cout << "Transaction " << this->tid << " Committed" << endl;
	this->status = COMMITTED;
	return OK;
}
