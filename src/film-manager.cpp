//===-- film-manager.cpp - main application src file ------------===//
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
/// This file contains the top-level application implementation for
/// the film manager front-end program.
///
//===------------------------------------------------------------===//

#include "fields_magic.h"
#include "backend.h"
#include "argcontrol.h"

#include <vector>
#include <string>
#include <cstring>
#include <utility>
#include <iostream>
#include <cstdarg>
#include <assert.h>

/// Generate the list of fields to fill out
void generateFields(std::vector<formdata>& _formdata,
		    const std::vector<const char*>& _labels)
{
  // Copy labels to formdata struct
  for (uint16_t i = 0; i < _labels.size(); ++i) {
    _formdata.push_back(formdata());
    formdata& fd = _formdata.back();
    fd = {
      .name = "",
      .data = ""
    };
    strncpy(fd.name, _labels[i], sizeof(fd.name)/sizeof(char));
  }
}

/// Export field information to backend
int processFields(film::Backend& _backend,
		  const std::vector<formdata>& _fd)
{
  std::vector<const char*> labels;
  std::vector<const char*> datas;

  assert(_fd.size() > 0);

  for (uint16_t i = 0; i < _fd.size(); ++i) {
    labels.push_back(_fd[i].name);
    datas.push_back(_fd[i].data);
  }

  _backend.send(&labels, &datas);

  return 0;
}

int printUsage(const film::ArgList& al,
	       std::ostream& out = std::cout)
{
  out << "Usage:\n"
      << al[0] << '\n'
      << al[0] << " -h | --help\n"
      << '\n'
      << "Options:\n"
      << "-h | --help\t\tPrint this help message\n";
  return 0;
}

int main(int argc, const char** argv)
{
  // Read arguments
  film::ArgList al(argc, argv);

  // Print help message if help flag is given
  if (al.checkOption("h", "help") != -1) {
    printUsage(al);
    return 0;
  }

  // Temporary definitions for testing
  std::vector<formdata> fd;
  std::vector<const char*> labels = {
    {
      "Location",
      "Subject",
      "Date/time",
      "Camera",
      "Film Type",
      "Film Set"
    }
  };
  film::TextBackend tb;

  // Populate the list of formdata from label
  generateFields(fd, labels);

  assert(fd.size() > 0);

  // Start the form
  if (buildForm(&fd[0], fd.size()) != 0) {
    std::cerr << "Form interface ended in complete failure" << '\n';
    return 1;
  }

  // Send data to backend
  if (processFields(tb, fd) != 0) {
    std::cerr << "Failed to process fields";
    return 1;
  }

  return 0;
}
