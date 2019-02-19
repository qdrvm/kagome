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
  virtual void put(Buffer key, Buffer value) = 0;

  /**
   * @brief Get value by key
   * @param key
   * @return
   */
  virtual Buffer get(Buffer key) = 0;
};

}

#endif //KAGOME_KEYVALUE_HPP
