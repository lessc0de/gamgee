#include "sam_header.h"

#include "../utils/hts_memory.h"

using namespace std;

namespace gamgee {

  /**
   * @brief creates a SamHeader object that points to htslib memory already allocated
   *
   * @note the resulting SamHeader object shares ownership of the pre-allocated memory via
   *       shared_ptr reference counting
   */
  SamHeader::SamHeader(const std::shared_ptr<bam_hdr_t>& header) :
    m_header { header }
  {}

  /**
   * @brief creates a deep copy of a SamHeader object
   *
   * @note the copy will have exclusive ownership over the newly-allocated htslib memory
   */
  SamHeader::SamHeader(const SamHeader& other) :
    m_header { utils::make_shared_sam_header(utils::sam_header_deep_copy(other.m_header.get())) }
  {}

  /**
   * @brief creates a deep copy of a SamHeader object
   *
   * @note the copy will have exclusive ownership over the newly-allocated htslib memory
   */
  SamHeader& SamHeader::operator=(const SamHeader& other) {
    if ( &other == this )  
      return *this;
    m_header = utils::make_shared_sam_header(utils::sam_header_deep_copy(other.m_header.get())); ///< shared_ptr assignment will take care of deallocating old sam record if necessary
    return *this;
  }

  /**
   * @brief Returns the length of the given sequence as stored in the \@SQ tag in the BAM header, or 0 if the sequence
   * name is not found.
   */
  uint32_t SamHeader::sequence_length(const std::string& sequence_name) const {
	  auto c = sequence_name.c_str();
	  for (int i = 0; i < m_header->n_targets; i++) {
		  if (strcmp(c,m_header->target_name[i]) == 0) {
			  return m_header->target_len[i];
		  }
	  }
	  return 0;
  }

  /**
   * @brief extracts read group objects from a SAM header
   */
  vector<ReadGroup> SamHeader::read_groups() const {
    const static auto RG_TAG = "@RG";
    const static auto NOT_FOUND = string::npos;
    auto result = vector<ReadGroup>();
    auto text = header_text();

    for (auto rg_start=text.find(RG_TAG), rg_end=rg_start; rg_start!=NOT_FOUND; rg_start=text.find(RG_TAG,rg_end+1) ) {
      rg_end = text.find('\n', rg_start+1);
      auto rg_record = text.substr(rg_start, rg_end-rg_start);
      result.push_back(ReadGroup(rg_record));
    }
    return result;
  }

}
