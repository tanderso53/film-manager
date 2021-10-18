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

#include <vector>
#include <string>
#include <cstring>
#include <utility>
#include <iostream>
#include <assert.h>
#include <getopt.h>

// Bring in variables from getopt library
extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

// Program operating modes
#define FM_OP_USAGE		0x01
#define FM_OP_VERSION		0x02
#define FM_OP_INTERACTIVE	0x03
#define FM_OP_BE_TEXT		0x04

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

int printUsage(int _argc, const char** _argv,
	       std::ostream& out = std::cout)
{
  assert(_argc > 0);

  out << "Usage:\n"
      << _argv[0] << " [ -i | --interactive ]" << '\n'
      << _argv[0] << " -h | --help\n"
      << _argv[0] << " -V | --version \n"
      << '\n'
      << "Options:\n"
      << "-i | --interactive\t\tRun in interactive mode (Default)\n"
      << "-b | --backend name\t\tName of data backend to use (Default text)\n"
      << "-V | --version\t\t\tPrint version information and exit\n"
      << "-h | --help\t\t\tPrint this help message\n"
      << '\n'
      << "Available Backend Handlers:\n"
      << "text\n";

  return 0;
}

int printVersion(std::ostream& out = std::cout)
{
#ifndef FM_VERSION
#define FM_VERSION "unknown"
#endif
  out << "Film Manager Version " << FM_VERSION << '\n';

  return 0;
}

int runInteractive(std::vector<formdata>& _formdata,
		   std::vector<const char*>& _labels)
{
  // Populate the list of formdata from label
  generateFields(_formdata, _labels);

  assert(_formdata.size() > 0);

  // Start the form
  if (buildForm(&_formdata[0], _formdata.size()) != 0) {
    std::cerr << "Form interface ended in complete failure" << '\n';
    return 1;
  }

  return 0;
}

int runParseOptions(int _argc, const char** _argv, uint8_t _modereg)
{
    // Read arguments
  struct option lopts[] = {
    {
      .name = "help",
      .has_arg = no_argument,
      .flag = NULL,
      .val = 'h'
    },

    {
      .name = "version",
      .has_arg = no_argument,
      .flag = NULL,
      .val = 'V'
    },

    {
      .name = "interactive",
      .has_arg = no_argument,
      .flag = NULL,
      .val = 'i'
    },

    {
      .name = "backend",
      .has_arg = required_argument,
      .flag = NULL,
      .val = 'b'
    },

    {
      .name = NULL,
      .has_arg = 0,
      .flag = NULL,
      .val = 0
    }
  };

  int ch;

  // Cast is required on Fedora Linux with GNU libc. May break with
  // other implementations
  while ((ch = getopt_long(_argc, (char * const *) _argv,
			   "hVib:", lopts, NULL)) != -1) {
    switch (ch) {

    case 'h':
      printUsage(_argc, _argv);
      exit(0);

    case 'V':
      printVersion();
      exit(0);

    case 'i':
      _modereg = _modereg | FM_OP_INTERACTIVE;
      break;

    case 'b':
      assert(optarg);

      if (strcmp(optarg, "text") == 0) {
	_modereg = _modereg | FM_OP_BE_TEXT;
	break;
      }

      std::cerr << "Backend handler " << optarg << " not found\n\n"
		<< "Available backend handlers:\n"
		<< "text\n";

      exit(1);

    case '?':
      printUsage(_argc, _argv);
      exit(1);

    default:
      break;

    }
  }

  return 0;
}

int main(int argc, const char** argv)
{
  // Registor to report active option flags
  uint8_t modeReg = 0;

  // Set program defaults
  modeReg = modeReg | FM_OP_INTERACTIVE | FM_OP_BE_TEXT;

  // Ptr to set to desired backend
  film::Backend* beptr = nullptr;

  // Parse CLI arguments
  runParseOptions(argc, argv, modeReg);

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

  // Set up backend based on given args
  if ((modeReg & FM_OP_BE_TEXT) == FM_OP_BE_TEXT)
    beptr = new film::TextBackend;

  // If interactive mode is set, run ncurses form interface
  if ((modeReg & FM_OP_INTERACTIVE) == FM_OP_INTERACTIVE) {
    if (runInteractive(fd, labels) != 0) {
      std::cerr << "Interactive failed\n";
      return 1;
    }
  }

  // Send data to backend
  assert(beptr);

  if (processFields(*beptr, fd) != 0) {
    std::cerr << "Failed to process fields\n";
    return 1;
  }

  // Free memory
  delete beptr;

  return 0;
}
