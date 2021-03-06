#ifndef gamgee__synced_variant_reader__guard
#define gamgee__synced_variant_reader__guard

#include "htslib/vcf.h"
#include "htslib/synced_bcf_reader.h"

#include "../exceptions.h"
#include "../utils/hts_memory.h"

#include <string>
#include <iostream>

using namespace std;

namespace gamgee {

/**
 * @brief Utility class to read multiple VCF.GZ/BCF files with an appropriate iterator in a for-each loop.
 *
 * @note This reader requires all input files to be indexed
 *
 * @note Output vectors have the same size and the same order as the input filename vector, but not all files will have
 * a variant at every location, so it's necessary to check using missing()
 *
 * This class is designed to parse the files in for-each loops with the following signatures:
 *
 * over whole files:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * for (auto& record : SyncedVariantReader<SyncedVariantIterator>{filenames, ""})
 *   if (!missing(record))
 *     do_something_with_record(record);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * over intervals:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * for (auto& record : SyncedVariantReader<SyncedVariantIterator>{filenames, interval_string})
 *   if (!missing(record))
 *     do_something_with_record(record);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
template<class ITERATOR>
class SyncedVariantReader {
 public:

  /**
   * @brief opens multiple files (vcf or bcf) and allows an iterator to parse them
   *
   * @param filenames the names of the variant files
   * @param interval_list a comma-separated string of the intervals to traverse.  Empty string for all intervals.
   * @exception will throw std::runtime_error if the htslib structure cannot be initialized
   */
  SyncedVariantReader(const std::vector<std::string>& filenames, const std::string& interval_list) :
    m_synced_readers {utils::make_shared_synced_variant_reader(bcf_sr_init())}
  {
    auto success = 0;
    if (interval_list.empty()) {
      m_synced_readers->require_index = 1;
    }
    else {
      success = bcf_sr_set_regions(m_synced_readers.get(), interval_list.c_str(), 0);
      if (success != 0)
        throw HtslibException(success);
    }

    for (const auto& filename : filenames) {
      success = bcf_sr_add_reader(m_synced_readers.get(), filename.c_str());
      if (success != 1) {        // returns 1 on success to indicate 1 file added
        throw FileOpenException{filename};
      }
    }
  }

  /**
   * @brief a SyncedVariantReader should never be copied, but it can be moved around
   */
  SyncedVariantReader(SyncedVariantReader&& other) = default;
  SyncedVariantReader& operator=(SyncedVariantReader&& other) = default;

  /**
   * @brief a SyncedVariantReader cannot be copied safely, as it is iterating over streams.
   */
  SyncedVariantReader(const SyncedVariantReader&) = delete;
  SyncedVariantReader& operator=(const SyncedVariantReader&) = delete;

  /**
   * @brief creates a ITERATOR pointing at the start of the input stream (needed by for-each
   * loop)
   *
   * @return a ITERATOR ready to start parsing the files
   */
  ITERATOR begin() const {
    return ITERATOR{m_synced_readers};
  }

  /**
   * @brief creates a ITERATOR with a nullified input stream (needed by for-each loop)
   *
   * @return a ITERATOR that will match the end status of the iterator at the end of the stream
   */
  ITERATOR end() const {
    return ITERATOR{};
  }

 private:
  std::shared_ptr<bcf_srs_t> m_synced_readers;          ///< pointer to the synced readers of the variant files
};

}  // end namespace gamgee

#endif /* gamgee__synced_variant_reader__guard */
