#ifndef nodecc_common_h
#define nodecc_common_h

#ifndef container_of
# define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#endif // nodecc_common_h
