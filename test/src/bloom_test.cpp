#include <cstddef>
#include <exception>
#include <iostream>

#include "bloom.hpp"

#include <gtest/gtest.h>

namespace
{
auto basic_init(const size_t expected_items = 100,
                const double false_positive_rate = 0.01) -> bloom_filter
{
  bloom_filter filter(bloom_filter::expected_items {expected_items},
                      bloom_filter::false_positive_rate {false_positive_rate});
  return filter;
}
}  // namespace

TEST(BloomFilterTest, BasicInsertCheck)
{
  auto filter = basic_init();

  filter.insert("apple");
  EXPECT_TRUE(filter.possibly_contains("apple"));
  EXPECT_FALSE(filter.possibly_contains("banana"));

  filter.insert("orange");
  EXPECT_TRUE(filter.possibly_contains("orange"));
}

TEST(BloomFilterTest, EmptyCheck)
{
  auto filter = basic_init();

  EXPECT_FALSE(filter.possibly_contains("some_value"));
  EXPECT_FALSE(filter.possibly_contains("other_value"));
}

TEST(BloomFilterTest, HighFailureRate)
{
  const size_t expected_items = 3;
  const double false_positive_rate = 0.70;
  // This results in a bitset of length 2
  auto filter = basic_init(expected_items, false_positive_rate);

  filter.insert("apple");
  filter.insert("elephant");
  filter.insert("parrot");
  // Even though 'orange' is not present, this should return 'true' as the
  // bit-string is not long enough for the number of elements.
  std::cout << "Filter: " << filter.inspect() << '\n';
  EXPECT_TRUE(filter.possibly_contains("orange"));
}

TEST(BloomFilterTest, InvalidExpectedItems)
{
  const size_t expected_items = -1;
  const double false_positive_rate = 0.01;
  try {
    auto filter = basic_init(expected_items, false_positive_rate);
  } catch (const std::exception& e) {
    EXPECT_TRUE(true);
    return;
  }
  EXPECT_TRUE(false);
}

TEST(BloomFilterTest, InvalidFalsePositiveRate)
{
  const size_t expected_items = 100;
  const double false_positive_rate_high = 1.1;
  try {
    auto filter = basic_init(expected_items, false_positive_rate_high);
  } catch (const std::exception& e) {
    EXPECT_TRUE(true);
  }

  const double false_positive_rate_low = -1.0;
  try {
    auto filter = basic_init(expected_items, false_positive_rate_low);
  } catch (const std::exception& e) {
    EXPECT_TRUE(true);
    return;
  }

  EXPECT_TRUE(false);
}
