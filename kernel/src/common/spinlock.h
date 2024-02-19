#pragma once

#define SPINLOCK_INIT 0

typedef bool spinlock_t;

/**
 * @brief Acquire a spinlock
 * @warning Spins until acquired
 */
void spinlock_acquire(volatile spinlock_t *lock);

/**
 * @brief Try to acquire a spinlock
 * @warning Does not spin, only tries to acquire the lock once
 * @returns true = acquired the lock
 */
static inline bool spinlock_try_acquire(volatile spinlock_t *lock) {
    return  !__atomic_test_and_set(lock, __ATOMIC_ACQUIRE);
}

/**
 * @brief Release a spinlock
 */
static inline void spinlock_release(volatile spinlock_t *lock) {
    __atomic_clear(lock, __ATOMIC_RELEASE);
}