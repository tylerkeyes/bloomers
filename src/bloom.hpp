#pragma once

#include <string_view>
#include <vector>

constexpr int const bitset_len = 100;

class bloom_filter
{
public:
  /**
   * @brief Number of items in the bloom filter
   *
   * The number of elements that is expected will populate the bloom filter.
   * Used to calculate the size of the bit array.
   */
  struct expected_items
  {
    size_t value;
  };

  /**
   * @brief Desired false positive rate.
   *
   * The desired false positive rate of checking if an element is in the bloom
   * filter. Used to calculate the size of the bit array.
   */
  struct false_positive_rate
  {
    double value;
  };

  bloom_filter(expected_items expected_items,
               false_positive_rate false_positive_rate);

  bloom_filter(size_t bits, std::vector<bool> bitset, size_t hashes)
      : m_bits(bits)
      , m_khashes(hashes)
      , m_bitset(std::move(bitset))
  {
  }

  /**
   * @brief Insert a string into the bloom filter
   */
  void insert(std::string_view data);

  /**
   * @brief Check if a string is probably in the bloom filter
   *
   * Checks if a string is probably in the bloom filter, with a false positive
   * rate specifed when creating the bloom filter. If the function returns
   * 'false', then the element is guaranteed to not be present.
   */
  auto possibly_contains(std::string_view data) const -> bool;

  /**
   * @brief View the state of a bloom_filter
   */
  auto inspect(bool full = true) -> std::string;

  auto get_bitset() -> std::vector<bool> { return m_bitset; }

  auto get_bits() const -> size_t { return m_bits; }

  auto get_khashes() const -> size_t { return m_khashes; }

  void set_bitset(std::vector<bool> bitset) { m_bitset = std::move(bitset); }

private:
  std::vector<bool> m_bitset;
  size_t m_bits;
  size_t m_khashes;
};
