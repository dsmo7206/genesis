#include <algorithm>
#include "compute_queue.h"
#include "glstuff.h"

ComputeQueue::ComputeQueue()
{
}

void ComputeQueue::addTimings(ComputeClient* client, unsigned itemsRun, double time)
{
	auto it = m_timings.find(client);
	if (it == m_timings.end())
	{
		m_timings.insert(std::make_pair(client, CountAndTime(itemsRun, time)));
	}
	else
	{
		it->second.m_count += itemsRun;
		it->second.m_time += time;
	}
}

double ComputeQueue::getAverageTime(ComputeClient* client)
{
	const auto it = m_timings.find(client);
	return (it == m_timings.end()) ? -1.0 : (it->second.m_time / it->second.m_count);
}

void ComputeQueue::runAll()
{
	for (auto computeClientPtr : m_clients)
	{
		const double startTime = glfwGetTime();
		glFinish(); // For accurate timing
		const unsigned itemsRun = computeClientPtr->runAllComputeItems();
		glFinish(); // For accurate timing
		const double endTime = glfwGetTime();

		addTimings(computeClientPtr, itemsRun, endTime - startTime);
	}

	m_clients.clear();
}

void ComputeQueue::_runSome(ComputeClient* client, int count)
{
	bool allRun;
	const double startTime = glfwGetTime();
	glFinish(); // For accurate timing
	const unsigned itemsRun = client->runSomeComputeItems(count, allRun);
	glFinish(); // For accurate timing
	const double endTime = glfwGetTime();

	//addTimings(client, itemsRun, endTime - startTime);

	if (!allRun)
		m_clients.push_back(client);

	printf("Ran %u batches in %lf sec\n", itemsRun, endTime - startTime);
}

/* 
MISTAKE: The next 2 functions shouldn't just look at
the first client, they should keep running clients in a loop
until the time is exhausted!!!
(This will only make a difference with >1 clients)
*/

void ComputeQueue::runSome(int count)
{
	if (m_clients.empty())
		return;

	// Find the client to run
	ComputeClient* const client = m_clients.front();
	m_clients.pop_front();

	// Run and add timings
	_runSome(client, count);
}

void ComputeQueue::runUntil(double endTime)
{
	if (m_clients.empty())
		return;

	const double startTime = glfwGetTime();

	// Find the client to run
	ComputeClient* const client = m_clients.front();
	m_clients.pop_front();

	double averageTime = getAverageTime(client);
	if (averageTime < 0.0) // No data present
	{
		_runSome(client, 1); // This will add some data
		if (endTime <= startTime)
			return; // In case populating the initial average took long enough
	}

	// Run a number that should take us up to the desired time,
	// but run a minimum of one unit, even if it takes too long.
	const double timeToSpend = endTime - startTime;
	const unsigned count = (unsigned)floor(
		std::max(timeToSpend / averageTime, 1.0)
	);
	
	// Run and add timings
	_runSome(client, count);
}
