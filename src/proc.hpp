/*                                              
**              mm                                      
**              MM                                      
** `7M'   `MF'mmMMmm .gP"Ya   ,6"Yb.  `7MMpMMMb.pMMMb.  
**   VA   ,V    MM  ,M'   Yb 8)   MM    MM    MM    MM  
**    VA ,V     MM  8M""""""  ,pm9MM    MM    MM    MM  
**     VVV      MM  YM.    , 8M   MM    MM    MM    MM  
**      W       `Mbmo`Mbmmd' `Moo9^Yo..JMML  JMML  JMML.
** 
** Copyright 2025 doublegsoft.ai
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and associated documentation files (the “Software”),
** to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense,
** and/or sell copies of the Software, and to permit persons to whom the
** Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
*/
#ifndef _PROCESS_EXECUTOR_HPP_
#define _PROCESS_EXECUTOR_HPP_

#pragma once

#include <string>
#include <vector>
#include <unistd.h>   // For fork, pipe, dup2, execvp, close, read
#include <sys/wait.h>  // For waitpid
#include <cstring>    // For strerror
#include <cerrno>     // For errno
#include <stdexcept>  // For std::runtime_error
#include <fcntl.h>    // For fcntl, O_NONBLOCK (optional non-blocking read)

namespace vt {
 
namespace proc {

struct Result {
  int return_code;
  std::string stdout_output;
  std::string stderr_output;
};

/*!
** Executes an external command and captures its standard output, standard error, and return code.
**
** @param command 
**          The command to execute (e.g., "ls", "grep", "python").
**
** @param args    
**          A vector of string arguments to pass to the command.
**
** @return An ExecutionResult struct containing the return code, stdout output, and stderr output.
**
** @throws std::runtime_error if there is an error during process execution (fork, pipe, execvp, read).
*/
Result execute(const std::string& command, const std::vector<std::string>& args) {
  int stdout_pipe[2]; // stdout_pipe[0] for reading, stdout_pipe[1] for writing
  int stderr_pipe[2]; // stderr_pipe[0] for reading, stderr_pipe[1] for writing
  pid_t pid;

  if (pipe(stdout_pipe) == -1) {
    throw std::runtime_error("pipe() failed for stdout: " + std::string(strerror(errno)));
  }
  if (pipe(stderr_pipe) == -1) {
    throw std::runtime_error("pipe() failed for stderr: " + std::string(strerror(errno)));
  }

  pid = fork();
  if (pid == -1) {
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[0]);
    close(stderr_pipe[1]);
    throw std::runtime_error("fork() failed: " + std::string(strerror(errno)));
  }

  if (pid == 0) { // Child process
    // Close read ends of pipes in child
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    // Redirect stdout to stdout_pipe
    if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1) {
      perror("dup2 stdout failed"); // Use perror in child process as exceptions might be problematic
      _exit(127); // Exit child process on error
    }
    close(stdout_pipe[1]); // No longer needed after dup2

    // Redirect stderr to stderr_pipe
    if (dup2(stderr_pipe[1], STDERR_FILENO) == -1) {
      perror("dup2 stderr failed");
      _exit(127);
    }
    close(stderr_pipe[1]); // No longer needed after dup2

    // Prepare arguments for execvp
    std::vector<char*> exec_args;
    exec_args.push_back(const_cast<char*>(command.c_str())); // command name as arg[0]
    for (const auto& arg : args) {
      exec_args.push_back(const_cast<char*>(arg.c_str()));
    }
    exec_args.push_back(nullptr); // execvp needs nullptr-terminated array

    // Execute the command
    execvp(command.c_str(), exec_args.data());

    // If execvp fails (returns), handle error
    perror("execvp failed"); // Use perror in child process
    _exit(127); // Exit child process on execvp failure

  } else { // Parent process
    // Close write ends of pipes in parent
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    std::string stdout_buffer;
    std::string stderr_buffer;
    char buffer[256]; // Buffer for reading from pipes
    ssize_t bytes_read;

    // Optionally set pipes to non-blocking for read (prevent deadlock if child produces too much output)
    // fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);
    // fcntl(stderr_pipe[0], F_SETFL, O_NONBLOCK);

    // Read stdout from pipe
    while ((bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
      buffer[bytes_read] = '\0'; // Null-terminate for string conversion
      stdout_buffer += buffer;
    }
    if (bytes_read == -1 && errno != EAGAIN && errno != EWOULDBLOCK) { // Check for read errors (excluding non-blocking would-block)
      close(stdout_pipe[0]);
      close(stderr_pipe[0]);
      throw std::runtime_error("read() from stdout pipe failed: " + std::string(strerror(errno)));
    }

    // Read stderr from pipe
    while ((bytes_read = read(stderr_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
      buffer[bytes_read] = '\0';
      stderr_buffer += buffer;
    }
    if (bytes_read == -1 && errno != EAGAIN && errno != EWOULDBLOCK) { // Check for read errors
      close(stdout_pipe[0]);
      close(stderr_pipe[0]);
      throw std::runtime_error("read() from stderr pipe failed: " + std::string(strerror(errno)));
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    int status;
    waitpid(pid, &status, 0); // Wait for child process to exit

    int return_code = 0;
    if (WIFEXITED(status)) { // Check if child process exited normally
      return_code = WEXITSTATUS(status); // Get exit status
    } else {
      // Process terminated by signal or other abnormal exit
      return_code = -1; // Indicate abnormal termination, or use a different value/mechanism to signal signal number etc.
    }

    return {return_code, stdout_buffer, stderr_buffer};
  }
}

};

};

#endif // _PROCESS_EXECUTOR_HPP_