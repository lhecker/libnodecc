#ifndef nodecc_common_h
#define nodecc_common_h

/**
 * container_of returns the struct/class of type "type"
 * by providing a pointer "ptr" on the member type::member.
 */
#ifndef container_of
# define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#endif // nodecc_common_h
