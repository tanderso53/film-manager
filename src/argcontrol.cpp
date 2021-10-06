//===-- argcontrol.cpp - Argument Parsing Source -----* C++ *----===//
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
/// Implementation file for command line argument parsing.
///
//===------------------------------------------------------------===//

#include "argcontrol.h"
#include <assert.h>
#include <cstring>

film::ArgList::ArgList(int argc, const char** argv, const char* optl)
{
  assert(argc > 0);

  // Add the executable name to the argument list
  this->push_back(OptArg());
  this->back().assign(argv[0]);
  this->back().flags = this->back().flags | OA_EXEC;

  // Return if no options or arguments added
  if (argc < 2)
    return;

  // Run through the rest of the arguments
  for (int i = 1; i < argc; ++i) {
    this->push_back(OptArg());
    OptArg* oa = &this->back();
    char c = 'f';
    int j = 0;

    // Read character-by-character until null character
    for (;;) {
      char lastc = c;
      c = argv[i][j];

      if (c == '\0')
	break;

      // Check initial characters to determine wheather option, long
      // option, or arg
      if (c == '-' && j == 0) {
	oa->flags = oa->flags | OA_OPTION;
	j++;
	continue;
      }

      if (c == '-' && lastc == '-' && j == 1) {
	oa->flags = oa->flags | OA_LONG;
	j++;
	continue;
      }

      if (c != '-' && j == 0) {
	oa->flags = oa->flags | OA_ARG;
        auto rit = this->rbegin() + 1;

	if (rit->checkFlag(OA_OPTION))
	  rit->flags = rit->flags | OA_HAS_ARG;
	else
	  oa->flags = oa->flags | OA_BAD;

	oa->push_back(c);
	j++;
	continue;
      }

      if (oa->checkFlag(OA_LONG) && c == '=') {
	oa->flags = oa->flags | OA_HAS_ARG;
	this->push_back(OptArg());
	oa = &this->back();
	oa->flags = oa->flags | OA_ARG;
	j++;
	continue;
      }

      if (oa->checkFlag(OA_OPTION) && !oa->checkFlag(OA_LONG)) {
	if (lastc != '-') {
	  this->push_back(OptArg());
	  oa = &this->back();
	  oa->flags = oa->flags | OA_OPTION;
	}
	oa->push_back(c);
	j++;
	continue;
      }

      oa->push_back(c);
      j++;
    }
  }
}

int film::ArgList::getArg(char* buff, unsigned int n,
			  const char* option)
{
  if (int check = checkOption(option) > 0) {
    strncpy(buff, this->at(check).c_str(), n);
    return strlen(buff);
  }

  return -1;
}

int film::ArgList::checkOption(const char* option, const char* alias)
{
  for (int i = 0; i < this->size(); ++i) {
    if (!this->at(i).checkFlag(OA_OPTION))
      continue;

    bool check;

    if (strcmp(alias, "") == 0)
      check = this->at(i).compare(option) == 0;
    else
      check = (this->at(i).compare(option) == 0) || (this->at(i).compare(alias) == 0);

    if (check)
      return i;
  }

  return -1;
}

int film::ArgList::checkCmd(char* cmdbuf, unsigned int n)
{
  for (int i = 0; i < this->size(); ++i) {
    OptArg& oa = this->at(i);

    if (oa.checkFlag(OA_CMD)) {
      strncpy(cmdbuf, oa.c_str(), n);
      return i;
    }
  }

  return -1;
}
