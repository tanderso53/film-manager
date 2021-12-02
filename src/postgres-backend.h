//===-- postgres-backend.h - postgres database client -* C++ *->-===//
//
// Part of film-manager project, Copyright 2021 Tyler J. Anderson
//
// This software is released under the BSD 3-Clause "New" or "Revised"
// License. You should have received a copy of the license with this
// source distribution
// 
// SPDX-License-Identifier: BSD-3-Clause
//
//===------------------------------------------------------------===//
///
/// \file
/// Backend functions for Postgresql client to film-management
/// database
///
//===------------------------------------------------------------===//

#ifndef POSTGRES_BACKEND_H
#define POSTGRES_BACKEND_H

#include "backend.h"

#include <libpq-fe.h>
#include <string>

namespace film {
  class BackendPostgres : public Backend {
  public:
    virtual void send(std::vector<const char*>* v...) override;
    virtual const char* receive(const char* query) override;
    virtual void connect() override;
    virtual void disconnect();
    virtual void init() override;
    void setTable(const std::string& table) {_table = table;};
    const char* getTable() {return _table.c_str();};
    BackendPostgres(const std::string& constring);
    BackendPostgres(const std::string& constring,
		    const std::string& table);
    ~BackendPostgres();
  private:
    std::string _constring;
    std::string _table;
    PGconn* _conn;
    PGresult* _res;
    void _pqkill();
    friend void postgres_begin(BackendPostgres& bp);
    friend void postgres_end(BackendPostgres& bp);
  };

  void postgres_begin(BackendPostgres& bp);
  void postgres_end(BackendPostgres& bp);
}

#endif // #ifndef POSTGRES_BACKEND_H
