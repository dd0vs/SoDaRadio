/*
  Copyright (c) 2012, Matthew H. Reilly (kb1vc)
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in
  the documentation and/or other materials provided with the
  distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "SoDaBase.hxx"
#include <string>
#include <iostream>

namespace SoDaTest {
  class ThN : public SoDa::SoDaThread {
  public:
    ThN() : SoDaThread("ThN") {};
    std::string name;
    void run() {
      while(1) {
	std::cerr << name << std::endl;
	boost::this_thread::sleep(boost::posix_time::seconds(1));
      }
    }
  };

  class Th0 : public ThN {
  public:
    Th0() { name = "zero"; }
  }; 
  class Th1 : public ThN {
  public:
    Th1() { name = "one"; }
  }; 
}

int main()
{
  SoDaTest::Th0 t0;
  SoDaTest::Th1 t1;

  t0.start();
  t1.start();

  for(int i = 0; i < 3; i++) {
    boost::this_thread::sleep(boost::posix_time::seconds(3));
    std::cerr << "main" << std::endl; 
  }
}
