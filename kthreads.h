// Some docs

struct kthread {
  int pid;
};


typedef struct kthread kthread_t;
typedef uint lock_t;

struct kthread thread_create(void (*start_routine)(void*), void *arg);

void thread_join(struct kthread);

void lock_acquire(volatile lock_t *);

void lock_release(volatile lock_t *);

void init_lock(lock_t *); 
