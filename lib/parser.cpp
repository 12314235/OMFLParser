#include "parser.h"
#include <string_view>
#include <string>
#include <fstream>
#include <cstring>

namespace omfl {
	void character_processing(state_flags& flags, OMFLParsed& parsed, char s, std::string& buff, std::string& current_section_name, bool is_end_of_str) {
		if (s == '\n' || is_end_of_str) {
			if (is_end_of_str) buff += s;
			flags.end_of_key = false;
			flags.is_array = false;
			flags.is_string = false;
			flags.is_comment = false;
			if (buff[0] == '[' && !flags.is_in_section) {
				OMFLParsed::section new_section;
				std::string_view val{ buff };
				parsed.is_valid = ParseSection(val.substr(1, buff.size() - 1), new_section);
				if (FindSection(buff, parsed) == nullptr) {
					parsed.Storage.push_back(new_section);
				}
				flags.is_in_section = true;
				current_section_name = buff;
			}
			else if (buff == "") {
				current_section_name = "";
				flags.is_in_section = false;
			}
			else {
				OMFLParsed::key new_key;
				OMFLParsed::section new_section;
				std::string_view val{ buff };
				parsed.is_valid = ParseKey(val, new_key);
				if (flags.is_in_section) {
					OMFLParsed::section* current_section = FindSection(current_section_name, parsed);
					if (current_section == nullptr) {
						return;
					}
					current_section->keys.push_back(new_key);
				}
				else {
					parsed.Storage.push_back(new_key);
				}
			}
			buff = "";
		}
		else if (s == '"' && !flags.is_comment) {
			buff += s;
			if (flags.end_of_key) parsed.is_valid = false;
			if (flags.is_string) flags.end_of_key = true && !flags.is_array;
			flags.is_string = !flags.is_string;
		}
		else if (s == '=') {
			flags.is_key = true;
			buff += s;
		}
		else if (flags.is_key && !flags.is_string && s == '[') {
			buff += s;
			flags.is_array = true;
		}
		else if (flags.end_of_key && s != ' ') {
			parsed.is_valid = false;
			return;
		}
		else if (s == '#' && !flags.is_string) {
			flags.is_comment = true;
		}
		else if (!flags.is_comment) {
			if (s == ' ' && !flags.is_string) {
				return;
			}
			buff += s;
		}
	}


	OMFLParsed parse(const std::filesystem::path& path)
	{
		OMFLParsed parsed;

		state_flags flags;

		std::string buff;
		std::string current_section_name;

		std::ifstream file;
		file.open(path, std::ios::binary);

		char s;

		while (file.get(s)) {
			character_processing(flags, parsed, s, buff, current_section_name, false);
		}

		return parsed;
	}

	OMFLParsed parse(const std::string& str)
	{
		OMFLParsed parsed;

		state_flags flags;

		std::string buff;
		std::string current_section_name;

		for (int i = 0; i < str.size(); i++) {
			character_processing(flags, parsed, str[i], buff, current_section_name, i == str.size() - 1);
		}

		return parsed;
	}

	bool is_alpha(char x)
	{
		return (x >= 'A' && x <= 'Z') || (x >= 'a' && x <= 'z');
	}

	bool ParseSection(std::string_view section, OMFLParsed::section& new_section)
	{
		bool is_valid = true;

		std::string buff = "";

		if (section.size() == 1) buff = section;
		if (section.size() == 0) return true;

		for (int i = 0; i < section.size(); i++) {
			if (section[i] == '.' || i == section.size() - 1) {
				if (i == section.size() - 1) {
					buff += section[i];
				}
				for (int j = 0; j < buff.size(); j++) {
					if (!(isdigit(buff[j]) || is_alpha(buff[j]) || buff[j] == '-' || buff[j] == '_')) {
						return false;
					}
				}
				new_section.section_name = buff;
				if (i < section.size() - 1) {
					OMFLParsed::section subsection;
					subsection.parent_section = &new_section;
					is_valid = ParseSection(section.substr(i + 1, section.size() - i - 1), subsection);
					new_section.subsections.push_back(subsection);
				}
				break;
			}
			else {
				buff += section[i];
			}
		}

		return is_valid;
	}

	bool ParseKey(std::string_view key, OMFLParsed::key& new_key)
	{
		int counter = 0;
		while (key[counter] != '=') {
			if (!(isdigit(key[counter]) || is_alpha(key[counter]) || key[counter] == '-' || key[counter] == '_')) {
				return false;
			}
			new_key.key_name += key[counter];
			counter++;
		}

		counter++;

		key = key.substr(counter, key.size() - counter);

		if (key.size() == 0 || counter == 1) {
			return false;
		}

		if (key[0] == '"') {
			new_key.is_string = true;
			return ParseString(key, new_key, false);
		}
		else if (key[0] == '[') {
			new_key.is_array = true;
			return ParseArray(key, new_key);
		}
		else if (key == "true" || key == "false") {
			new_key.is_bool = true;
			new_key.bool_value = key == "true";
			return true;
		}
		else {
			return ParseInt(key, new_key);
		}

		return false;;
	}

	bool ParseArray(std::string_view value, OMFLParsed::key& key)
	{
		std::string buff = "";
		bool is_valid = true;
		bool is_string = false;
		bool is_float = false;

		if (value[0] != '[' || value[value.size() - 1] != ']') {
			return false;
		}

		if (value.size() == 0) {
			return true;
		}

		for (int i = 1; i < value.size() && is_valid; i++) {
			if (value[i] == ',' || i == value.size() - 1) {
				if (buff == "") {
					continue;
				}
				if (value[i] != ' ' && value[i] != ',' && value[i] != ']') {
					buff += value[i];
				}
				if (buff[0] == '"') {
					is_valid = ParseString(buff, key, true);
				}
				else if (buff == "true" || buff == "false") {
					OMFLParsed::key buff_key;
					buff_key.is_bool = true;
					buff_key.bool_value = buff == "true";
					key.array_value.push_back(buff_key);
				}
				else {
					is_valid = ParseInt(buff, key);
				}
				buff = "";
			}
			else if (value[i] == '"') {
				is_string = !is_string;
				buff += value[i];
			}
			else if (!is_string && value[i] == '[') {
				OMFLParsed::key buff_key;
				buff_key.is_array = true;
				buff += value[i];
				i++;
				while (i < value.size() - 1 && value[i] != ']') {
					buff += value[i];
					i++;
				}

				if (i == value.size() - 2 && value[i] != ']') {
					return false;
				}
				else {
					buff += value[i];
					is_valid = ParseArray(buff, buff_key);
					key.array_value.push_back(buff_key);
				}
				buff = "";
			}
			else {
				if (value[i] == ' ' && !is_string) {
					continue;
				}
				buff += value[i];
			}
		}

		return is_valid;
	}

	bool ParseString(std::string_view value, OMFLParsed::key& key, bool is_array)
	{
		if (value[0] != '"' || value[value.size() - 1] != '"') {
			return false;
		}
		value = value.substr(1, value.size() - 2);
		if (is_array) {
			OMFLParsed::key sub_key;
			sub_key.is_string = true;
			sub_key.string_value = value;
			key.array_value.push_back(sub_key);
		}
		else {
			key.string_value = value;
		}

		return true;
	}

	bool ParseInt(std::string_view key, OMFLParsed::key& new_key)
	{
		for (int i = 0; i < key.size(); i++) {
			if (!isdigit(key[i]) && key[i] != '.' && key[i] != '+' && key[i] != '-') {
				return false;
			}
			else if ((key[i] == '+' || key[i] == '-') && (i != 0 || i == key.size() - 1)) {
				return false;
			}
			else if ((key[i] == '.' && (i == 0 || i == key.size() - 1)) || (key[i] == '.' && !isdigit(key[i - 1]))) {
				return false;
			}
			else if (key[i] == '.' && !new_key.is_float) {
				new_key.is_float = true;
			}
			else if (key[i] == '.' && new_key.is_float) {
				return false;
			}
		}

		if (new_key.is_float) {
			if (new_key.IsArray()) {
				OMFLParsed::key sub_key;
				sub_key.is_float = true;
				sub_key.float_value = std::stof(key.data());
				new_key.array_value.push_back(sub_key);
			}
			else {
				new_key.float_value = std::stof(key.data());
			}
			return true;
		}
		else {
			if (new_key.IsArray()) {
				OMFLParsed::key sub_key;
				sub_key.is_int = true;
				sub_key.int_value = std::stoi(key.data());
				new_key.array_value.push_back(sub_key);
			}
			else {
				new_key.is_int = true;
				new_key.int_value = std::stoll(key.data());
			}
			return true;
		}
	}

	OMFLParsed::section* FindSection(const std::string& section_name, OMFLParsed& parsed)
	{
		std::string buff = "";

		for (int i = 0; i < section_name.size(); i++) {
			if (section_name[i] != '.' && i != section_name.size() - 1) {
				buff += section_name[i];
			}
			else {
				if (i == section_name.size() - 1) {
					buff += section_name[i];
				}
				for (int j = 0; j < parsed.Storage.size(); j++) {
					OMFLParsed::section* buff_section = std::get_if<OMFLParsed::section>(&parsed.Storage[j]);
					if (buff_section == nullptr) continue;
					else if (buff_section->section_name == buff) {
						if (i != section_name.size() - 1) {
							return SubsectionSearch(buff_section, section_name.substr(i + 1, section_name.size() - i));
						}
						else {
							return buff_section;
						}
					}
				}
				buff = "";
			}
		}
		return nullptr;
	}

	OMFLParsed::section* SubsectionSearch(OMFLParsed::section* section, const std::string& section_name) {
		std::string buff = "";

		for (int i = 0; i < section_name.size(); i++) {
			if (section_name[i] != '.' && i != section_name.size() - 1) {
				buff += section_name[i];
			}
			else {
				if (i == section_name.size() - 1) {
					buff += section_name[i];
				}
				for (int j = 0; j < section->subsections.size(); j++) {
					if (section->subsections[j].section_name == buff) {
						if (i != section_name.size() - 1) {
							return SubsectionSearch(&section->subsections[j], section_name.substr(i + 1, section_name.size() - i));
						}
						else {
							return &section->subsections[j];
						}
					}
				}
				buff = "";
			}
		}

		return nullptr;
	}

	bool OMFLParsed::valid() const
	{
		return is_valid;
	}

	std::string GetKeyName(const std::string& da) {
		std::string buff = "";
		for (auto i : da) {
			if (i == '.') {
				buff = "";
			}
			else {
				buff += i;
			}
		}

		return buff;
	}

	OMFLParsed::key OMFLParsed::Get(const std::string& key_name) const
	{
		const section* section_buff;
		const key* key_buff;

		std::string buff;

		std::string key = GetKeyName(key_name);

		for (int i = 0; i < key_name.size(); i++) {
			if (key_name[i] != '.' && i != key_name.size() - 1) {
				buff += key_name[i];
			}
			else {
				if (i == key_name.size() - 1) {
					buff += key_name[i];
				}
				for (int j = 0; j < Storage.size(); j++) {
					section_buff = std::get_if<OMFLParsed::section>(&Storage[j]);
					key_buff = std::get_if<OMFLParsed::key>(&Storage[j]);
					if (key_buff != nullptr && key_buff->key_name == buff) {
						return *key_buff;
					}
					else if (section_buff == nullptr) continue;
					else if (section_buff->section_name == buff) {
						if (i != key_name.size() - 1) {
							buff = "";
							for (int k = i + 1; k < key_name.size() && section_buff->subsections.size() != 0; k++) {
								if (key_name[k] != '.' && k != key_name.size() - 1) {
									buff += key_name[k];
								}
								else {
									for (int h = 0; h < section_buff->subsections.size(); h++) {
										if (section_buff->subsections[h].section_name == buff) {
											section_buff = &section_buff->subsections[h];
										}
									}
									buff = "";
								}
							}
							for (int z = 0; z < section_buff->keys.size(); z++) {
								if (section_buff->keys[z].key_name == key) {
									return section_buff->keys[z];
								}
							}
						}
						else {
							return void_key;
						}
					}
				}
				buff = "";
			}
		}
	}

	OMFLParsed::key& OMFLParsed::key::operator[](size_t index)
	{
		if (IsArray()) {
			if (index <= array_value.size()) {
				return array_value[index];
			}

			return void_key;
		}
		else {
			return void_key;
		}

		return void_key;
	}
}