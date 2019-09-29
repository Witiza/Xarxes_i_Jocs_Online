#include "ModuleTaskManager.h"


void ModuleTaskManager::threadMain()
{
	while (true)
	{
		// TODO 3:
		// - Wait for new tasks to arrive
		// - Retrieve a task from scheduledTasks
		// - Execute it
		// - Insert it into finishedTasks
		Task* t = nullptr;
		{
			std::unique_lock<std::mutex> lock(mtx);
			while (t == nullptr)
			{
				if (scheduledTasks.empty())
				{
					_event.wait(lock);
				}
				else
				{
					t = scheduledTasks.front();
					scheduledTasks.pop();

					t->execute();

					finishedTasks.push(t);
				}
			}
		}
	}
}

bool ModuleTaskManager::init()
{
	// TODO 1: Create threads (they have to execute threadMain())
	for (int i = 0; i < MAX_THREADS; i++)
	{
		threads[i] = std::thread(&ModuleTaskManager::threadMain, this);
		//https://stackoverflow.com/questions/40353512/stdinvoke-no-matching-overloaded-function-found-error-given-in-vs-2015
		//https://stackoverflow.com/questions/28050325/illegal-operation-on-bound-member-function-expression
	}
	return true;
}

bool ModuleTaskManager::update()
{
	// TODO 4: Dispatch all finished tasks to their owner module (use Module::onTaskFinished() callback)
	
	while(!finishedTasks.empty())
	{
		auto item = finishedTasks.front();
		item->owner->onTaskFinished(item);
		finishedTasks.pop();
	}
	
	return true;
}

bool ModuleTaskManager::cleanUp()
{
	// TODO 5: Notify all threads to finish and join them
	for (int i = 0; i < MAX_THREADS; i++)
	{
		threads[i].join();
	}

	return true;
}

void ModuleTaskManager::scheduleTask(Task *task, Module *owner)
{
	task->owner = owner;
	{
		std::unique_lock<std::mutex> lock(mtx);

		scheduledTasks.push(task);
	}
	_event.notify_one();
	// TODO 2: Insert the task into scheduledTasks so it is executed by some thread
}
