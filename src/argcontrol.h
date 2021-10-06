//===-- argcontrol.h - Argument Parsing Header -------* C++ *----===//
//
// Part of film-manager project, Copyright 2021 Tyler J. Anderson This
// software is released under the BSD 3-Clause "New" or "Revised"
// License. You should have received a copy of the license with this
// source distribution
// 
// SPDX-License-Identifier: BSD-3-Clause
//
//===------------------------------------------------------------===//
///
/// \file
/// Header to handle comand line option and argument parsing.
///
//===------------------------------------------------------------===//

#ifndef ARGCONTROL_H
#define ARGCONTROL_H

#include <iostream>
#include <string>
#include <vector>

/// Load args into an arg object
#define OA_OPTION 0x01
#define OA_ARG 0x02
#define OA_LONG 0x03
#define OA_HAS_ARG 0x04
#define OA_LAST 0x05
#define OA_BAD 0x06
#define OA_EXEC 0x07
#define OA_CMD 0x08

namespace film {
  struct OptArg : public std::string {
    uint8_t flags = 0;
    bool checkFlag(uint8_t flag) {return (flags & flag) == flag;};
  };

  class ArgList : public std::vector<OptArg> {
  public:
    ArgList(int argc, const char** argv, const char* optl = "");

    /// Fill buffer with argument for option option up to length n
    /// characters or null character. Exits characters read on success,
    /// -1 if option does not exist.
    int getArg(char* buff, unsigned int n,
	       const char* option = "");

    /// Check for option, return index if it exists, -1 if it does not
    int checkOption(const char* option, const char* alias = "");

    /// Check to see if command is present, -1 if not present
    int checkCmd(char* cmdbuf, unsigned int n);
  };
}

#endif // #ifndef ARGCONTROL_H

