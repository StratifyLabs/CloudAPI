#include <fs.hpp>
#include <inet.hpp>
#include <json.hpp>
#include <var.hpp>

#include "cloud/Database.hpp"

using namespace cloud;

Database::Database(const Cloud &cloud, const var::StringView database_project)
  : Cloud::SecureClient(cloud, database_project) {}

Database &Database::listen(
  const var::StringView path,
  const fs::FileObject &destination,
  thread::Mutex *lock_on_receive) {

  HttpSecureClient http_client;
  const String url = "/" + path + ".json" + (credentials().get_token().is_empty() == false ? (String("?auth=") + credentials().get_token()) : String());

  struct ListenContext {
    var::String incoming;
    thread::Mutex *lock_on_receive = nullptr;
    const fs::FileObject *file = nullptr;
  };

  ListenContext listen_context;
  listen_context.file = &destination;
  listen_context.lock_on_receive = lock_on_receive;

  auto response_file = LambdaFile().set_write_callback(
    [&](int location, const var::View view) -> int {
      MCU_UNUSED_ARGUMENT(location);

      auto incoming = String{};
      incoming += String(View(view).to_const_char(), return_value());

      size_t end = StringView(incoming).find("\n\n");

      if (end != StringView::npos) {

        StringViewList line_list = incoming.split("\n");

        if (line_list.count() > 1) {
          const auto data_colon_pos = line_list.at(1).find(":");
          if (data_colon_pos != String::npos) {
            const auto json_string
              = line_list.at(1).get_substring_at_position(data_colon_pos + 1);
            {
              thread::Mutex::Scope m_scope(lock_on_receive);
              const auto result = destination.write(json_string).return_value();
              if (result < 0) {
                return result;
              }
            }
          }
          incoming.clear();
        }
      }

      return int(view.size());
    });

  auto response = Http::MethodResponse(std::move(response_file));
  http_client.add_header_field("Accept", "text/event-stream")
    .connect(database_host())
    .get(url, response);

  assign_error_from_status();

  return *this;
}

Database &Database::get_value(
  var::StringView path,
  const fs::FileObject &dest,
  IsRequestShallow is_shallow) {

  const bool is_shallow_bool = (is_shallow == IsRequestShallow::yes);
  const auto url = "/" + path + ".json" +
                          (is_shallow_bool ? String("?shallow=true") : String())
                          + (credentials().get_token().is_empty() == false ? ((is_shallow_bool ? String("&") : String("?")) + "auth=" + credentials().get_token()) : String());
  thread::Mutex::Scope(mutex(), [&]() {
    refresh_http_client_database_headers();
    http_client().get(url, HttpClient::Get().set_response(&dest));
    assign_error_from_status();
  });
  return *this;
}

json::JsonValue
Database::get_value(var::StringView path, IsRequestShallow is_shallow) {

  fs::DataFile response;

  get_value(path, response, is_shallow);

  if (response.size() == 0) {
    return {};
  }

  return JsonDocument()
    .set_flags(JsonDocument::Flags::decode_any)
    .load(response.seek(0));
}

var::KeyString Database::create_object(
  var::StringView path,
  const json::JsonObject &object,
  var::StringView id) {
  const auto url = !id.is_empty() ? get_database_url_path(path / id)
                                  : get_database_url_path(path);
  thread::Mutex::Scope(mutex(), [&]() {
    refresh_http_client_database_headers();
  });
  const auto method = id.is_empty() ? Http::Method::post : Http::Method::put;
  const auto response = execute_method(method, url, object);
  return id.is_empty()
           ? KeyString(response.to_object().at("name").to_string_view())
           : KeyString(id);
}

Database &
Database::patch_object(var::StringView path, const json::JsonObject &object) {
  const auto url = get_database_url_path(path);
  thread::Mutex::Scope(mutex(), [&]() {
    refresh_http_client_database_headers();
  });
  execute_method(Http::Method::patch, url, object);
  return *this;
}

Database &Database::remove_object(var::StringView path) {
  const auto url = get_database_url_path(path);
  thread::Mutex::Scope(mutex(), [&]() {
    refresh_http_client_database_headers();
    auto response
      = Http::MethodResponse(fs::NullFile());
    http_client().remove(url, response);
    assign_error_from_status();
  });
  return *this;
}
