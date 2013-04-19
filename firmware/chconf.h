#ifndef _CHCONF_H_
#define _CHCONF_H_

#define CH_FREQUENCY 1000 /* System tick frequency */
#define CH_TIME_QUANTUM 20 /* Round Robin interval, in ticks */
#define CH_MEMCORE_SIZE 0 /* Managed RAM size, 0 = all */
#define CH_NO_IDLE_THREAD FALSE /* Idle thread automatic spawn suppression */

#define CH_OPTIMIZE_SPEED TRUE /* Optimize for speed rather than space */

#define CH_USE_REGISTRY TRUE /* Build threads registry API */
#define CH_USE_WAITEXIT TRUE /* build chThdWait() */
#define CH_USE_SEMAPHORES TRUE /* Build semaphores APIs */
#define CH_USE_SEMAPHORES_PRIORITY FALSE /* Seamphore queue by priority rather than FIFO */
#define CH_USE_SEMSW TRUE /* Build chSemSignalWait() */
#define CH_USE_MUTEXES TRUE /* Build mutex APIs */
#define CH_USE_CONDVARS TRUE /* Build conditional variables APIs */
#define CH_USE_CONDVARS_TIMEOUT TRUE /* Build condvars with timeout support */
#define CH_USE_EVENTS TRUE /* Build event flags APIs */
#define CH_USE_EVENTS_TIMEOUT TRUE /* Build events with timeout support */
#define CH_USE_MESSAGES TRUE /* Build messages APIs */
#define CH_USE_MESSAGES_PRIORITY FALSE /* Serve message by priority rather than FIFO */
#define CH_USE_MAILBOXES TRUE /* Build mailbox APIs */
#define CH_USE_QUEUES TRUE /* Build I/O queues API */
#define CH_USE_MEMCORE TRUE /* Build core mm APIs */
#define CH_USE_HEAP TRUE /* Build heap allocator */
#define CH_USE_MALLOC_HEAP FALSE /* Heap allocatore uses malloc/free */
#define CH_USE_MEMPOOLS TRUE /* Build memory pools allocator APIs */
#define CH_USE_DYNAMIC TRUE /* Build Dynamic Threads APIs */

/* Debug */
#define CH_DBG_SYSTEM_STATE_CHECK FALSE /* Runtime state check */
#define CH_DBG_ENABLE_CHECKS FALSE /* Runtime parameters check */
#define CH_DBG_ENABLE_ASSERTS FALSE /* Enable runtime asserts */
#define CH_DBG_ENABLE_TRACE FALSE /* Enable trace buffer */
#define CH_DBG_ENABLE_STACK_CHECK FALSE /* Enable runtime stack check */
#define CH_DBG_FILL_THREADS FALSE /* fill threads working area */
#define CH_DBG_THREADS_PROFILING TRUE /* Count thread execution time */

/* Hooks */
#define THREAD_EXT_FIELDS
#define THREAD_EXT_INIT_HOOK(tp) { \
}
#define THREAD_EXT_EXIT_HOOK(tp) { \
}
#define THREAD_CONTEXT_SWITCH_HOOK(ntp, otp) { \
}
#define IDLE_LOOP_HOOK() { \
}
#define SYSTEM_TICK_EVENT_HOOK() { \
}
#define SYSTEM_HALT_HOOK() { \
}

#endif  /* _CHCONF_H_ */
