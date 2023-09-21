#pragma once

#define SLOCK_INIT 0

typedef bool slock_t;

/**
 * @brief Acquire a spinclock
 * @warning Spins until acquired
 *
 * @param lock Spinlock
 */
void slock_acquire(volatile slock_t *lock);

/**
 * @brief Try acquire a spinlock
 * @warning Does not spin, only tries to acquire the lock once
 *
 * @param slock Spinlock
 * @return true = acquired the lock
 */
static inline bool slock_try_acquire(volatile slock_t *lock) {
    return  !__atomic_test_and_set(lock, __ATOMIC_ACQUIRE);
}

/**
 * @brief Release a spinlock
 */
static inline void slock_release(volatile slock_t *lock) {
    __atomic_clear(lock, __ATOMIC_RELEASE);
}