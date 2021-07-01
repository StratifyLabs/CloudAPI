// Copyright 2016-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md

#include "cloud/CloudObject.hpp"

using namespace cloud;

printer::NullPrinter CloudObject::m_null_printer;
printer::Printer *CloudObject::m_default_printer = &m_null_printer;

const char * CloudObject::cloud_service_git_hash(){
#if defined SOS_GIT_HASH
	return SOS_GIT_HASH;
#else
	return "unknown";
#endif
}
