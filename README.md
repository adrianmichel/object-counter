[TOC]

# C++ Object Counter and Leak Detector #

## Introduction ##

It is straightforward nowadays to write C++ code that is “free” of memory leaks without having to give up the use of pointers, by using smart pointer classes such as std::auto_ptr or shared pointers such as std::shared_ptr or boost::shared_ptr.

However, complex software, or legacy code may still contain memory allocation bugs, which are traditionally quite hard to detect and fix. Memory debugging libraries or programs can be used to debug such issues, but they usually provide low level information which needs to be further interpreted in the context of the application to be useful.

The utility classes presented here can be used to do object level allocation tracking, thus offering a higher level view of the object creation and destruction behavior. They require minimal code changes and have very little memory usage or performance overhead. Furthermore, they are disabled in release mode.

## Detecting object leaks ##
Here is a typical usage example:

Without object counter:


```
#!C++

class C {
                // members
};
```


With object counter:


```
#!C++

class C {
                OBJ_COUNTER( C )

                // members
};

```

Once added to a class, the object counter will identify each new instance of that class by an integer value, or id (unique for all objects of a class) that will be stored in a per class static container for tracking. When an instance is destructed, the corresponding id will be removed from the container.

If all instances of a class have been destructed upon application exit, a success message is logged.

If there are still objects that haven’t been destructed, an object leak message is logged, indicating the class name and some extra information about the current status: the number of instances still in existence, the maximum number of instances at any one time, as well as a list of ids of leaked objects.

An assert is triggered if the application attempts to destruct an object with an id that is not found, usually indicating that an object is destructed multiple times.

## Debugging object leaks ##
The object counter provides code to help with debugging object creation/destruction issues as well.

If object leaks have been detected it is useful to run the application several times and determine if there is a pattern in the number and indexes of the leaked objects.

If an object with the same id between application run is leaked, a second object counter macro can be used to break when this specific instance is created. For example, if the leaked object id is 5, the usage is as follows:


```
#!C++

class C {
                OBJ_COUNTER_BREAK_ON_INDEX( C , 5 )
                // members
};
```


When the class C instance index 5 is created, the execution breaks into the debugger by using the __asm int 3; (or DebugBreak() ) statement. It is then possible to examine the context, call stack etc to find the reason for the leak.

## Multi-threaded execution ##
The current implementation works well in a single-threaded environment (the application can be multithreaded, but the objects themselves should be created/destructed within the same thread). If the objects being tracked are created/destructed from multiple threads the object counter classes will require thread synchronization to work properly.

The object counter classes make use of a placeholder Lockable class, which in a real multi-threaded application will have to implement the synchronization methods.

## Logging ##
The code uses the boost logging macro BOOST_LOG_TRIVIAL, however this can be easily replaced with any other logging class or macro.

## Static instances ##
The object counter utility classes rely on the fact that static variables are destructed when the process exits, which ensures the object counting works well with heap (allocated using new) and local variables. If however the instances being tracked are themselves static, the object counter utility may not work anymore, as C++ doesn’t guarantee the order in which static variables are allocated or destructed. It is therefore possible that the tracking static variable be destructed before the tracked instances, thus detecting false leaks.

