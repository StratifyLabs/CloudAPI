// Copyright 2016-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md

#include <chrono.hpp>
#include <var.hpp>

#include "cloud/DatabaseObject.hpp"

using namespace cloud;

DatabaseObject::DatabaseObject() {}

var::String DatabaseObject::upload(
  var::StringView path,
  var::StringView id,
  IsCreate is_create) {

  var::String result;

  set_timestamp(DateTime::get_system_time().ctime());

  if (id == "" || (is_create == IsCreate::yes)) {
    // if id.is_empty() then we add a new document to path otherwise modify a
    // document
    set_document_id(
      cloud().create_object(path, to_object(), id).cstring());
  } else {
    // once document is uploaded it should be modified to include the id
    set_document_id(var::StackString64(id).cstring());
    cloud().patch_database_object(path + "/" + id, to_object());
    API_RETURN_VALUE_IF_ERROR(String());

    result = String(id);
  }

  return result;
}

DatabaseObject &
DatabaseObject::download(var::StringView path, var::StringView id) {
  // path is cloud::host()/path/id.json
  // add id if it doesn't exist
  const var::String id_path
    = path + (id.is_empty() ? String() : String("/") + id);

  to_object() = cloud().get_database_value(id_path);

  if (is_success()) {
    set_document_id(var::StackString64(id).cstring());
  }

  return *this;
}
