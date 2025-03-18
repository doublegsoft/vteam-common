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
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h> 
#endif
#include <cstring>    
#include <cerrno>
#include <stdexcept> 
#include <fcntl.h>    

#if _WIN32
std::string ReadFromPipe(HANDLE hPipe) {
  const int BUFFER_SIZE = 4096;
  char buffer[BUFFER_SIZE];
  DWORD bytesRead;
  std::string output;

  while (ReadFile(hPipe, buffer, BUFFER_SIZE - 1, &bytesRead, NULL) && bytesRead != 0) {
      buffer[bytesRead] = '\0';
      output += buffer;
  }
  return output;
}
#endif

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
  int return_code = 0;
  std::string stdout_buffer;
  std::string stderr_buffer;
#if _WIN32  
  SECURITY_ATTRIBUTES saAttr;
  HANDLE hRead, hWrite;

  // Set the bInheritHandle flag on the security attributes structure.
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  // Create a pipe for the child process's STDOUT.
  if (!CreatePipe(&hRead, &hWrite, &saAttr, 0)) {
    fprintf(stderr, "StdoutRd CreatePipe failed.\n"); 
  }

  STARTUPINFOW si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.hStdOutput = hWrite;
  si.hStdError = hWrite;       // Redirect STDERR to the same pipe
  si.dwFlags |= STARTF_USESTDHANDLES;

  ZeroMemory(&pi, sizeof(pi));

  // Use wide strings (Unicode)
  wchar_t appName[] = L"C:\\Windows\\System32\\cmd.exe"; // Full path to cmd.exe
  wchar_t commandLine[] = L"cmd.exe /c dir"; // Command to execute

  // Start the child process.
  if (!CreateProcessW(appName,     
                      commandLine,  
                      NULL,         
                      NULL,         
                      TRUE,         
                      CREATE_NO_WINDOW,
                      NULL,          
                      NULL,         
                      &si,
                      &pi))
  {
    fprintf(stderr, "CreateProcess failed (%d).\n", GetLastError());
  }

  // Close the write end of the pipe (parent doesn't write to the pipe).
  CloseHandle(hWrite);

  // Read output from the child process
  std::string output = ReadFromPipe(hRead);
  printf("Child process output:\n%s\n", output.c_str());

  // Wait until child process exits.
  WaitForSingleObject(pi.hProcess, INFINITE);

  // Close process and thread handles.
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  CloseHandle(hRead); // Close the read handle
#else
  pid_t pid;
  int stdout_pipe[2]; // stdout_pipe[0] for reading, stdout_pipe[1] for writing
  int stderr_pipe[2]; // stderr_pipe[0] for reading, stderr_pipe[1] for writing
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

    if (WIFEXITED(status)) { // Check if child process exited normally
      return_code = WEXITSTATUS(status); // Get exit status
    } else {
      // Process terminated by signal or other abnormal exit
      return_code = -1; // Indicate abnormal termination, or use a different value/mechanism to signal signal number etc.
    }
#endif
    return {return_code, stdout_buffer, stderr_buffer};
  }

}; // namespace proc

}; // namespace vt

#endif // _PROCESS_EXECUTOR_HPP_