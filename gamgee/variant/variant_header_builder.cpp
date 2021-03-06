#include "variant_header_builder.h"

#include "../utils/hts_memory.h"
#include "../utils/variant_utils.h"

#include "htslib/vcf.h"

#include <string>
#include <iostream>

namespace gamgee {

using namespace std;

static inline string required_parameter(const string& prefix, const string& parameter) {
  return string{","}.append(prefix).append(parameter);
}

static inline string optional_parameter(const string& prefix, const string& parameter) {
  return parameter.empty() ? "" : required_parameter(prefix, parameter);
}

VariantHeaderBuilder::VariantHeaderBuilder() noexcept : 
  m_header {bcf_hdr_init("w"), utils::VariantHeaderDeleter()}
{}

VariantHeaderBuilder::VariantHeaderBuilder(const VariantHeader& header) :
  m_header {utils::make_shared_variant_header(utils::variant_header_deep_copy(header.m_header.get()))}
{}

VariantHeaderBuilder& VariantHeaderBuilder::add_chromosome(const string& id, const string& length, const string& url, const string& extra) {
  auto s = string{"##contig=<ID=" + id};
  s.append(optional_parameter("length=", length));
  s.append(optional_parameter("url=", url));
  s.append(optional_parameter("", extra));
  s.append(">");
  bcf_hdr_append(m_header.get(), s.c_str());
  return *this;
}

VariantHeaderBuilder& VariantHeaderBuilder::add_filter(const string& id, const string& description, const string& extra) {
  auto s = string{"##FILTER=<ID=" + id};
  s.append(optional_parameter("Description=", description));
  s.append(optional_parameter("", extra));
  s.append(">");
  bcf_hdr_append(m_header.get(), s.c_str());
  return *this;
}

VariantHeaderBuilder& VariantHeaderBuilder::add_shared_field(const string& id, const string& number, const string& type, const string& description, const string& source, const string& version, const string& extra) {
  auto s = string{"##INFO=<ID=" + id};
  s.append(required_parameter("Number=", number));
  s.append(required_parameter("Type=", type));
  s.append(optional_parameter("Description=", description));
  s.append(optional_parameter("Source=", source));
  s.append(optional_parameter("Version=", version));
  s.append(optional_parameter("", extra));
  s.append(">");
  bcf_hdr_append(m_header.get(), s.c_str());
  return *this;
}

VariantHeaderBuilder& VariantHeaderBuilder::add_individual_field(const string& id, const string& number, const string& type, const string& description, const string& extra) {
  auto s = string{"##FORMAT=<ID=" + id};
  s.append(required_parameter("Number=", number));
  s.append(required_parameter("Type=", type));
  s.append(optional_parameter("Description=", description));
  s.append(optional_parameter("", extra));
  s.append(">");
  bcf_hdr_append(m_header.get(), s.c_str());
  return *this;
}

VariantHeaderBuilder& VariantHeaderBuilder::add_source(const string& source) {
  auto s = string{"##FORMAT=<source=" + source + ">"};
  bcf_hdr_append(m_header.get(), s.c_str());
  return *this;
}

VariantHeaderBuilder& VariantHeaderBuilder::add_sample(const string& sample) {
  bcf_hdr_add_sample(m_header.get(), sample.c_str());
  return *this;
}

VariantHeaderBuilder& VariantHeaderBuilder::advanced_add_arbitrary_line(const std::string& line) {
  bcf_hdr_append(m_header.get(), line.c_str());
  return *this;
}

VariantHeaderBuilder& VariantHeaderBuilder::merge(const VariantHeader& other_header) {
  merge_variant_headers(m_header, other_header.m_header);
  return *this;
}


} // end of namespace
