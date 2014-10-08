#include "lock.h"
#include <task/scheduler.h>

void lock_spin_wait(spinlock_t *lock)
{
	while(__sync_lock_test_and_set(lock, 1))
	{
		sched_yield();
	}
}
void lock_release(spinlock_t *lock)
{
	__sync_lock_release(lock);
}
