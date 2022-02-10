// Copyright 2016-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md

#include <fs.hpp>
#include <inet.hpp>
#include <json.hpp>
#include <var.hpp>

#include "cloud/Cloud.hpp"

using namespace cloud;

Cloud::Cloud(const var::StringView api_key, u32 lifetime)
  : m_ticket_lifetime(lifetime), m_api_key(api_key) {}

void Cloud::SecureClient::assign_error_from_status() {
  API_RETURN_IF_ERROR();

  if (m_client.response().status() == Http::Status::ok) {
    return;
  }

  const auto error_string = Http::to_string(m_client.response().status());
  int error_number = EINVAL;
  if (m_client.response().status() == Http::Status::not_found) {
    error_number = ENOENT;
  } else if (m_client.response().status() == Http::Status::forbidden) {
    error_number = EPERM;
  }

  API_RETURN_ASSIGN_ERROR(error_string.cstring(), error_number);
}

bool Cloud::is_logged_in() const {
  if (
    credentials().get_uid() == "<invalid>"
    || credentials().get_uid().is_empty()) {
    return false;
  }
  if (
    credentials().get_token() == "<invalid>"
    || credentials().get_token().is_empty()) {
    return false;
  }
  return credentials().get_token_timestamp().age() < 1_hours;
}

var::String Cloud::SecureClient::execute_method(
  inet::Http::Method method,
  var::StringView url,
  var::StringView request) {

  fs::DataFile response_file(fs::OpenMode::append_write_only());
  auto request_file = fs::ViewFile(View(request));

  {
    thread::Mutex::Scope m_scope(mutex());
    http_client().execute_method(
      method,
      url,
      HttpClient::ExecuteMethod()
        .set_request(request.is_empty() ? nullptr : &request_file)
        .set_response(&response_file));
  }

  assign_error_from_status();

  return String(response_file.data());
}

json::JsonValue Cloud::SecureClient::execute_method(
  inet::Http::Method method,
  var::StringView path,
  const json::JsonObject &request) {

  String string_request = !request.is_empty()
                            ? JsonDocument()
                                .set_flags(JsonDocument::Flags::compact)
                                .to_string(request)
                            : String();

  String string_response = execute_method(method, path, string_request);
  return string_response.is_empty()
           ? JsonObject()
           : JsonDocument().from_string(string_response).to_object();
}

json::JsonValue Cloud::SecureClient::execute_get_json(var::StringView path) {

  return execute_method(Http::Method::get, path, JsonObject());
}

var::String Cloud::SecureClient::execute_get_string(var::StringView path) {
  return execute_method(Http::Method::get, path, StringView());
}

Cloud &Cloud::login(var::StringView email, var::StringView password) {
  const String path
    = "/identitytoolkit/v3/relyingparty/verifyPassword?key=" + api_key();

  SecureClient client(*this, "");
  client.client().connect(identity_host());
  API_RETURN_VALUE_IF_ERROR(*this);

  credentials().clear();
  API_RETURN_VALUE_IF_ERROR(*this);

  JsonObject response_object = client.execute_method(
    Http::Method::post,
    path,
    JsonObject()
      .insert("email", JsonString(String(email).cstring()))
      .insert("password", JsonString(String(password).cstring()))
      .insert("returnSecureToken", JsonTrue()));

  API_RETURN_VALUE_IF_ERROR(*this);

  m_traffic = client.traffic();

  if (is_error()) {
    return *this;
  }

  // these ids are defined by the cloud API
  credentials()
    .set_uid(response_object.at("localId").to_cstring())
    .set_token(response_object.at("idToken").to_cstring())
    .set_refresh_token(response_object.at("refreshToken").to_cstring())
    .set_token_timestamp(chrono::DateTime::get_system_time());
  return *this;
}

Cloud &Cloud::refresh_login() {

  const PathString path = "/v1/token?key=" & api_key();

  SecureClient client(*this, "");
  client.client().connect(refresh_login_host());

  JsonObject response_object = client.execute_method(
    Http::Method::post,
    path,
    JsonObject()
      .insert("grant_type", JsonString("refresh_token"))
      .insert(
        "refresh_token",
        JsonString(credentials().get_refresh_token_cstring())));
  m_traffic = client.traffic();

  // these ids are defined by the cloud API
  credentials()
    .set_token(response_object.at("id_token").to_cstring())
    .set_refresh_token(response_object.at("refresh_token").to_cstring())
    .set_token_timestamp(chrono::DateTime::get_system_time());

  return *this;
}

CloudMap CloudMap::from_json(const json::JsonObject &input) {
  CloudMap result = CloudMap(JsonObject());
  result.insert("fields", JsonObject());
  result.import_json_recursive(
    &result.at("fields").to_object(),
    nullptr,
    input);
  return result;
}

json::JsonObject CloudMap::to_json() {
  JsonObject result;
  export_json_recursive(&result, nullptr, to_object());
  return result;
}

int CloudMap::import_json_recursive(
  json::JsonObject *object,
  json::JsonArray *array,
  const json::JsonValue &input_json) {
  if (object) {
    JsonValue::KeyList keys = input_json.to_object().get_key_list();
    for (u32 i = 0; i < keys.count(); i++) {
      JsonValue value = input_json.to_object().at(keys.at(i));
      if (import_value(object, nullptr, keys.at(i), value) < 0) {
        return -1;
      }
    }
  } else if (array) {
    // put the values from input_json into an
    for (u32 i = 0; i < input_json.to_array().count(); i++) {
      if (import_value(nullptr, array, "", input_json.to_array().at(i)) < 0) {
        return -1;
      }
    }
  }
  return 0;
}

int CloudMap::import_value(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key,
  const json::JsonValue &value) {
  if (value.is_false()) {
    import_boolean(object, array, key, value);
  } else if (value.is_true()) {
    import_boolean(object, array, key, value);
  } else if (value.is_string()) {
    import_string(object, array, key, value);
  } else if (value.is_null()) {
    import_null(object, array, key);
  } else if (value.is_integer()) {
    import_integer(object, array, key, value);
  } else if (value.is_real()) {
    import_real(object, array, key, value);
  } else if (value.is_array()) {
    JsonArray array_result;
    import_json_recursive(nullptr, &array_result, value.to_array());
    if (import_array(object, array, key, array_result) < 0) {
      // this will return -1 if an array is inserted in an array
      return -1;
    }
  } else if (value.is_object()) {
    JsonObject map_result;
    map_result.insert(("fields"), JsonObject());
    import_json_recursive(
      &map_result.at(("fields")).to_object(),
      nullptr,
      value.to_object());
    import_map(object, array, key, map_result);
  }
  return 0;
}

int CloudMap::import_string(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key,
  const json::JsonValue &value) {
  JsonObject value_object;
  value_object.insert(("stringValue"), value);
  if (object) {
    object->insert(key, value_object);
  }
  if (array) {
    array->append(value_object);
  }
  return 0;
}

int CloudMap::import_integer(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key,
  const json::JsonValue &value) {
  JsonObject value_object;
  value_object.insert(("integerValue"), value);
  if (object) {
    object->insert(key, value_object);
  }
  if (array) {
    array->append(value_object);
  }
  return 0;
}

int CloudMap::import_real(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key,
  const json::JsonValue &value) {
  JsonObject value_object;
  value_object.insert(("doubleValue"), value);
  if (object) {
    object->insert(key, value_object);
  }
  if (array) {
    array->append(value_object);
  }
  return 0;
}

int CloudMap::import_boolean(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key,
  const json::JsonValue &value) {
  JsonObject value_object;
  value_object.insert(("booleanValue"), value);
  if (object) {
    object->insert(key, value_object);
  }
  if (array) {
    array->append(value_object);
  }
  return 0;
}

int CloudMap::import_null(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key) {
  JsonObject value_object;
  value_object.insert(("nullValue"), JsonInteger(0));
  if (object) {
    object->insert(key, value_object);
  }
  if (array) {
    array->append(value_object);
  }
  return 0;
}

int CloudMap::import_map(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key,
  const json::JsonObject &map) {
  JsonObject value_object;
  value_object.insert(("mapValue"), map);
  if (object) {
    object->insert(key, value_object);
  }
  if (array) {
    array->append(value_object);
  }
  return 0;
}

int CloudMap::import_array(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key,
  const json::JsonArray &array_map) {
  JsonObject array_values;
  array_values.insert(("values"), array_map);
  JsonObject value_object;
  value_object.insert(("arrayValue"), array_values);
  if (object) {
    object->insert(key, value_object);
  }
  if (array) {
    // can't insert ann array inside another array
    return -1;
  }
  return 0;
}

int CloudMap::export_json_recursive(
  json::JsonObject *object,
  json::JsonArray *array,
  const json::JsonObject &input_json) {
  // input is an object or array
  if (object) {
    // output is an object, input is an object inside fields
    JsonObject fields = input_json.at("fields").to_object();
    const auto key_list = fields.get_key_list();
    for (const auto &key : key_list) {
      export_value(object, nullptr, key, fields.at(key).to_object());
    }
  } else if (array) {
    // output is an array, input is an arrayValue
    JsonArray array_input = input_json.at(("values")).to_array();
    for (u32 i = 0; i < array_input.count(); i++) {
      export_value(nullptr, array, "", array_input.at(i).to_object());
    }
  }

  return 0;
}

int CloudMap::export_value(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key,
  const json::JsonObject &value) {

  const JsonObject::KeyList keys = value.get_key_list();
  if (keys.count() > 0) {
    if (keys.at(0) == var::StringView("stringValue")) {
      return export_string(object, array, key, value);
    }
    if (keys.at(0) == var::StringView("integerValue")) {
      return export_integer(object, array, key, value);
    }
    if (keys.at(0) == var::StringView("doubleValue")) {
      return export_real(object, array, key, value);
    }
    if (keys.at(0) == var::StringView("booleanValue")) {
      return export_boolean(object, array, key, value);
    }
    if (keys.at(0) == var::StringView("nullValue")) {
      return export_null(object, array, key, value);
    }
    if (keys.at(0) == var::StringView("arrayValue")) {
      return export_array(object, array, key, value);
    }
    if (keys.at(0) == var::StringView("mapValue")) {
      return export_map(object, array, key, value);
    }
  }
  return -1;
}

int CloudMap::export_string(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key,
  const json::JsonObject &value) {
  if (object) {
    object->insert(key, value.to_object().at(("stringValue")));
    return object->return_value();
  }
  if (array) {
    array->append(value.to_object().at(("stringValue")));
    return array->return_value();
  }
  return -1;
}

int CloudMap::export_integer(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key,
  const json::JsonObject &value) {
  if (object) {
    object->insert(key, value.at(("integerValue")));
    return object->return_value();
  }
  if (array) {
    array->append(value.at(("integerValue")));
    return array->return_value();
  }
  return -1;
}

int CloudMap::export_real(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key,
  const json::JsonObject &value) {
  if (object) {
    object->insert(key, value.at("doubleValue"));
    return object->return_value();
  }
  if (array) {
    array->append(value.at(("doubleValue")));
    return array->return_value();
  }
  return -1;
}

int CloudMap::export_boolean(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key,
  const json::JsonObject &value) {
  JsonValue booleanObject;
  if (value.at(("booleanValue")).to_bool()) {
    booleanObject = JsonTrue();
  } else {
    booleanObject = JsonFalse();
  }
  if (object) {
    object->insert(key, booleanObject);
    return object->return_value();
  }
  if (array) {
    array->append(booleanObject);
    return array->return_value();
  }
  return -1;
}

int CloudMap::export_null(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key,
  const json::JsonObject &value) {
  MCU_UNUSED_ARGUMENT(value);
  if (object) {
    object->insert(key, JsonNull());
    return object->return_value();
  }
  if (array) {
    array->append(JsonNull());
    return array->return_value();
  }
  return -1;
}

int CloudMap::export_array(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key,
  const json::JsonObject &value) {
  JsonArray output_array;
  export_json_recursive(
    nullptr,
    &output_array,
    value.at(("arrayValue")).to_object());
  if (object) {
    object->insert(key, output_array);
    return object->return_value();
  }
  if (array) {
    array->append(output_array);
    return array->return_value();
  }
  return -1;
}

int CloudMap::export_map(
  json::JsonObject *object,
  json::JsonArray *array,
  const var::StringView key,
  const json::JsonObject &value) {
  JsonObject output_object;
  export_json_recursive(
    &output_object,
    nullptr,
    value.at(("mapValue")).to_object());
  if (object) {
    object->insert(key, output_object);
    return object->return_value();
  }
  if (array) {
    array->append(output_object);
    return array->return_value();
  }
  return -1;
}
