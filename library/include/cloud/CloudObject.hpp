// Copyright 2016-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md

#ifndef CLOUDSERVICE_CLOUDSERVICE_OBJECT_HPP
#define CLOUDSERVICE_CLOUDSERVICE_OBJECT_HPP

/*! \mainpage
 *
 */

#include <sdk/types.h>
#include <printer/Printer.hpp>
#include <var/String.hpp>

#define CLOUD_PRINTER_TRACE(msg) PRINTER_TRACE(printer(), msg)

namespace cloud {

/*! \brief Application Programming Interface Object
 *
 */
class CloudObject : public api::ExecutionContext {
public:
  /*! \details Returns a pointer to a string
   * that shows the Toolkit version.
   */
  static const char *cloud_service_version() { return "0.0.1"; }
  static const char *cloud_service_git_hash();

  printer::Printer &printer() const {
    if (m_default_printer == nullptr) {
      // fatal error
      printf("No printer\n");
      exit(1);
    }
    return *m_default_printer;
  }

  static printer::Printer &null_printer() { return m_null_printer; }

  static void set_default_printer(printer::Printer &printer) {
    m_default_printer = &printer;
  }

protected:
private:
  static printer::Printer *m_default_printer;
  static printer::NullPrinter m_null_printer;
};

} // namespace cloud

#endif // CLOUDSERVICE_CLOUDSERVICE_OBJECT_HPP
