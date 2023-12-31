#pragma once

#define SLOCK_INIT 0

typedef bool slock_t;

/**
 * @brief Acquire a spinlock.
 * @warning Spins until acquired.
 * @param lock
 */
void spinlock_acquire(volatile slock_t *lock);

/**
 * @brief Try to acquire a spinlock.
 * @warning Does not spin, only tries to acquire the lock once.
 * @param lock
 * @returns true = acquired the lock
 */
static inline bool spinlock_try_acquire(volatile slock_t *lock) {
    return  !__atomic_test_and_set(lock, __ATOMIC_ACQUIRE);
}

/**
 * @brief Release a spinlock.
 * @param lock
 */
static inline void spinlock_release(volatile slock_t *lock) {
    __atomic_clear(lock, __ATOMIC_RELEASE);
}