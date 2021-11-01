#ifndef CLOUDAPI_CLOUD_DATABASE_HPP
#define CLOUDAPI_CLOUD_DATABASE_HPP

#include "Cloud.hpp"

namespace cloud {

class Database : public Cloud::SecureClient {
public:
  using IsRequestShallow = Cloud::IsRequestShallow;
  Database(const Cloud & cloud, var::StringView database_project);

  Database& set_project_id(const var::StringView project){
    interface_set_project_id(project);
    return *this;
  }

  json::JsonValue get_value(
    var::StringView path,
    IsRequestShallow is_shallow = IsRequestShallow::no);

  Database &get_value(
    var::StringView path,
    const fs::FileObject &dest,
    IsRequestShallow is_shallow = IsRequestShallow::no);

  Database &remove_object(var::StringView path);

  var::KeyString create_object(
    var::StringView path,
    const json::JsonObject &object,
    var::StringView id = var::StringView());

  Database &patch_object(var::StringView path, const json::JsonObject &object);

  // realtime database operations
  Database &listen(
    var::StringView path,
    const fs::FileObject &destination,
    thread::Mutex *lock_on_receive = nullptr);

private:
  var::PathString database_host() {
    return database_project() & ".firebaseio.com";
  }

  var::String get_database_url_path(const var::StringView path) {
    return "/" + path + ".json" +
           (!credentials().get_token().is_empty() ?
                                                          ("?auth=" + credentials().get_token()) : var::String());
  }

  void refresh_http_client_database_headers() {
    http_client().add_header_field("Content-Type", "application/json");
    if (!http_client().is_connected()) {
      http_client().connect(database_host());
    }
  }
};

} // namespace cloud

#endif // CLOUDAPI_CLOUD_DATABASE_HPP
