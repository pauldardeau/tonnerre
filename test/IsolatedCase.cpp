// Copyright Paul Dardeau, SwampBits LLC 2026
// BSD License

#include "IsolatedCase.h"

#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <thread>

#include <fcntl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace tonnerre_test {

static const std::size_t MAX_LOG_BYTES = 65536;

//******************************************************************************

std::string signalName(int signalNumber) {
   switch (signalNumber) {
      case SIGSEGV: return "SIGSEGV";
      case SIGABRT: return "SIGABRT";
      case SIGBUS:  return "SIGBUS";
      case SIGFPE:  return "SIGFPE";
      case SIGILL:  return "SIGILL";
      case SIGPIPE: return "SIGPIPE";
      case SIGKILL: return "SIGKILL";
      case SIGALRM: return "SIGALRM";
      case SIGTERM: return "SIGTERM";
      default:      return "signal " + std::to_string(signalNumber);
   }
}

//******************************************************************************

static std::string findSanitizerFinding(const std::string& log) {
   static const char* const markers[] = {
      "ERROR: AddressSanitizer",
      "ERROR: LeakSanitizer",
      "runtime error:",
      "WARNING: ThreadSanitizer",
      "ERROR: UndefinedBehaviorSanitizer"
   };

   std::size_t lineStart = 0;
   while (lineStart < log.size()) {
      std::size_t lineEnd = log.find('\n', lineStart);
      if (lineEnd == std::string::npos) {
         lineEnd = log.size();
      }
      const std::string line = log.substr(lineStart, lineEnd - lineStart);
      for (const char* marker : markers) {
         if (line.find(marker) != std::string::npos) {
            return line;
         }
      }
      lineStart = lineEnd + 1;
   }

   return std::string();
}

//******************************************************************************

IsolatedResult runIsolated(const std::function<int()>& body,
                           int timeoutMillis) {
   IsolatedResult result;

   char logPath[] = "/tmp/tonnerre_isoXXXXXX";
   const int logFd = ::mkstemp(logPath);
   if (logFd < 0) {
      return result;
   }

   // anything still buffered would otherwise be written by both processes
   fflush(stdout);
   fflush(stderr);

   const auto started = std::chrono::steady_clock::now();
   const pid_t pid = ::fork();

   if (pid < 0) {
      ::close(logFd);
      ::unlink(logPath);
      return result;
   }

   if (pid == 0) {
      // child
      struct rlimit noCore;
      noCore.rlim_cur = 0;
      noCore.rlim_max = 0;
      ::setrlimit(RLIMIT_CORE, &noCore);

      ::dup2(logFd, STDOUT_FILENO);
      ::dup2(logFd, STDERR_FILENO);
      ::close(logFd);

      // backstop in case the parent's kill never arrives
      ::alarm(static_cast<unsigned int>(timeoutMillis / 1000 + 10));

      int exitCode = 98;
      try {
         exitCode = body();
      } catch (...) {
         exitCode = 97;
      }

      fflush(stdout);
      fflush(stderr);
      ::_exit(exitCode);
   }

   // parent
   ::close(logFd);

   int status = 0;
   bool reaped = false;
   bool timedOut = false;

   while (!reaped) {
      const pid_t rc = ::waitpid(pid, &status, WNOHANG);
      if (rc == pid) {
         reaped = true;
      } else if (rc < 0) {
         break;
      } else {
         const auto now = std::chrono::steady_clock::now();
         const long elapsed = static_cast<long>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - started).count());
         if (elapsed >= timeoutMillis) {
            ::kill(pid, SIGKILL);
            ::waitpid(pid, &status, 0);
            timedOut = true;
            reaped = true;
         } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
         }
      }
   }

   result.elapsedMillis = static_cast<long>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
         std::chrono::steady_clock::now() - started).count());

   if (timedOut) {
      result.outcome = IsolatedResult::TimedOut;
   } else if (reaped && WIFEXITED(status)) {
      result.outcome = IsolatedResult::Exited;
      result.exitCode = WEXITSTATUS(status);
   } else if (reaped && WIFSIGNALED(status)) {
      result.outcome = IsolatedResult::Signaled;
      result.signalNumber = WTERMSIG(status);
   }

   {
      std::ifstream logFile(logPath, std::ios::binary);
      std::string content((std::istreambuf_iterator<char>(logFile)),
                          std::istreambuf_iterator<char>());
      if (content.size() > MAX_LOG_BYTES) {
         content.resize(MAX_LOG_BYTES);
      }
      result.log = content;
   }

   ::unlink(logPath);
   result.sanitizerFinding = findSanitizerFinding(result.log);

   return result;
}

}
