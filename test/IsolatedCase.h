// Copyright Paul Dardeau, SwampBits LLC 2026
// BSD License

#ifndef TONNERRE_TEST_ISOLATEDCASE_H
#define TONNERRE_TEST_ISOLATEDCASE_H

#include <functional>
#include <string>

namespace tonnerre_test {

/**
 * Outcome of running a function in a forked child process under a hard
 * timeout. POSIX only (fork/waitpid); tests that need it are not built on
 * Windows.
 */
struct IsolatedResult {
   enum Outcome {
      Exited,        // child called _exit(exitCode)
      Signaled,      // child was terminated by signalNumber
      TimedOut,      // child exceeded the timeout and was killed with SIGKILL
      StartFailed    // fork or log-file creation failed
   };

   Outcome outcome = StartFailed;
   int exitCode = -1;
   int signalNumber = 0;
   long elapsedMillis = 0;
   std::string log;                 // child's stdout+stderr (truncated)
   std::string sanitizerFinding;    // first sanitizer line found in the log, if any
};

/**
 * Runs body() in a forked child with stdout/stderr redirected to a log file,
 * core dumps disabled and an alarm() backstop. The parent polls with
 * waitpid and sends SIGKILL after timeoutMillis. body's return value becomes
 * the child's exit status. Nothing the child does can affect the parent's
 * memory or file descriptors.
 */
IsolatedResult runIsolated(const std::function<int()>& body, int timeoutMillis);

/**
 * Human-readable name of a signal number (SIGSEGV, SIGABRT, ...).
 */
std::string signalName(int signalNumber);

}

#endif
