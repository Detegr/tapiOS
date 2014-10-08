#ifndef _TAPIOS_LOCK_H_
#define _TAPIOS_LOCK_H_

#include <stdint.h>

// According to Intel documentation, either int/long/uint/ulong should be used
typedef volatile unsigned int spinlock_t;

void lock_spin_wait(spinlock_t *lock);
void lock_release(spinlock_t *lock);

#endif
