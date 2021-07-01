#ifndef CLOUDAPI_CLOUD_STORAGE_HPP
#define CLOUDAPI_CLOUD_STORAGE_HPP

#include "Cloud.hpp"

namespace cloud {

class Storage : public Cloud::SecureClient {
public:
  Storage(const Cloud & cloud, var::StringView database_project);

  json::JsonObject get_details(var::StringView path);
  Storage &
  get_object(var::StringView path, const fs::FileObject &destination);

  Storage &create_object(
    var::StringView destination,
    const fs::FileObject &source,
    var::StringView count_description = var::StringView());

  Storage& remove_object(var::StringView path);


private:

  static const var::StringView storage_host() { return "www.googleapis.com"; }

  var::PathString storage_bucket() { return database_project() & ".appspot.com"; }
  var::PathString get_storage_bucket_path() {
    return "/storage/v1/b" / storage_bucket() / "o";
  }

  var::PathString get_storage_path(const var::StringView path) {
    return get_storage_bucket_path() / inet::Url::encode(path);
  }

};
}

#endif // CLOUDAPI_CLOUD_STORAGE_HPP
