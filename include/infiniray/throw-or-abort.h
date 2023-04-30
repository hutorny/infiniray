#pragma once
#ifndef infiniray_throw_or_abort
# if defined(__cpp_exceptions)
#   define infiniray_throw_or_abort(e) (throw (e))
# else
#  include <cassert>
#   define infiniray_throw_or_abort(e) (assert((e,1)), abort())
# endif
#endif

 // namespace 
