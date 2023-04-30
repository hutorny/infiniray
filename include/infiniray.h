/*
 * Copyright (C) 2023 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * infiniray.h - Infinite Array primary header
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */
#pragma once

/*
 * Infinite array - a memory-mapped ring buffer
 */

#include <infiniray/infinite-array.h>
#ifdef ANDROID
#  include <infiniray/android.h>
#endif
#include <infiniray/mirror-mmap.h>
