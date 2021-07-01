#ifndef CLOUDAPI_CLOUD_STORE_HPP
#define CLOUDAPI_CLOUD_STORE_HPP

#include "Cloud.hpp"

namespace cloud {

class Store : public Cloud::SecureClient {
public:
  using IsExisting = Cloud::IsExisting;
  Store(const Cloud & cloud, const var::StringView database_project);

  Store& set_project_id(const var::StringView project){
    interface_set_project_id(project);
    return *this;
  }

  // Cloud Firestore operations
  var::KeyString create_document(
    const var::StringView path,
    const json::JsonObject &object,
    const var::StringView id = var::StringView(""));

  Store &patch_document(
    var::StringView path,
    const json::JsonObject &object,
    IsExisting is_existing = IsExisting::yes);

  json::JsonObject get_document(var::StringView path);

  Store &remove_document(var::StringView path);

  json::JsonObject
  list_documents(var::StringView path, var::StringView mask_options);

  const var::StringList &document_mask_fields() const {
    return m_document_mask_fields;
  }
  var::StringList &document_mask_fields() { return m_document_mask_fields; }

  const var::StringList &document_update_mask_fields() const {
    return m_document_update_mask_fields;
  }
  var::StringList &document_update_mask_fields() {
    return m_document_update_mask_fields;
  }

private:
  var::StringList m_document_mask_fields;
  var::StringList m_document_update_mask_fields;

  static constexpr auto m_document_host = "firestore.googleapis.com";

  var::String create_mask_fields();

  var::PathString document_api_path() {
    return "v1/projects" / database_project() / "databases/(default)/documents";
  }

  var::PathString get_document_url_path(var::StringView path) {
    return "/" & document_api_path() / path;
  }

  void refresh_http_client_document_headers() {

    http_client().add_header_field(
      "Content-Type",
      "application/json");

    if (credentials().get_token().is_empty() == false) {
      http_client().add_header_field(
        "Authorization",
        var::String("Bearer ") + credentials().get_token());
    }

    if (http_client().is_connected() == false) {
      http_client().connect(m_document_host);
    }
  }
};

} // namespace cloud

#endif // CLOUDAPI_CLOUD_STORE_HPP
