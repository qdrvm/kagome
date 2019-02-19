#ifndef KAGOME_KEYVALUE_HPP
#define KAGOME_KEYVALUE_HPP

namespace kagome::storage {

class KeyValue {
 public:

  /**
   * @brief Store value by key
   * @param key non-empty byte buffer
   * @param value byte buffer
   */
  virtual void put(const Buffer &key, const Buffer &value) = 0;

  /**
   * @brief Get value by key
   * @param key non-empty byte buffer
   * @return buffer of size 0 if no data is stored given {@param key} or buffer with data
   */
  virtual Buffer get(const Buffer &key) const = 0;
};

}

#endif //KAGOME_KEYVALUE_HPP
