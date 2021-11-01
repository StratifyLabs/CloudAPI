#include <fs.hpp>
#include <inet.hpp>
#include <json.hpp>
#include <var.hpp>

#include "cloud/Storage.hpp"

using namespace cloud;

Storage::Storage(const Cloud &cloud, const var::StringView database_project)
  : Cloud::SecureClient(cloud, database_project) {}

json::JsonObject Storage::get_details(var::StringView path) {
  const auto url = get_storage_path(path);

  if (!http_client().is_connected()) {
    thread::Mutex::Scope m_scope(mutex());
    http_client().connect(storage_host());
  }

  return execute_get_json(url).to_object();
}

Storage &
Storage::get_object(var::StringView path, const fs::FileObject &destination) {

  JsonObject details = get_details(path);
  API_RETURN_VALUE_IF_ERROR(*this);

  const StringView url = details.at("mediaLink").to_string_view();

  const StringView minimum = "https://..";
  if (url.length() < minimum.length()) {
    API_RETURN_VALUE_ASSIGN_ERROR(*this, "mediaLink for object is invalid", EBADMSG);
  }

  size_t start_pos = url.find("/", minimum.length());

  const StringView path_value = url.get_substring_at_position(start_pos);
  // strip the https://wwww.host.com

  // request the media link file
  printer().set_progress_key("downloading");

  {
    thread::Mutex::Scope m_scope(mutex());
    http_client().get(
      path_value,
      HttpClient::Get()
        .set_response(&destination)
        .set_progress_callback(printer().progress_callback()));
  }

  printer().set_progress_key("progress");

  return *this;
}

Storage &Storage::create_object(
  var::StringView destination,
  const fs::FileObject &source,
  var::StringView count_description) {

  const String url = "/upload/storage/v1/b/" + storage_bucket()
                     + "/o?uploadType=media&name=" + Url::encode(destination);

  http_client().add_header_field("Content-Type", "application/octet-stream");
  fs::DataFile response_file(fs::OpenMode::append_write_only());

  {
    thread::Mutex::Scope mg(mutex());

    if (!http_client().is_connected()) {
      http_client().connect(storage_host());
    }

    PathString progress_key = "uploading";
    if (!count_description.is_empty()) {
      progress_key.append("|").append(count_description);
    }
    printer().set_progress_key(progress_key.string_view());

    http_client().post(
      url,
      HttpClient::Post()
        .set_request(&source)
        .set_response(&response_file)
        .set_progress_callback(printer().progress_callback()));
  }
  assign_error_from_status();
  printer().set_progress_key("progress");

  return *this;
}

Storage &Storage::remove_object(var::StringView path) {
  MCU_UNUSED_ARGUMENT(path);
  return *this;
}
