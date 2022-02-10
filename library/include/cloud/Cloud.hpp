// Copyright 2016-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md

#ifndef CLOUD_API_CLOUD_CLOUD_HPP
#define CLOUD_API_CLOUD_CLOUD_HPP

#include <chrono/DateTime.hpp>
#include <inet/Http.hpp>
#include <inet/SecureSocket.hpp>
#include <inet/Url.hpp>
#include <json/Json.hpp>
#include <thread/Mutex.hpp>
#include <var/String.hpp>
#include <var/Vector.hpp>

#include "CloudObject.hpp"

namespace cloud {

class CloudMap : public json::JsonObject {
public:
  // constructs an existing cloud map
  explicit CloudMap(const json::JsonObject &object) : json::JsonObject(object) {}

  // imports regular old JSON to the cloud map
  static CloudMap from_json(const json::JsonObject &input);
  // exports to regular old JSON
  json::JsonObject to_json();

private:
  static int import_json_recursive(
    json::JsonObject *object,
    json::JsonArray *array,
    const json::JsonValue &input_json);

  static int import_value(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key,
    const json::JsonValue &value);

  static int import_string(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key,
    const json::JsonValue &value);

  static int import_integer(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key,
    const json::JsonValue &value);

  static int import_real(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key,
    const json::JsonValue &value);

  static int import_boolean(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key,
    const json::JsonValue &value);

  static int import_null(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key);

  static int import_array(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key,
    const json::JsonArray &map_array);

  static int import_map(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key,
    const json::JsonObject &map);

  static int export_json_recursive(
    json::JsonObject *object,
    json::JsonArray *array,
    const json::JsonObject &input_json);
  static int export_value(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key,
    const json::JsonObject &value);

  static int export_string(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key,
    const json::JsonObject &value);

  static int export_integer(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key,
    const json::JsonObject &value);

  static int export_real(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key,
    const json::JsonObject &value);

  static int export_boolean(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key,
    const json::JsonObject &value);

  static int export_null(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key,
    const json::JsonObject &value);

  static int export_array(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key,
    const json::JsonObject &value);

  static int export_map(
    json::JsonObject *object,
    json::JsonArray *array,
    var::StringView key,
    const json::JsonObject &value);
};

class Cloud : public CloudObject {
public:
  enum class IsRequestShallow { no, yes };
  enum class IsExisting { no, yes };

  class Credentials : public json::JsonObject {
  public:
    Credentials() = default;

    explicit Credentials(const json::JsonObject &object)
      : json::JsonObject(object) {}
    JSON_ACCESS_STRING(Credentials, uid);
    JSON_ACCESS_STRING(Credentials, token);
    JSON_ACCESS_STRING_WITH_KEY(Credentials, refreshToken, refresh_token);
    JSON_ACCESS_INTEGER_WITH_KEY(
      Credentials,
      tokenTimestamp,
      token_timestamp_number);
    JSON_ACCESS_STRING_WITH_KEY(Credentials, sessionTicket, session_ticket);
    JSON_ACCESS_BOOL(Credentials, global);

    API_NO_DISCARD chrono::DateTime get_token_timestamp() const {
      return chrono::DateTime(get_token_timestamp_number());
    }

    Credentials &set_token_timestamp(const chrono::DateTime &t) {
      return set_token_timestamp_number(t.ctime());
    }

    bool is_expired() const {
      const auto age = get_token_timestamp().age();
      if( age.second() > 60 ){
        return true;
      }
      return false;
    }


  private:
  };

  class SecureClient : public CloudObject {
  public:

    SecureClient(const Cloud & cloud, const var::StringView database_project) : m_cloud(cloud), m_database_project(database_project){}

    inet::HttpSecureClient &client() { return m_client; }
    const inet::HttpSecureClient &client() const { return m_client; }
    const thread::Mutex &mutex() const { return m_mutex; }
    thread::Mutex &mutex() { return m_mutex; }
    void assign_error_from_status();

    const Credentials & credentials() const { return m_cloud.credentials(); }

    const inet::HttpSecureClient &http_client() const {
      return m_client;
    }

    inet::HttpSecureClient &http_client(){
      return m_client;
    }

    const var::String & traffic() const {
      return http_client().traffic();
    }

    json::JsonValue execute_method(
      inet::Http::Method method,
      var::StringView url,
      const json::JsonObject &request);

    var::String execute_method(
      inet::Http::Method method,
      var::StringView url,
      var::StringView request);

    json::JsonValue
    execute_get_json(var::StringView url);

    var::String
    execute_get_string(var::StringView url);

    var::StringView database_project() const {
      return m_database_project.string_view();
    }

  protected:

    void interface_set_project_id(const var::StringView a){
      m_database_project = a;
    }

  private:
    const Cloud & m_cloud;
    thread::Mutex m_mutex;
    inet::HttpSecureClient m_client;
    var::PathString m_database_project;
    API_ACCESS_STRING(SecureClient, error_string);

  };

  Cloud() = default;
  explicit Cloud(var::StringView api_key,
    u32 lifetime = 0);

  // needs a host to connect to
  // needs an authorization token
  // needs a token for each transaction

  API_NO_DISCARD bool is_logged_in() const;

  Cloud &login(var::StringView email, var::StringView password);
  Cloud &refresh_login();


private:
  API_ACCESS_FUNDAMENTAL(Cloud, u32, ticket_lifetime, 0);
  API_ACCESS_COMPOUND(Cloud, Cloud::Credentials, credentials);
  API_ACCESS_COMPOUND(Cloud, var::PathString, api_key);
  API_ACCESS_STRING(Cloud, traffic);

  API_NO_DISCARD static var::StringView identity_host() { return "www.googleapis.com"; }
  API_NO_DISCARD static var::StringView refresh_login_host() {
    return "securetoken.googleapis.com";
  }


};


} // namespace cloud

#endif // CLOUD_API_CLOUD_CLOUD_HPP
