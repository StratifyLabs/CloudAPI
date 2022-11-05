
#include <test/Test.hpp>
#include <crypto/Random.hpp>
#include <fs/File.hpp>
#include <fs/DataFile.hpp>
#include <fs/ViewFile.hpp>
#include <json/JsonDocument.hpp>
#include <thread/Thread.hpp>

#include "cloud.hpp"

using namespace var;
using namespace printer;
using namespace crypto;
using namespace fs;
using namespace json;
using namespace thread;


//"cloudapitest-2ec81"

class UnitTest : public test::Test {
  static constexpr auto database_project = "cloudapitest-2ec81";

public:
  UnitTest(var::StringView name)
    : test::Test(name), cloud("AIzaSyCFaqqhpCAQIOXQbtmvcXTvuerk2tPE6tI"),
      database(cloud, database_project), storage(cloud, database_project),
      store(cloud, database_project) {}

  bool execute_class_api_case() {
    Cloud::set_default_printer(printer());

    TEST_ASSERT_RESULT(credentials_case());
    TEST_ASSERT_RESULT(storage_case());
    TEST_ASSERT_RESULT(document_case());
    TEST_ASSERT_RESULT(database_case());
    TEST_ASSERT_RESULT(database_event_stream_case());

    return true;
  }

  bool storage_case() {
    Printer::Object po(printer(), "storage");

    const StringView path = "comment-20.txt";
    printer().object("details", storage.get_details(path));
    TEST_ASSERT(is_success());

    TEST_ASSERT(storage.get_object(path, NullFile()).is_success());

    const auto name = [](){
      Array<char, 16> result;
      Random().seed().randomize(result);
      return result;
    }();

    const PathString contents_path
      = PathString("path/to")
        / View(name).to_string<GeneralString>().string_view();
    const StringView contents = "This is a new object";

    printer().key("create", contents_path);
    TEST_ASSERT(storage.create_object(contents_path, ViewFile(contents))
                  .is_success());

    printer().key("verify", contents_path);
    DataFile contents_file;
    TEST_ASSERT(
      storage.get_object(contents_path, contents_file).is_success());

    TEST_ASSERT(View(contents_file.data()) == View(contents));

    return true;
  }

  bool database_event_stream_case() {
    Printer::Object po(printer(), "databaseEventStream");

    {
      KeyString id = database.create_object(
        "events",
        JsonObject().insert("name", JsonString("no name")),
        "myevent");

      TEST_ASSERT(is_success());

      thread::Mutex mutex;
      DataFile incoming_data;

      struct ListenContext {
        UnitTest *self;
        thread::Mutex mutex;
        DataFile incoming_data;
      };

      ListenContext listen_context;

      listen_context.self = this;

      Thread t(Thread::Construct()
                 .set_argument(&listen_context)
                 .set_function([](void *args) -> void * {
                   ListenContext *listen_context
                     = reinterpret_cast<ListenContext *>(args);

                   listen_context->self->database.listen(
                     "events/myevent",
                     listen_context->incoming_data,
                     &listen_context->mutex);

                   return nullptr;
                 }));

      database.create_object(
        "events",
        JsonObject().insert("name", JsonString("testing")),
        "myevent");

      wait(1_seconds);
      {
        thread::Mutex::Guard mg(listen_context.mutex);
        printer().key(
          "dataSize",
          NumberString(listen_context.incoming_data.size()));
        printer().key(
          "content",
          listen_context.incoming_data.data().add_null_terminator());
      }

      API_ASSERT(is_success());
    }
    API_ASSERT(is_success());

    return true;
  }

  bool database_case() {
    Printer::Object po(printer(), "database");

    {
      const auto id = database.create_object(
        "projects",
        JsonObject().insert("name", JsonString("a new object")));

      TEST_ASSERT(is_success());

      database.remove_object("projects/" + id);
      TEST_ASSERT(is_success());
    }

    {
      KeyString id = database.create_object(
        "projects",
        JsonObject().insert("named", JsonString("a named object")),
        "namedObject");

      TEST_ASSERT(id == "namedObject");
      TEST_ASSERT(is_success());

      KeyString overwrite_id = database.create_object(
        "projects",
        JsonObject().insert("named", JsonString("another name")),
        "namedObject");

      printer().key("overwriteId", overwrite_id);
      TEST_ASSERT(is_success());

      database.patch_object(
        "projects/namedObject",
        JsonObject()
          .insert("named", JsonString("another name"))
          .insert("number", JsonReal(5.0f)));
      TEST_ASSERT(is_success());
    }
    {

      JsonObject test_object = database.get_value("projects/namedObject");

      printer().object("testObject", test_object);
      TEST_ASSERT(test_object.at("named").to_string_view() == "another name");
      TEST_ASSERT(test_object.at("number").to_integer() == 5);
    }
    {
      fs::DataFile test_file;
      database.get_value("projects/namedObject", test_file);
      JsonObject test_object = JsonDocument().load(test_file.seek(0));

      printer().object("testObject", test_object);
      TEST_ASSERT(test_object.at("named").to_string_view() == "another name");
      TEST_ASSERT(test_object.at("number").to_integer() == 5);
    }

    {
      database.remove_object("projects/namedObject");
      TEST_ASSERT(is_success());
    }

    return true;
  }

  bool document_case() {
    Printer::Object po(printer(), "document");
    {
      TEST_ASSERT(cloud.is_logged_in());

      KeyString id = store.create_document(
        "projects",
        JsonObject().insert("name", JsonString("test")),
        StringView());

      printer().key("id", id);

      TEST_ASSERT(is_success());

      JsonObject test_document = store.get_document(String("projects/" + id));
      TEST_ASSERT(is_success());

      TEST_ASSERT(test_document.at("name").to_string() == "test");
      if (0) {
        api::ignore = store.get_document(String("projects/badrequest"));
        printer().key_bool("error", is_error());
        TEST_ASSERT(is_error());
        API_RESET_ERROR();

        // test create existing document
        api::ignore = store.create_document(
          "projects",
          JsonObject().insert("name", JsonString("test")),
          id);
        TEST_ASSERT(is_error());
        API_RESET_ERROR();
      }
      {
        auto test_document = store.get_document(String("projects/" + id));
        printer().object("doc", test_document);
        TEST_ASSERT(is_success());
        test_document.insert("float", JsonReal(5.0f));
        test_document.insert("true", JsonTrue());
        test_document.insert("false", JsonFalse());

        store.patch_document(
          String("projects/" + id),
          test_document,
          Cloud::IsExisting::yes);
        TEST_ASSERT(is_success());
      }

      {
        const auto test_document = store.get_document(String("projects/" + id));
        TEST_ASSERT(test_document.at("float").to_real() == 5.0f);
        TEST_ASSERT(test_document.at("true").to_bool());
        TEST_ASSERT(!test_document.at("false").to_bool());
      }

      {
        store.remove_document(String("projects/" + id));
        TEST_ASSERT(is_success());
      }

      {

        store.remove_document("projects/namedDocument");
        API_RESET_ERROR();

        // create a document by name
        KeyString id = store.create_document(
          "projects",
          JsonObject()
            .insert("name", JsonString("named"))
            .insert("document", JsonString("already has a name")),
          StringView("namedDocument"));

        TEST_ASSERT(id = "namedDocument");

        auto test_document = store.get_document("projects/namedDocument");

        TEST_ASSERT(test_document.at("name").to_string() == "named");
      }
    }
    return true;
  }

  bool credentials_case() {
    Printer::Object po(printer(), "credentials");
    {

      TEST_ASSERT(!cloud.is_logged_in());
      TEST_ASSERT(
        cloud.login("test@stratifylabs.co", "testing-user").is_success());

      TEST_ASSERT(cloud.is_logged_in());

      printer().key("uid", cloud.credentials().get_uid());
      TEST_ASSERT(
        cloud.credentials().get_uid() == "7BYQMxwh7JfUk94LlMCFlhFKfll2");

      TEST_ASSERT(cloud.refresh_login().is_success());
      TEST_ASSERT(cloud.is_logged_in());
    }
    return true;
  }

private:
  Cloud cloud;
  Database database;
  Storage storage;
  Store store;
};
