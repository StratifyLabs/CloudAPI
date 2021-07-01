#ifndef CLOUDACCESS_HPP
#define CLOUDACCESS_HPP

#include "Database.hpp"
#include "Storage.hpp"
#include "Store.hpp"

namespace cloud {

class CloudService : public CloudObject {
  Cloud m_cloud;
  Database m_database;
  Storage m_storage;
  Store m_store;

public:
  CloudService(const var::StringView api_key, const var::StringView project)
    : m_cloud(api_key, 0), m_database(m_cloud, project),
      m_storage(m_cloud, project), m_store(m_cloud, project) {}

  const Database &database() const { return m_database; }
  Database &database() { return m_database; }

  const Store &store() const { return m_store; }
  Store &store() { return m_store; }

  const Storage &storage() const { return m_storage; }
  Storage &storage() { return m_storage; }

  const Cloud &cloud() const { return m_cloud; }
  Cloud &cloud() { return m_cloud; }
};

class CloudAccess : public CloudObject {
public:


  enum class IsCreate { no, yes };

  CloudAccess() { m_service = m_default_service; }

  explicit CloudAccess(CloudService &service) { set_cloud_service(service); }

  static void set_default_cloud_service(CloudService &service) { m_default_service = &service; }

  void set_cloud_service(CloudService &service) {
    m_service = &service;
    if (m_default_service == nullptr) {
      m_default_service = m_service;
    }
  }

  CloudService &cloud_service() {
    API_ASSERT(m_service != nullptr);
    return *m_service;
  }

private:
  static CloudService *m_default_service;
  CloudService *m_service = nullptr;
};

} // namespace cloud

#endif // CLOUDACCESS_HPP
