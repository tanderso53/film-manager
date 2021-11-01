//===-- backend.cpp - Backend Connection Source -----------------===//
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
/// This file contains the implementation for the available
/// implementation for backend data transfer systems.
///
//===------------------------------------------------------------===//

#include "backend.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <assert.h>
#include <cstdarg>

film::TextBackend::TextBackend(std::ostream& outstream)
  :_outstream(outstream)
{
  flags = flags | FM_BE_RECEIVE_ENABLED;
  init();
}

void film::TextBackend::send(std::vector<const char*>* v...)
{
  std::va_list args;

  assert(v);

  va_start(args, v);

  std::vector<const char*>* varg = va_arg(args,
					  std::vector<const char*>*);

  assert(varg);

  _outstream << "{\n";

  for (uint16_t i = 0; i < v->size(); ++i) {
    _outstream << '\t' << "\"" << v->at(i) << "\": "
	       << "\"" << varg->at(i) << "\"";

    if (i < v->size() - 1)
      _outstream << ",";

    _outstream << '\n';
  }

  _outstream << "}\n";
}

const char* film::TextBackend::receive(const char* query)
{
  if ((flags & FM_BE_RECEIVE_ENABLED) == 0) {
    return "";
  }

  assert(query);

  std::ifstream datafile;

  resultbuffer.clear();
  datafile.open(query);

  if (datafile.fail()) {
    std::runtime_error("Failed to open file for receive");
  }

  while (!datafile.eof()) {
    std::string line;
    getline(datafile, line);

    std::cerr << line << '\n';

    if (!line.empty())
      resultbuffer.push_back(move(line));
  }

  datafile.close();

  return "";
}

void film::TextBackend::connect() {};

void film::TextBackend::init() {};

std::vector<const char*> film::Backend::results()
{
  std::vector<const char*> v;
  for (auto it = resultbuffer.begin(); it != resultbuffer.end(); ++it) {
    v.push_back(it->c_str());
  }

  return v;
}
