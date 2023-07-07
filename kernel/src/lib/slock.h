#ifndef LIB_SLOCK_H
#define LIB_SLOCK_H

#define SLOCK_INIT 0

typedef unsigned char slock_t;

/**
 * @brief Acquire a spinclock
 * @warning Spins until acquired
 * 
 * @param lock Spinlock
 */
void slock_acquire(slock_t *lock);

/**
 * @brief Try acquire a spinlock
 * @warning Does not spin, only tries to acquire the lock once
 *
 * @param slock Spinlock
 * @return true = acquired the lock
 */
static inline bool slock_try_acquire(slock_t *lock) {
    return __sync_bool_compare_and_swap(lock, 0, 1);
}

/**
 * @brief Release a spinlock
 */
static inline void slock_release(slock_t *lock) {
    __atomic_store_n(lock, 0, __ATOMIC_SEQ_CST);
}

#endif