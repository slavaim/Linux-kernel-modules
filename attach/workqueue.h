#ifndef WORQUEUEH
#define WORQUEUEH

int wq_init(void);
void wq_exit(void);
int add_entry(void (*fn)(void*), void* data);


#endif // WORQUEUEH
