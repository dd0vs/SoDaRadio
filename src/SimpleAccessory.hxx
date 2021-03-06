/*
  Copyright (c) 2019 Matthew H. Reilly (kb1vc)
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

#ifndef SODA_SIMPLE_ACC_HDR
#define SODA_SIMPLE_ACC_HDR

#include "SoDaBase.hxx"
#include "SoDaThread.hxx"

/**
 * @file SimpleAccessory.hxx
 *
 * an example of a simple bolt-on accessory for SoDaRadio.  It counts
 * the number of commands that go by and reports the count at the end
 * of the run. 
 *
 * @author Matt Reilly (kb1vc)
 *
 */

// namespace doesn't matter here... let's do without. 
class SimpleAccessory : public SoDa::Thread {
public:
  SimpleAccessory(const std::string & name);

  /**
   * @brief connect to useful mailboxes. 
   * 
   * @param mbox_name which mailbox are we being offered? 
   * @param mbox_p a pointer to the mailbox we are being offered. 
   */
  void subscribeToMailBox(const std::string & mbox_name, SoDa::BaseMBox * mbox_p);

  void run();

  /**
   * @brief execute GET commands from the command channel
   * @param cmd the incoming command
   */
  void execGetCommand(SoDa::Command * cmd) {
    get_count++; 
  }
  /**
   * @brief handle SET commands from the command channel
   * @param cmd the incoming command
   */
  void execSetCommand(SoDa::Command * cmd) {
    set_count++;
  }
  /**
   * @brief handle Report commands from the command channel
   * @param cmd the incoming command
   */
  void execRepCommand(SoDa::Command * cmd) {
    rep_count++;
  }

  void shutDown() {
    printReport();
  }

  void printReport();

  unsigned int cmd_subs; ///< mailbox subscription ID for command stream
  SoDa::CmdMBox * cmd_stream; ///< mailbox producing command stream from user
    
  unsigned int get_count, set_count, rep_count; 
};

#endif
