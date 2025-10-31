#include <bit>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numbers>
#include <optional>
#include <type_traits>
#include <vector>

#include <cxxopts.hpp>

#include "bloom.hpp"

namespace
{
template<typename T>
auto byteswap(T value) -> T
{
  const size_t eight = 8;
  const size_t bit_mask = 0xFF;
  static_assert(std::is_integral_v<T>, "Only works with integral types");
  auto uvalue = static_cast<std::make_unsigned_t<T>>(value);
  T result = 0;
  auto uresult = static_cast<std::make_unsigned_t<T>>(result);
  for (size_t i = 0; i < sizeof(T); ++i) {
    uresult |= ((uvalue >> (eight * i)) & bit_mask)
        << (eight * (sizeof(T) - 1 - i));
  }
  return uresult;
}

template<typename T>
auto to_big_endian(T value) -> T
{
#if __cpp_lib_endian
  if constexpr (std::endian::little == std::endian::native) {
    return byteswap(value);
  }
#endif
  return value;
}

auto parse_file(std::string& file_name) -> std::vector<std::string>
{
  std::vector<std::string> contents;

  std::ifstream file(file_name);
  if (!file) {
    std::cerr << "Error: could not open file " << file_name << '\n';
    return contents;
  }

  std::string line;
  while (std::getline(file, line)) {
    contents.push_back(line);
  }

  file.close();
  return contents;
}

auto write_binary_file(const std::string& file_name, bloom_filter& filter)
{
  std::ofstream file(file_name, std::ios::binary);
  if (!file) {
    std::cerr << "Error: could not write file: " << file_name << '\n';
    return;
  }

  std::string identifier = "CCBF";
  auto len = static_cast<uint32_t>(identifier.size());
  file.write(identifier.data(), len);

  std::array<char, sizeof(uint32_t)> buffer_list {};
  const auto version_num = to_big_endian<uint16_t>(1);
  std::memcpy(buffer_list.data(), &version_num, sizeof(uint16_t));
  file.write(buffer_list.data(), sizeof(uint16_t));

  const auto hash_functions = to_big_endian<uint16_t>(filter.get_khashes());
  std::memcpy(buffer_list.data(), &hash_functions, sizeof(uint16_t));
  file.write(buffer_list.data(), sizeof(uint16_t));

  const auto bits = to_big_endian<uint32_t>(filter.get_bits());
  std::memcpy(buffer_list.data(), &bits, sizeof(uint32_t));
  file.write(buffer_list.data(), sizeof(uint32_t));

  // std::cout << "\tversion_num: " << version_num << '\n';
  // std::cout << "\thash_functions: " << hash_functions << '\n';
  // std::cout << "\tbits: " << bits << '\n';

  uint8_t buffer = 0;
  size_t bit_count = 0;
  const size_t bits_per_byte = 8;

  for (bool bit : filter.get_bitset()) {
    buffer = (static_cast<unsigned>(buffer) << 1U) | (bit ? 1U : 0U);
    ++bit_count;

    if (bit_count == bits_per_byte) {
      file.put(static_cast<char>(buffer));
      buffer = 0;
      bit_count = 0;
    }
  }

  if (bit_count > 0) {
    buffer <<= (sizeof(uint8_t) - bit_count);
    file.put(static_cast<char>(buffer));
  }

  file.close();
}

auto read_binary_file(const std::string& file_name)
    -> std::optional<bloom_filter>
{
  std::ifstream file(file_name, std::ios::binary);
  if (!file) {
    std::cerr << "Could not read file: " << file_name << '\n';
    return std::nullopt;
  }

  std::string expected_identifier = "CCBF";
  auto len = static_cast<uint32_t>(expected_identifier.size());
  std::string identifier(len, '\0');
  file.read(identifier.data(), len);
  assert(identifier == expected_identifier);

  std::array<char, sizeof(uint32_t)> buffer_list {};
  file.read(buffer_list.data(), sizeof(uint16_t));
  uint16_t version_number_buff = 0;
  std::memcpy(&version_number_buff, buffer_list.data(), sizeof(uint16_t));
  auto version_number = to_big_endian<uint16_t>(version_number_buff);

  file.read(buffer_list.data(), sizeof(uint16_t));
  uint16_t hash_functions_buff = 0;
  std::memcpy(&hash_functions_buff, buffer_list.data(), sizeof(uint16_t));
  auto hash_functions = to_big_endian<uint16_t>(hash_functions_buff);

  file.read(buffer_list.data(), sizeof(uint32_t));
  uint32_t bits_buff = 0;
  std::memcpy(&bits_buff, buffer_list.data(), sizeof(uint32_t));
  auto bits = to_big_endian<uint32_t>(bits_buff);

  // std::cout << "\tversion_num: " << version_number << '\n';
  // std::cout << "\thash_functions: " << hash_functions << '\n';
  // std::cout << "\tbits: " << bits << '\n';

  uint8_t buffer = 0;
  size_t bit_count = 0;
  const size_t bits_per_byte = 8;

  const size_t windows =
      (bits / bits_per_byte) + (bits % bits_per_byte != 0 ? 1 : 0);
  std::vector<bool> bitset;
  bitset.reserve(bits);
  std::array<char, sizeof(uint8_t)> byte_buff {};

  for (size_t byte_idx = 0; byte_idx < windows; ++byte_idx) {
    uint8_t byte = 0;
    file.read(byte_buff.data(), sizeof(uint8_t));
    std::memcpy(&byte, byte_buff.data(), sizeof(uint8_t));

    for (size_t bit_idx = 0; bit_idx < bits_per_byte && bitset.size() < bits;
         ++bit_idx)
    {
      // bool bit = (byte >> bit_idx) & 1;
      bool bit = ((static_cast<unsigned>(byte) >> bit_idx) & 1U) != 0;
      bitset.push_back(bit);
    }
  }

  bloom_filter filter(bits, bitset, hash_functions);
  file.close();
  return filter;
}

auto insert_into_filter(bloom_filter& filter,
                        std::vector<std::string>& contents)
{
  for (const auto& input : contents) {
    filter.insert(input);
  }
}
}  // namespace

const double false_positive_rate = 0.01;

auto main(int argc, char* argv[]) -> int
{
  cxxopts::Options options("Bloomers", "Spell checker using bloom filter");
  options.add_options()(
      "f,file", "dictionary file", cxxopts::value<std::string>());
  options.add_options()("h,help", "print help");
  auto result = options.parse(argc, argv);

  if (result.contains("help")) {
    std::cout << options.help() << '\n';
    return 0;
  }

  if (!result.contains("file")) {
    std::cout << "'file' must be specified\n";
    return 0;
  }

  // Read in the file contents
  std::string file = result["file"].as<std::string>();
  auto file_contents = parse_file(file);

  // Insert into the bloom filter
  bloom_filter filter(bloom_filter::expected_items {file_contents.size()},
                      bloom_filter::false_positive_rate {false_positive_rate});
  insert_into_filter(filter, file_contents);

  const bool inspect_full = false;
  // std::cout << filter.inspect(inspect_full) << '\n';

  // Write to a file
  const std::string output_file_name = "words.bf";
  if (std::filesystem::exists(output_file_name)) {
    std::cout << "Reading file: " << output_file_name << '\n';
    read_binary_file(output_file_name);
  } else {
    std::cout << "Writing file: " << output_file_name << '\n';
    write_binary_file(output_file_name, filter);
  }

  auto words = result.unmatched();
  std::vector<std::string> spelled_wrong;
  for (const auto& test_word : words) {
    if (!filter.possibly_contains(test_word)) {
      spelled_wrong.push_back(test_word);
    }
  }

  if (spelled_wrong.size() > 0) {
    std::cout << "These words are spelt wrong:\n";
    for (const auto& word : spelled_wrong) {
      std::cout << "  - " << word << '\n';
    }
  } else {
    std::cout << "All words spelt correctly\n";
  }

  return 0;
}
