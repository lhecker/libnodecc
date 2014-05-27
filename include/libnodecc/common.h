#ifndef node_cpp_common_h
#define node_cpp_common_h

#ifndef container_of
# define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#endif
