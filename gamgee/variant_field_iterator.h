#ifndef __gamgee__format_field_iterator__
#define __gamgee__format_field_iterator__ 

#include "utils/utils.h"

#include "htslib/vcf.h"

#include<iterator>
#include<sstream>

namespace gamgee {


/** 
 * @brief iterator for VariantField objects. 
 * 
 * This iterator will walk through all the samples in a Variant record for a
 * given VariantField object. For example if you want to iterate over all the GQ
 * values of a Variant record you would do so through this iterator.
 *
 * @note implements a random access iterator which gives you full performance
 * on STL algorithms that use iterators (mostly every one)
 * 
 * @note this iterator never makes any copies of the underlying memory. It is
 * implemented with pointer acrobatics behind the scenes for maximum
 * performance while maintaining a friendly interface.
 *
 * @warning modifying any elements via this iterator **will modify** the values
 * in the Variant record. 
 */
template<class TYPE>
class VariantFieldIterator : public std::iterator<std::random_access_iterator_tag, TYPE> {
 public:

  /**
   * @brief simple constructor used by VariantField to create an iterator
   * @param body the Variant record used to access the data (shared ownership guarantees availability until iterator goes out of scope)
   * @param format_ptr pointer to the raw byte array in body where all the values for this format field is encoded
   * @param end_iterator whether or not this is being called by the VariantField::end() function.
   * @note this constructor serves only the VariantField::begin() and VariantField::end() functions. 
   */
  VariantFieldIterator(const std::shared_ptr<bcf1_t>& body, const bcf_fmt_t* const format_ptr, bool end_iterator = false) :
    m_body {body}, 
    m_format_ptr {format_ptr},
    m_data_ptr {end_iterator ? format_ptr->p + m_format_ptr->size * m_body->n_sample : format_ptr->p} 
  {}

  /**
   * @brief copy is not allowed in this iterator. Use move constructor instead.
   */
  VariantFieldIterator(const VariantFieldIterator& other) = delete;

  /**
   * @brief safely moves the data from one VariantField to a new one without making any copies
   * @param other another VariantField object
   */
  VariantFieldIterator(VariantFieldIterator&& other) noexcept : 
    m_body {std::move(other.m_body)},
    m_format_ptr {other.m_format_ptr},
    m_data_ptr {other.m_data_ptr}
  {}

  /**
   * @copydoc VariantFieldIterator::VariantFieldIterator(const VariantFieldIterator&)
   */
  VariantFieldIterator& operator=(const VariantFieldIterator& other) = delete;

  /**
   * @copydoc VariantFieldIterator::VariantFieldIterator(VariantFieldIterator&&)
   */
  VariantFieldIterator& operator=(VariantFieldIterator&& other) noexcept {
    if (&this == other)
      return *this;
    m_body = std::move(other.m_body);
    m_format_ptr = other.m_format_ptr;
    m_data_ptr = other.m_data_ptr;
    return *this;
  }

  /**
   * @brief simple compound assignment operation for random advances (back/forward) to the iterator
   * @param n how much to advance (negative numbers to go the other direction)
   * @warning there is no boundary check in this operator
   */
  VariantFieldIterator& operator+=(const int n) {
    m_data_ptr += n * m_format_ptr->size;
    return *this;
  }

  /**
   * @copydoc VariantFieldIterator::operator+=(int)
   */
  VariantFieldIterator& operator-=(const int n) {
    m_data_ptr -= n * m_format_ptr->size;
    return *this;
  }

  /**
   * @brief two iterators are equal if they are in exactly the same state (pointing at the same location in memory
   */
  bool operator==(const VariantFieldIterator& other) {
    return m_body == other.m_body && m_data_ptr == other.m_data_ptr;
  }

  /**
   * @brief the oposite check of VariantFieldIterator::operator==()
   */
  bool operator!=(const VariantFieldIterator& other) {
    return m_body != other.m_body || m_data_ptr != other.m_data_ptr;
  }

  /**
   * @brief an operator is greater/less than another iterator if it is pointing to a previous element (sample) in the VariantField 
   * object. The order is determined by the Variant record.
   */
  bool operator<(const VariantFieldIterator& other) {
    return m_body == other.m_body && m_data_ptr < other.m_data_ptr;
  }

  /**
   * @copydoc VariantFieldIterator::operator<()
   */
  bool operator>(const VariantFieldIterator& other) {
    return m_body == other.m_body && m_data_ptr > other.m_data_ptr;
  }

  /**
   * @copydoc VariantFieldIterator::operator<()
   */
  bool operator<=(const VariantFieldIterator& other) {
    return m_body == other.m_body && m_data_ptr <= other.m_data_ptr;
  }

  /**
   * @copydoc VariantFieldIterator::operator<()
   */
  bool operator>=(const VariantFieldIterator& other) {
    return m_body == other.m_body && m_data_ptr >= other.m_data_ptr;
  }

  /**
   * @brief direct access to the value of the current sample
   * @return the value if it is a basic type (e.g. GQ, GL), or a specific object if it is a complex type (e.g. PL, AD,...)
   */
  TYPE operator*() const noexcept {
    return TYPE{m_body, m_format_ptr, m_data_ptr};
  }

  /**
   * @brief advances to the next sample
   * @note mainly designed for iterators
   * @warning does not check for bounds exception, you should verify whether or not you've reached the end by comparing the result of operator* with end(). This is the STL way.
   * @return a reference to the start of the values of the next sample
   */
  TYPE operator++() noexcept {
    m_data_ptr += m_format_ptr->size;
    return TYPE{m_body, m_format_ptr, m_data_ptr};
  }

  /**
   * @brief advances to the previous sample
   * @note mainly designed for iterators
   * @warning does not check for bounds exception, you should verify whether or not you've reached the end by comparing the result of operator* with end(). This is the STL way.
   * @return a reference to the start of the values of the previous sample
   */
  TYPE operator--() {
    m_data_ptr -= m_format_ptr->size;
    return TYPE{m_body, m_format_ptr, m_data_ptr};
  }

  /**
   * @brief random access to the value of a given sample for reading or writing
   * @param sample must be between 0 and the number of samples for this record 
   * @note implementation guarantees this operation to be O(1)
   * @exception std::out_of_range if index is out of range
   * @return the value if it is a basic type (e.g. GQ, GL), or a specific object if it is a complex type (e.g. PL, AD,...)
   */
  TYPE operator[](const uint32_t sample) const {
    utils::check_boundaries(sample, m_body->n_sample);
    return TYPE{m_body, m_format_ptr, m_data_ptr + (sample * m_body->n_sample)};
  }

 private:
  std::shared_ptr<bcf1_t> m_body; ///< shared ownership of the Variant record memory so it stays alive while this object is in scope
  const bcf_fmt_t* const m_format_ptr;           ///< pointer to the format_field in the body so we can access the tag's information 
  uint8_t* m_data_ptr;            ///< pointer to m_body structure where the data for this particular type is located.

};

// template specialization for simple types 

/**
 * Specialized version for uint8_t where the value returned is just a value instead of a full type
 *
 * @copydoc VariantFieldIterator::operator*()
 */
template<> inline
uint8_t VariantFieldIterator<uint8_t>::operator*() const noexcept {
  return *m_data_ptr;
}

/**
 * Specialized version for uint8_t where the value returned is just a value instead of a full type
 *
 * @copydoc VariantFieldIterator::operator++()
 */
template<> inline
uint8_t VariantFieldIterator<uint8_t>::operator++() noexcept {
  ++m_data_ptr;
  return *m_data_ptr;
}

/**
 * Specialized version for uint8_t where the value returned is just a value instead of a full type
 *
 * @copydoc VariantFieldIterator::operator--()
 */
template<> inline
uint8_t VariantFieldIterator<uint8_t>::operator--() {
  --m_data_ptr;
  return *m_data_ptr;
}

/**
 * Specialized version for uint8_t where the value returned is just a value instead of a full type
 *
 * @copydoc VariantFieldIterator::operator[]()
 */
template<> inline
uint8_t VariantFieldIterator<uint8_t>::operator[](const uint32_t sample) const {
  utils::check_boundaries(sample, m_body->n_sample);
  return m_data_ptr[sample];
}

// non-member functions 
 
/**
 * @brief simple operation for random advances (back and forward) on the iterator
 */
template<class TYPE> inline
VariantFieldIterator<TYPE>& operator+(const VariantFieldIterator<TYPE>& it, const int n) {
  return it += n;
}

/**
 * @copydoc operator+(const VariantFieldIterator&, const int)
 */
template<class TYPE> inline
VariantFieldIterator<TYPE>& operator+(const int n, const VariantFieldIterator<TYPE>& it) {
  return it += n;
}

/**
 * @copydoc operator+(const VariantFieldIterator&, const int)
 */
template<class TYPE> inline
VariantFieldIterator<TYPE>& operator-(const VariantFieldIterator<TYPE>& it, const int n) {
  return it -= n;
}

/**
 * @copydoc operator+(const VariantFieldIterator&, const int)
 */
template<class TYPE> inline
VariantFieldIterator<TYPE>& operator-(const int n, const VariantFieldIterator<TYPE>& it) {
  return it -= n;
}


}

#endif