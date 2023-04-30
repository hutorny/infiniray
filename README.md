## infiniray - Infinite Array
A memory mirrored ring buffer

### Introduction

Using `mmap` to create a mirrored region for a ring buffer is not a new idea,
there are several solution available [1], [2], [3], [4], [5], [6], [7], [8].
Although, each of **infiniray** features is not unique, their collection is believed
to be unique.

### Features

**infiniray** follows design of `std::array`/`std::vector`, featuring:
- header-only library, requiring only `stdlib`, `sys/mman.h` and `unistd.h`
- memory allocation abstracted into `allocator`
- element access operators
- `being`/`end` iterators
- initialization bypass for trivially constructible data type
- mirrored_region abstraction
- `fno-exceptions` compatibility
- Android support (at NDK level)

### Requirements
- C++17 capable compiler
- For Android: Android NDK

### Usage

1. Obtain the sources
2. Add `infiniray/include` to the include path
3. Use it in your code:

     ```
    #include <infiniray.h>                   // Include the main header
    infinite::array<long long> buffer(1024); // Declare an buffer of desired capacity
    //...
    buffer.append(data);                     // Append data at the end
    buffer.push_back(value);                 // push back an element
    buffer.emplace_back(value);              // emplace an element
    //...
    buffer.erase(512);                       // Erase elements in the front
    ```
4. To use on Android, provide a path to a temporary directory, accessible to the app with
`infinite::ashmem::region::settmpdir`, as in the following example:

    ```
    extern "C"
    JNIEXPORT void JNICALL
    Java_Infiniray_setTempDir(JNIEnv *env, jobject thiz, jstring jPath) {
        jboolean is_path_copy{};
        const char* const path = env->GetStringUTFChars(jPath, &is_path_copy);
        infinite::ashmem::region::settmpdir(path);
        if (is_path_copy)
            env->ReleaseStringUTFChars(jPath, path);
    }
    ```

### References

1. https://abhinavag.medium.com/a-fast-circular-ring-buffer-4d102ef4d4a3
2. https://github.com/smcho-kr/magic-ring-buffer
3. https://fgiesen.wordpress.com/2012/07/21/the-magic-ring-buffer/
4. https://gist.github.com/rygorous/3158316
5. https://github.com/lava/linear_ringbuffer
6. https://lo.calho.st/posts/black-magic-buffer/
7. https://github.com/tmick0/toy-queue/
8. https://github.com/google/wuffs/blob/main/script/mmap-ring-buffer.c
