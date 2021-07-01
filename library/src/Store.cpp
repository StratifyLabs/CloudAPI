#include <fs.hpp>
#include <inet.hpp>
#include <json.hpp>
#include <var.hpp>

#include "cloud/Store.hpp"

using namespace cloud;

Store::Store(const Cloud &cloud, const var::StringView database_project)
  : Cloud::SecureClient(cloud, database_project) {}

var::KeyString Store::create_document(
  const var::StringView path,
  const json::JsonObject &object,
  const var::StringView id) {
  const String path_arguments
    = path + (id.is_empty() == false ? String("?documentId=") + id : String());

  const auto url = get_document_url_path(path_arguments);
  refresh_http_client_document_headers();

  const String response = execute_method(
    Http::Method::post,
    url,
    JsonDocument()
      .set_flags(JsonDocument::Flags::compact)
      .to_string(CloudMap::from_json(object)));

  return KeyString(fs::Path::name(
    JsonDocument().from_string(response).to_object().at("name").to_cstring()));
}

Store &Store::patch_document(
  var::StringView path,
  const json::JsonObject &object,
  IsExisting is_existing) {

  const String mask_field_arguments = create_mask_fields();
  const String path_arguments
    = path + "?currentDocument.exists="
      + (is_existing == IsExisting::yes ? "true" : "false")
      + (mask_field_arguments.is_empty() == false ? String("&") + mask_field_arguments : String());

  String result;
  int count = 0;
  CloudMap cloud_map = CloudMap::from_json(object);

  do {

    const var::String url = "/" + document_api_path() + "/" + path_arguments;
    refresh_http_client_document_headers();

    const String request = JsonDocument()
                             .set_flags(JsonDocument::Flags::compact)
                             .to_string(cloud_map);

    result = execute_method(Http::Method::patch, url, request);

    count++;
  } while (result.is_empty() && count < 1);

  return *this;
}

json::JsonObject Store::get_document(var::StringView path) {
  const auto url = get_document_url_path(path);
  refresh_http_client_document_headers();
  return CloudMap(execute_get_json(url).to_object()).to_json();
}

Store &Store::remove_document(var::StringView path) {
  const auto url = get_document_url_path(path);
  refresh_http_client_document_headers();
  execute_method(Http::Method::delete_, url, StringView());
  return *this;
}

json::JsonObject
Store::list_documents(var::StringView path, var::StringView mask_options) {
  const auto url
    = get_document_url_path(path)
      & (mask_options.is_empty() == false ? String() : String("?") + mask_options);
  refresh_http_client_document_headers();
  return execute_get_json(url).to_object();
}

var::String Store::create_mask_fields() {
  String result;
  for (const auto &field : document_mask_fields()) {
    if (result.is_empty() == false) {
      result += "&";
    }
    result += var::StringView("mask.fieldPaths=") + field;
  }

  for (const auto &field : document_update_mask_fields()) {
    if (result.is_empty() == false) {
      result += "&";
    }
    result += var::StringView("updateMask.fieldPaths=") + field;
  }
  // now clear the masks once they have been used
  document_update_mask_fields().clear();
  document_mask_fields().clear();
  return result;
}
