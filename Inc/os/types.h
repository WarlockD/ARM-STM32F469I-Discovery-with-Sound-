/* Copyright (c) 2013 The F9 Microkernel Project. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef TYPES_H_
#define TYPES_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys\types.h>
#include "f9_conf.h"

typedef uint32_t ptr_t;
typedef uintptr_t memptr_t;

typedef uint32_t l4_thread_t;

#include <stdbool.h>


#ifndef _SIZE_T_
#define _SIZE_T_
typedef uint32_t size_t;
#endif



#define ALIGNED(size, align) \
	(size / align) + ((size & (align - 1)) != 0)

#endif /* TYPES_H_ */
