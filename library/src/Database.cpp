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
    thread::Mutex *lock_on_receive;
    const fs::FileObject *file;
  };

  ListenContext listen_context;
  listen_context.file = &destination;
  listen_context.lock_on_receive = lock_on_receive;

  LambdaFile response_file
    = LambdaFile()
        .set_context(&listen_context)
        .set_write_callback(
          [](void *context, int location, const var::View view) -> int {
            MCU_UNUSED_ARGUMENT(location);
            auto *listen_context = reinterpret_cast<ListenContext *>(context);

            listen_context->incoming
              += String(View(view).to_const_char(), return_value());

            size_t end = StringView(listen_context->incoming).find("\n\n");

            if (end != StringView::npos) {

              StringViewList line_list = listen_context->incoming.split("\n");

              if (line_list.count() > 1) {
                const auto data_colon_pos = line_list.at(1).find(":");
                if (data_colon_pos != String::npos) {
                  StringView json_string
                    = line_list.at(1).get_substring_at_position(
                      data_colon_pos + 1);
                  {
                    thread::Mutex::Scope m_scope(
                      listen_context->lock_on_receive);
                    int result
                      = listen_context->file->write(json_string).return_value();

                    if (result < 0) {
                      return result;
                    }
                  }
                }
                listen_context->incoming.clear();
              }
            }

            return view.size();
          })
        .move();

  http_client.add_header_field("Accept", "text/event-stream")
    .connect(database_host())
    .get(url, HttpClient::Get().set_response(&response_file));

  assign_error_from_status();

  return *this;
}

Database &Database::get_value(
  var::StringView path,
  const fs::FileObject &dest,
  IsRequestShallow is_shallow) {

  const bool is_shallow_bool = (is_shallow == IsRequestShallow::yes);
  const var::String url = "/" + path + ".json" +
                          (is_shallow_bool ? String("?shallow=true") : String())
                          + (credentials().get_token().is_empty() == false ? ((is_shallow_bool ? String("&") : String("?")) + "auth=" + credentials().get_token()) : String());

  printf("get url %s\n", url.cstring());
  {
    thread::Mutex::Scope m_scope(mutex());

    refresh_http_client_database_headers();

    http_client().get(url, HttpClient::Get().set_response(&dest));
    assign_error_from_status();
  }

  return *this;
}

json::JsonValue
Database::get_value(var::StringView path, IsRequestShallow is_shallow) {

  fs::DataFile response;

  get_value(path, response, is_shallow);

  if (response.size() == 0) {
    return JsonValue();
  }

  return JsonDocument()
    .set_flags(JsonDocument::Flags::decode_any)
    .load(response.seek(0));
}

var::KeyString Database::create_object(
  var::StringView path,
  const json::JsonObject &object,
  var::StringView id) {

  const auto url = id.is_empty() == false ? get_database_url_path(path / id)
                                          : get_database_url_path(path);

  {
    thread::Mutex::Scope m_scope(mutex());
    refresh_http_client_database_headers();
  }

  const Http::Method method
    = id.is_empty() ? Http::Method::post : Http::Method::put;

  JsonValue response = execute_method(method, url, object);

  return id.is_empty()
           ? KeyString(response.to_object().at("name").to_string_view())
           : KeyString(id);
}

Database &
Database::patch_object(var::StringView path, const json::JsonObject &object) {

  const auto url = get_database_url_path(path);

  {
    thread::Mutex::Scope m_scope(mutex());
    refresh_http_client_database_headers();
  }

  execute_method(Http::Method::patch, url, object);

  return *this;
}

Database &Database::remove_object(var::StringView path) {
  const auto url = get_database_url_path(path);

  {
    thread::Mutex::Scope m_scope(mutex());
    refresh_http_client_database_headers();

    fs::DataFile response_file(fs::OpenMode::append_write_only());
    http_client().remove(
      url,
      HttpClient::Remove().set_response(&response_file));
    assign_error_from_status();
  }

  return *this;
}
