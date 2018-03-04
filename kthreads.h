// Some docs

struct kthread {
  int pid;
};

struct lock {
  volatile unsigned int lock_value;
};

typedef struct kthread kthread_t;
typedef struct lock lock_t;

struct kthread thread_create(void (*start_routine)(void*), void *arg);

void thread_join(struct kthread);

void lock_acquire(lock_t *);

void lock_release(lock_t *);

void init_lock(lock_t *); 
