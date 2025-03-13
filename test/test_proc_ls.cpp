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
#include <iostream>
#include "proc.hpp"

int main() {
  try {
    vt::proc::Result result = vt::proc::execute("ls", {"-l", "."});

    std::cout << "Return Code: " << result.return_code << std::endl;
    std::cout << "Stdout:\n" << result.stdout_output << std::endl;
    std::cout << "Stderr:\n" << result.stderr_output << std::endl;

    if (result.return_code != 0) {
      std::cerr << "Command execution failed." << std::endl;
    }

    vt::proc::Result error_result = vt::proc::execute("non_existent_command", {});
    std::cout << "\nError Command Return Code: " << error_result.return_code << std::endl;
    std::cout << "Error Command Stderr:\n" << error_result.stderr_output << std::endl;


  } catch (const std::runtime_error& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}