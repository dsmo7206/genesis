#pragma once

#include <deque>
#include <map>

class ComputeClient
{
	public:
	
	// Returns the number of items run
	virtual unsigned runAllComputeItems() = 0;

	// Returns the number of items run
	virtual unsigned runSomeComputeItems(int count, bool& allRun) = 0;
};

struct CountAndTime
{
	unsigned m_count;
	double m_time;

	CountAndTime(unsigned count, double time) : 
		m_count(count), m_time(time) 
	{}
};

class ComputeQueue
{
	std::deque<ComputeClient*> m_clients;
	std::map<ComputeClient*, CountAndTime> m_timings;

	ComputeQueue();

	void addTimings(ComputeClient* client, unsigned itemsRun, double time);
	double getAverageTime(ComputeClient* client);
	void _runSome(ComputeClient* client, int count);

	public:

	inline static ComputeQueue& get()
	{
		static ComputeQueue queue;
		return queue;
	}

	inline void addClient(ComputeClient* client)
	{
		m_clients.push_back(client);
	}

	void runAll();
	void runSome(int count);
	void runUntil(double endTime);
};
