//===-- backend.h - Backend Connection Header --------* C++ *----===//
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
/// This file contains the header definitions describing the backends
/// available to send data through, including the text console, files,
/// and database systems.
///
//===------------------------------------------------------------===//

#ifndef BACKEND_H
#define BACKEND_H

#include <iostream>
#include <vector>

namespace film {
  /// Abstract class to describe backends that accept data
  class Backend {
  public:
    virtual void send(std::vector<const char*>* v...) = 0;
    virtual void connect() = 0;
    virtual void init() = 0;
  };

  class TextBackend :public Backend {
  public:
    virtual void send(std::vector<const char*>* v...) override;
    virtual void connect() override;
    virtual void init() override;
    TextBackend(std::ostream& outstream = std::cout);

  private:
    std::ostream& _outstream;
  };
}


#endif // #ifndef BACKEND_H
