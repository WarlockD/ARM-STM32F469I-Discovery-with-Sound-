#ifndef __DEVICE_HPP__
#define __DEVICE_HPP__

#include <os/types.h>
#include <queue.h>

typedef struct __dev_t {
	TREE_ENTRY(__dev_t) children;
	int (*probe)();
	int (*open)(const char* name, uint32_t flags);
	int (*close)();
	int (*ioctrl)(uint32_t flags, void* args, size_t size);

} dev_t;

#endif
