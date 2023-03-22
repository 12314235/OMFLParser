#pragma once

#include <filesystem>
#include <istream>
#include <vector>
#include <variant>
#include <string_view>


namespace omfl {

	class OMFLParsed {
	public:

		struct key {
			std::string key_name = "";

			std::string string_value = "";
			int32_t int_value = 0;
			float float_value = 0.0f;
			bool bool_value = false;

			std::vector<key> array_value;

			bool is_int = false;
			bool is_float = false;
			bool is_string = false;
			bool is_bool = false;
			bool is_array = false;

			bool IsString() {
				return is_string;
			}

			bool IsFloat() {
				return is_float;
			}

			bool IsArray() {
				return is_array;
			}

			bool IsBool() {
				return is_bool;
			}

			bool IsInt() {
				return is_int;
			}

			int32_t AsInt() {
				return int_value;
			}

			const std::string& AsString() {
				return string_value;
			}

			bool AsBool() {
				return bool_value;
			}

			float AsFloat() {
				return float_value;
			}

			std::string AsStringOrDefault(const std::string& default) {
				return is_string ? string_value : default;
			}

			float AsFloatOrDefault(float default) {
				return is_float ? float_value : default;
			}

			int32_t AsIntOrDefault(int32_t default) {
				return is_int ? int_value : default;
			}

			OMFLParsed::key& operator[](size_t index);
		};

		struct section {
			std::string section_name = "";
			section* parent_section = nullptr;

			std::vector<key> keys;
			std::vector<section> subsections;
		};

		bool is_valid = true;

		std::vector<std::variant<section, key>> Storage;

		bool valid() const;
		key Get(const std::string& key_name) const;
	};

	struct state_flags {
		bool is_string = false;
		bool is_in_section = false;
		bool is_comment = false;
		bool end_of_key = false;
		bool is_key = false;
		bool is_array = false;
	};

	static OMFLParsed::key void_key;

	OMFLParsed parse(const std::filesystem::path& path);
	OMFLParsed parse(const std::string& str);

	bool ParseSection(std::string_view section, OMFLParsed::section& new_section);
	bool ParseKey(std::string_view key, OMFLParsed::key& new_key);

	bool ParseArray(std::string_view value, OMFLParsed::key& key);
	bool ParseString(std::string_view value, OMFLParsed::key& key, bool is_array);
	bool ParseInt(std::string_view value, OMFLParsed::key& key);

	OMFLParsed::section* FindSection(const std::string& section_name, OMFLParsed& parsed);
	OMFLParsed::section* SubsectionSearch(OMFLParsed::section* section, const std::string& section_name);

}