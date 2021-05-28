# build service
set(SOURCE_FILES
    st_blocking/ServerImpl.cpp
    mt_blocking/ServerImpl.cpp

    st_nonblocking/ServerImpl.cpp
    st_nonblocking/Connection.cpp
    st_nonblocking/Utils.cpp

    st_coroutine/ServerImpl.cpp
    st_coroutine/Connection.cpp
    st_coroutine/Utils.cpp

    mt_nonblocking/ServerImpl.cpp
    mt_nonblocking/Connection.cpp
    mt_nonblocking/Worker.cpp
    mt_nonblocking/Utils.cpp
)

add_library(Network ${SOURCE_FILES})
<<<<<<< HEAD
target_link_libraries(Network pthread Logging Protocol Execute Coroutine Concurrency ${CMAKE_THREAD_LIBS_INIT})

||||||| 4f4a358
target_link_libraries(Network pthread Logging Protocol Execute Coroutine ${CMAKE_THREAD_LIBS_INIT})
=======
target_link_libraries(Network pthread Logging Protocol Execute Coroutine Concurrency ${CMAKE_THREAD_LIBS_INIT})
>>>>>>> 018c31987091fc776b09234c3ebad4a6d1a15383
