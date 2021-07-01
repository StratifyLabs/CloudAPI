// Copyright 2016-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md

#ifndef DATABASEOBJECT_HPP
#define DATABASEOBJECT_HPP

#include <json/Json.hpp>
#include <printer/Printer.hpp>
#include <var/String.hpp>

#include "Cloud.hpp"

namespace cloud {

class DatabaseObject : public CloudAccess, public json::JsonObject {
public:
  DatabaseObject();
  explicit DatabaseObject(Cloud &cloud) { set_cloud(cloud); }

  // download from cloud
  DatabaseObject &download(var::StringView path, var::StringView id);

  // upload to cloud
  var::String upload(
    var::StringView path,
    var::StringView id = "",
    IsCreate is_create = IsCreate::no);

  json::JsonArray list(var::StringView path, var::StringView mask);

  // save to fs as a json file
  int save(var::StringView path);

  // load a json file from fs
  int load(var::StringView path);

  bool is_valid() const;

  JSON_ACCESS_STRING_WITH_KEY(DatabaseObject, documentId, document_id);
  JSON_ACCESS_INTEGER(DatabaseObject, timestamp);

  int import_binary_file_to_base64(var::StringView path, var::StringView key);
  int export_base64_to_binary_file(var::StringView path, var::StringView key);

protected:
  var::String get_key_as_string(var::StringView key);

private:
  API_ACCESS_COMPOUND(DatabaseObject, var::String, path);

  static int import_json_recursive(
    const json::JsonObject &input,
    json::JsonObject &output);
  static int export_json_recursive(
    const json::JsonObject &input,
    json::JsonObject &output);
  static json::JsonObject
  convert_map_to_object(const json::JsonObject &input_map, var::StringView key);
  static json::JsonValue convert_field_to_value(
    const json::JsonObject &input_map,
    var::StringView key);
};

} // namespace cloud

#endif // DATABASEOBJECT_HPP
