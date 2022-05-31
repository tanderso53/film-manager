//===-- postgres-backend.cpp - postgres database client ---------===//
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
/// Implementation of Postgresql client for film-management data
/// sending
///
//===------------------------------------------------------------===//

#include "postgres-backend.h"

#include <vector>
#include <sstream>
#include <cstdarg>
#include <assert.h>

extern "C";

void film::BackendPostgres::connect()
{
  assert(!_conn);

  _conn = PQconnectdb(_constring.c_str());

  if (PQstatus(_conn) != CONNECTION_OK) {
    std::string pgerror = PQerrorMessage(_conn);
    std::string reerror = "In function film::BackendPostgres::connect():";
    reerror += pgerror;
    _pqkill();
    throw std::runtime_error(reerror);
  }

  _res = PQexec(_conn,
		"SELECT pg_catalog.set_config('search_path', '', false)");

  if (PQresultStatus(_res) != PGRES_TUPLES_OK) {
    PQclear(_res);
    _pqkill();
    std::runtime_error("Failed to configure Postgres connection");
  }

  PQclear(_res);
}

void film::BackendPostgres::disconnect()
{
  PQfinish(_conn);
}

void film::BackendPostgres::send(std::vector<const char*>* v...)
{
  std::stringstream query;
  std::va_list args;
  assert(_conn);

  // Begin transaction
  film::postgres_begin(*this);

  // Append data to table
  va_start(args, v);
  std::vector<const char*>* varg = va_arg(args,
					  std::vector<const char*>*);

  assert(varg);

  // Build query
  std::string fieldlist;
  std::string valuelist;
  assert (!this->_table.empty());
  query << "INSERT INTO " << this->_table << " as t1 ";

  for (unsigned int i = 0; i < v->size(); ++i) {
    fieldlist += v->at(i);

    if (i < v->size() - 1) {
      fieldlist += ", ";
    }

    valuelist += varg->at(i);

    if (i < v->size() - 1) {
      valuelist += ", ";
    }
  }

  query << "(" << fieldlist << ") "
	<< "VALUES (" << valuelist << ")";

  // Execute query on server
  _res = PQexec(_conn, query.str().c_str());

  if (PQresultStatus(_res) != PGRES_COMMAND_OK) {
    std::stringstream pqerror;

    pqerror << "Query " << query.str() << " failed with error "
	    << PQerrorMessage(_conn) << '\n';

    PQclear(_res);
    _pqkill();
    std::runtime_error(pqerror.str());
  }

  // Complete connection
  postgres_end(*this);
}

/// Note: Receive is not thread safe
const char* film::BackendPostgres::receive(const char* query)
{
  std::vector<std::string> fields;
  static std::string result;

  assert(_conn);

  postgres_begin(*this);

  // Run requested select query
  _res = PQexec(_conn, query);

  if (PQresultStatus(_res) != PGRES_TUPLES_OK) {
    std::stringstream pgerror;
    pgerror << "Postgres query \"" << query
	    << "\" failed with error " << PQerrorMessage(_conn)
	    << '\n';
    _pqkill();
    throw std::runtime_error(pgerror.str());
  }

  // Get num fields
  for (int i = 0; i < PQnfields(_res); ++i) {
    fields.push_back(PQfname(_res, i));
  }

  assert((unsigned long) PQnfields(_res) == fields.size());

  result = "{";

  for (unsigned int i = 0; i < fields.size(); ++i) {
    result += "\"";
    result += fields[i];
    result += "\": {[";

    for (int j = 0; j < PQntuples(_res); ++j) {
      result += "\"";
      result += std::string(PQgetvalue(_res, j, i));
      result += "\"";

      if (j < PQntuples(_res) -1) {
	result += ", ";
      }
    }

    result += "]}";

    if (i < fields.size() - 1) {
      result += ", ";
    }
  }

  result += "}";

  return (const char*) result.c_str();
}

film::BackendPostgres::BackendPostgres(const std::string& constring)
  : _constring(constring), _conn(nullptr), _res(nullptr)
{
  this->flags = this->flags | FM_BE_RECEIVE_ENABLED;
  init();
}

film::BackendPostgres::BackendPostgres(const std::string& constring,
				       const std::string& table)
  : _constring(constring), _table(table), _conn(nullptr), _res(nullptr)
{
  this->flags = this->flags | FM_BE_RECEIVE_ENABLED;
  init();
}

void film::BackendPostgres::init()
{
}

void film::BackendPostgres::_pqkill()
{
  if (_conn)
    PQfinish(_conn);
}

film::BackendPostgres::~BackendPostgres()
{
  _pqkill();
}

void film::postgres_begin(BackendPostgres& bp)
{
  // Begin transaction block
  assert(!bp._res);
  bp._res = PQexec(bp._conn, "BEGIN");

  if (PQresultStatus(bp._res) != PGRES_COMMAND_OK) {
    std::string pgerror = "In film::BackendPostgres::send: ";
    pgerror += PQerrorMessage(bp._conn);
    PQclear(bp._res);
    bp._pqkill();
    std::runtime_error(pgerror.c_str());
  }

  PQclear(bp._res);
}

void film::postgres_end(BackendPostgres& bp)
{
  bp._res = PQexec(bp._conn, "END");
  PQclear(bp._res);
}
