#include <bit>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <numbers>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>

#include "bloom.hpp"

namespace
{
const uint32_t shift_8 = 8;
const uint32_t shift_13 = 13;
const uint32_t shift_15 = 15;
const uint32_t shift_16 = 16;
const uint32_t shift_17 = 17;
const uint32_t shift_19 = 19;

const uint32_t hash_const1 = 0xe6546b64;
const uint32_t hash_mult1 = 5;
const uint32_t hash_const2 = 0x85ebca6b;
const uint32_t hash_const3 = 0xc2b2ae35;

inline auto murmur3_32(std::span<const std::byte> key, uint32_t seed = 0)
    -> uint32_t
{
  const uint32_t cst1 = 0xcc9e2d51;
  const uint32_t cst2 = 0x1b873593;

  uint32_t hash = seed;
  const size_t len = key.size();
  const size_t nblocks = len / 4;

  for (int i = 0; i < nblocks; ++i) {
    uint32_t tmp = 0;
    std::memcpy(&tmp,
                key.subspan(i * sizeof(uint32_t), sizeof(uint32_t)).data(),
                sizeof(uint32_t));

    tmp *= cst1;
    tmp = std::rotl(tmp, shift_15);
    tmp *= cst2;

    hash ^= tmp;
    hash = std::rotl(hash, shift_13);
    hash = (hash * hash_mult1) + hash_const1;
  }

  // const uint8_t* tail = key + (nblocks * 4);
  auto tail = key.subspan(nblocks * sizeof(uint32_t));
  uint32_t tmp = 0;
  const size_t rem = tail.size();

  if (rem >= 3) {
    tmp ^= static_cast<uint32_t>(std::to_integer<uint8_t>(tail[2])) << shift_16;
  }
  if (rem >= 2) {
    tmp ^= static_cast<uint32_t>(std::to_integer<uint8_t>(tail[1])) << shift_8;
  }
  if (rem >= 1) {
    tmp ^= static_cast<uint32_t>(std::to_integer<uint8_t>(tail[0]));
    tmp *= cst1;
    tmp = std::rotl(tmp, shift_15);
    tmp *= cst2;
    hash ^= tmp;
  }

  hash ^= static_cast<uint32_t>(len);
  hash ^= hash >> shift_16;
  hash *= hash_const2;
  hash ^= hash >> shift_13;
  hash *= hash_const3;
  hash ^= hash >> shift_16;

  return hash;
}

auto hash_pair(std::string_view data) -> std::pair<uint32_t, uint32_t>
{
  const uint32_t hash_seed1 = 0x9747b28c;
  const uint32_t hash_seed2 = 0x12345678;

  auto bytes = std::as_bytes(std::span {data});

  uint32_t hash1 = murmur3_32(bytes, hash_seed1);
  uint32_t hash2 = murmur3_32(bytes, hash_seed2);
  return {hash1, hash2};
}
}  // namespace

bloom_filter::bloom_filter(expected_items expected_items,
                           false_positive_rate false_positive_rate)
{
  // Compute optimal m and k using standard Bloom filter formulas:
  // m = - (n * ln(p) / (ln(2)^2))
  // k = (m / n) * ln(2)
  if (expected_items.value < 0) {
    throw std::runtime_error(std::format(
        "Expected items should be positive: {}", expected_items.value));
  }
  if (false_positive_rate.value < 0.0 || false_positive_rate.value > 1.0) {
    throw std::runtime_error(
        std::format("False positive rate should be between 0.0 and 1.0: {}",
                    false_positive_rate.value));
  }

  const double ln2 = std::numbers::ln2;
  const auto num_items = static_cast<double>(expected_items.value);

  const double num_bits =
      -(num_items * std::log(false_positive_rate.value)) / (ln2 * ln2);
  const double num_hashes = (num_bits / num_items) * ln2;

  m_bits = static_cast<size_t>(num_bits);
  m_khashes = static_cast<size_t>(std::round(num_hashes));

  m_bitset.resize(m_bits);
}

auto bloom_filter::possibly_contains(std::string_view data) const -> bool
{
  auto [hash1, hash2] = hash_pair(data);
  return std::ranges::all_of(std::views::iota(size_t {0}, m_khashes),
                             [&](size_t idx) -> bool
                             {
                               size_t bit_idx = (hash1 + idx * hash2) % m_bits;
                               return m_bitset[bit_idx];
                             });
}

void bloom_filter::insert(std::string_view data)
{
  auto [hash1, hash2] = hash_pair(data);
  for (size_t idx : std::views::iota(size_t {0}, m_khashes)) {
    size_t bit_idx = (hash1 + idx * hash2) % m_bits;
    m_bitset[bit_idx] = true;
  }
}

auto bloom_filter::inspect(bool full) -> std::string
{
  std::string bit_str;
  bit_str.reserve(this->m_bitset.size());
  for (bool bit : this->m_bitset) {
    bit_str.push_back(bit ? '1' : '0');
  }

  if (full) {
    return std::format("bits: {}, n_hashes: {}, bitset: {}",
                       this->m_bits,
                       this->m_khashes,
                       bit_str);
  }
  return std::format("bits: {}, n_hashes: {}, bitset: {}",
                     this->m_bits,
                     this->m_khashes,
                     this->m_bitset.size());
}
