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
#include "postgres-backend.h"

#include <json/json.h>
#include <vector>
#include <string>
#include <cstring>
#include <utility>
#include <iostream>
#include <assert.h>
#include <getopt.h>
#include <stdio.h>

// Bring in variables from getopt library
extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

// Program operating modes
#define FM_OP_USAGE		0x01
#define FM_OP_VERSION		0x02
#define FM_OP_INTERACTIVE	0x04
#define FM_OP_BE_TEXT		0x08
#define FM_OP_BE_POSTGRES	0x10

/// Class to store application global variables and methods
class App {
public:
  typedef std::vector<std::vector<const char*>> acvector;
  acvector aclist;
  std::string acfile;
  std::string connString;
  std::string dbTable;
} app;

/// Generate autocomplete information
App::acvector autoCompleteLists(film::Backend& _be,
				uint8_t& _modereg,
				const std::vector<const char*>& _labels)
{
  std::vector<std::vector<const char*>> _aclist(_labels.size());

  if ((_modereg & FM_OP_BE_TEXT) == FM_OP_BE_TEXT) {
    if (!app.acfile.empty()) {
      try {
	_be.receive(app.acfile.c_str());
      }
      catch (std::exception& e) {
	std::cerr << "Failed to receive autocomplete info with error "
		  << e.what();
	exit(1);
      }

      _aclist.assign(_labels.size(), _be.results());
    }
  }
  else if ((_modereg & FM_OP_BE_POSTGRES) == FM_OP_BE_POSTGRES) {
    try {
      _be.connect();

      for (unsigned int i = 0; i < _labels.size(); ++i) {
	char query[1024];

	// Build select query for this field
	snprintf(query, 1024, "SELECT DISTINCT \"%s\" FROM \"%s\""
		 "ORDER BY \"%s\"",
		 _labels[i], app.dbTable.c_str(),
		 _labels[i]);

	assert(query);

	// Get and parse list of existing values from database
	Json::Value resultlist = _be.receive(query);

	assert(resultlist.isObject());
	assert(resultlist[_labels[i]].isArray());
	
	Json::Value& datalist = resultlist[_labels[i]];

	// Add values to back of vector for this field
	for (unsigned int j = 0; j < datalist.size(); ++j) {
	  _aclist[i].push_back(datalist[j].asCString());
	}
      }
    }
    catch (std::exception& e) {
	std::cerr << "Failed to receive autocomplete info with error "
		  << e.what();
	exit(1);
    }
  }

  return _aclist;
}

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
      .data = "",
      .aclist = NULL,
      .naclist = 0
    };

    if (!app.aclist.empty()) {
      fd.aclist = (char* const*) &app.aclist.at(i)[0];
      fd.naclist = app.aclist[i].size();
    }

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

/// @brief Get list of labels from the database
///
/// This all depends on the datamodel of the database. The db
/// datamodel should support the following for this function to work:
///
/// - A schema is defined for basic data components named 'film_data'
/// - The labels are contained in a table called 'labels'
/// - The name of the label is in a column called 'label_name'
/// - The field 'loid' determines the order the label should be
///   displayed
///
/// @returns std::vector of field labels
std::vector<std::string> getLabelsFromDB(film::Backend& be)
{
  std::vector<std::string> ret;
  std::string q;
  Json::Value rsp;

  q =
    "SELECT label_name "
    "FROM film_data.labels "
    "ORDER BY loid";

  rsp = be.receive(q.c_str());

  for (Json::Value &el : rsp["label_name"]) {
    ret.push_back(el.asString());
  }

  if (!ret.size()) {
    std::string estr =
      "getLabelsFromDB(): No results from query, invalid label list";

    throw std::runtime_error(estr);
  }

  return ret;
}

int printUsage(int _argc, const char** _argv,
	       std::ostream& out = std::cout)
{
  assert(_argc > 0);

  out << "Usage:\n"
      << _argv[0] << " [ -i | --interactive ] [ -b | --backend name ]\n"
      << _argv[0] << " -h | --help\n"
      << _argv[0] << " -V | --version \n"
      << '\n'
      << "Options:\n"
      << "-i | --interactive\t\t\tRun in interactive mode\n"
      << "\t\t\t\t\t(Default)\n"
      << "-b | --backend name\t\t\tName of data backend to\n"
      << "\t\t\t\t\tuse (Default text)\n"
      << "-a | --auto-complete-file filename\tFile to look\n"
      << "\t\t\t\t\tfor auto-complete list\n"
      << "-P | --connection-string string\t\tDatabase string\n"
      << "-t | --database-table name\t\tName of data table\n"
      << "-V | --version\t\t\t\tPrint version information\n"
      << "\t\t\t\t\tand exit\n"
      << "-h | --help\t\t\t\tPrint this help message\n"
      << '\n'
      << "Available Backend Handlers:\n"
      << "text\n"
      << "postgres\n";

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

int runParseOptions(int _argc, const char** _argv, uint8_t& _modereg)
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
      .name = "auto-complete-file",
      .has_arg = required_argument,
      .flag = NULL,
      .val = 'a'
    },

    {
      .name = "connection-string",
      .has_arg = required_argument,
      .flag = NULL,
      .val = 'S'
    },

    {
      .name = "database-table",
      .has_arg = required_argument,
      .flag = NULL,
      .val = 't'
    },

    {
      .name = NULL,
      .has_arg = 0,
      .flag = NULL,
      .val = 0
    }
  };

  int ch;

  while ((ch = getopt_long(_argc, (char * const *) _argv,
			   "hVib:a:S:t:", lopts, NULL)) != -1) {
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
      else if (strcmp(optarg, "postgres") == 0) {
	_modereg = _modereg | FM_OP_BE_POSTGRES;
	break;
      }

      std::cerr << "Backend handler " << optarg << " not found\n\n"
		<< "Available backend handlers:\n"
		<< "text\n"
		<< "postgres\n";

      exit(1);

    case 'a':
      assert(optarg);
      app.acfile = optarg;
      break;

    case 'S':
      assert(optarg);
      app.connString = optarg;

    case 't':
      assert(optarg);
      app.dbTable = optarg;

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
  uint8_t modeReg = 0; // Registor to report active option flags
  film::Backend* beptr = nullptr; // Ptr to set to desired backend
  std::vector<const char*> labels; // Labels describing form fields
  std::vector<formdata> fd;

  // Set program defaults
  modeReg = modeReg | FM_OP_INTERACTIVE | FM_OP_BE_TEXT;

  // Parse CLI arguments
  runParseOptions(argc, argv, modeReg);

  // Keep the testing labels around, but can get from postgres if the
  // option is supplied
  if (modeReg == FM_OP_BE_POSTGRES) {
    std::vector<std::string> bestr = getLabelsFromDB(*beptr);

    for (std::string &el : bestr) {
      labels.push_back(el.c_str());
    }
  } else {
    // Temporary definitions for testing
    labels = {
      {
	"Location",
	"Subject",
	"Date/time",
	"Camera",
	"Film Type",
	"Film Set",
	"ID Number",
	"Camera Serial",
	"Lens Name",
	"Lens Serial",
	"F number",
	"Focal Length"
      }
    };
  }

  // Set up backend based on given args
  if ((modeReg & FM_OP_BE_TEXT) == FM_OP_BE_TEXT)
    beptr = new film::TextBackend;
  else if ((modeReg & FM_OP_BE_POSTGRES) == FM_OP_BE_POSTGRES) {
    assert(!app.connString.empty());
    assert(!app.dbTable.empty());

    beptr = new film::BackendPostgres(app.connString, app.dbTable);
  }

  // Load autocomplete lists
  assert(beptr);
  app.aclist = autoCompleteLists(*beptr, modeReg, labels);

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
