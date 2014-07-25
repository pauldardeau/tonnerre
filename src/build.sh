#!/bin/sh

CC=clang++
CC_OPTS="-c -std=c++11"

$CC $CC_OPTS EpollServer.cpp
$CC $CC_OPTS IniReader.cpp
$CC $CC_OPTS InvalidKeyException.cpp
$CC $CC_OPTS KernelEventServer.cpp
$CC $CC_OPTS KeyValuePairs.cpp
$CC $CC_OPTS KqueueServer.cpp
$CC $CC_OPTS Logger.cpp
$CC $CC_OPTS Message.cpp
$CC $CC_OPTS MessageRequestHandler.cpp
$CC $CC_OPTS Messaging.cpp
$CC $CC_OPTS MessagingServer.cpp
$CC $CC_OPTS PthreadsConditionVariable.cpp
$CC $CC_OPTS PthreadsMutex.cpp
$CC $CC_OPTS PthreadsThread.cpp
$CC $CC_OPTS RequestHandler.cpp
$CC $CC_OPTS ServerSocket.cpp
$CC $CC_OPTS ServiceInfo.cpp
$CC $CC_OPTS Socket.cpp
$CC $CC_OPTS SocketRequest.cpp
$CC $CC_OPTS SocketServer.cpp
$CC $CC_OPTS StdConditionVariable.cpp
$CC $CC_OPTS StdLogger.cpp
$CC $CC_OPTS StdMutex.cpp
$CC $CC_OPTS StdThread.cpp
$CC $CC_OPTS StrUtils.cpp
$CC $CC_OPTS StringTokenizer.cpp
$CC $CC_OPTS Thread.cpp
$CC $CC_OPTS ThreadPool.cpp
$CC $CC_OPTS ThreadPoolDispatch.cpp
$CC $CC_OPTS ThreadPoolQueue.cpp
$CC $CC_OPTS ThreadPoolWorker.cpp
$CC $CC_OPTS ThreadingFactory.cpp
