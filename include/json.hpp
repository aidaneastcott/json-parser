
#pragma once
#ifndef JSON_HPP
#define JSON_HPP


#include "base.hpp"


#include <cctype>

#include <iterator>
#include <optional>
#include <string>
#include <string_view>


namespace json {


class io {
private:
	template <typename Iterator>
	static void write_value(Iterator output, const value &argument) {

		if (argument.type() == tag::null) {

			auto value = std::string_view{"null"};
			std::copy(std::begin(value), std::end(value), output);
			return;
		}

		class {
		private:
			Iterator output;

		public:
			void operator()(const object &argument) {
				write_object(output, argument);
			}

			void operator()(const array &argument) {
				write_array(output, argument);
			}

			void operator()(const number &argument) {
				write_number(output, argument);
			}

			void operator()(const string &argument) {
				write_string(output, argument);
			}

			void operator()(const boolean &argument) {
				write_boolean(output, argument);
			}

		} functor{output};

		return argument.visit(functor);
	}

	template <typename Iterator>
	static void write_object(Iterator output, const object &argument) {

		auto add_character = [&output](char value) {
			*output = value;
			++output;
		};

		auto result = std::string{};

		add_character('{');

		auto first = true;

		for (const auto &[key, value] : argument) {

			if (!first) {
				add_character(',');
			}

			add_character('"');
			std::copy(key.begin(), key.end(), output);
			add_character('\"');
			add_character(':');

			write_value(output, value);

			first = false;
		}

		add_character('}');
	}

	template <typename Iterator>
	static void write_array(Iterator output, const array &argument) {

		auto add_character = [&output](char value) {
			*output = value;
			++output;
		};

		add_character('[');

		auto first = true;

		for (const auto &value : argument) {

			if (!first) {
				add_character(',');
			}

			write_value(output, value);

			first = false;
		}

		add_character(']');
	}

	template <typename Iterator>
	static void write_number(Iterator output, const number &argument) {
		auto value = argument.visit([](auto value) {
			return std::to_string(value);
		});

		if (value.find('.') != std::string::npos) {

			while (value.back() == '0') {
				value.pop_back();
			}

			if (value.back() == '.') {
				value.pop_back();
			}
		}

		std::copy(value.begin(), value.end(), output);
	}

	template <typename Iterator>
	static void write_string(Iterator output, const string &argument) {

		auto add_character = [&output](char value) {
			*output = value;
			++output;
		};

		add_character('"');

		for (const auto &value : argument.get()) {

			if (value == '"') {
				add_character('\\');
				add_character('\"');
			}
			else if (value == '\\') {
				add_character('\\');
				add_character('\\');
			}
			else if (value == '/') {
				add_character('\\');
				add_character('/');
			}
			else if (value == '\b') {
				add_character('\\');
				add_character('b');
			}
			else if (value == '\f') {
				add_character('\\');
				add_character('f');
			}
			else if (value == '\n') {
				add_character('\\');
				add_character('n');
			}
			else if (value == '\r') {
				add_character('\\');
				add_character('r');
			}
			else if (value == '\t') {
				add_character('\\');
				add_character('t');
			}
			else {
				add_character(value);
			}
		}

		add_character('"');
	}

	template <typename Iterator>
	static void write_boolean(Iterator output, const boolean &argument) {

		auto value = std::string_view{argument.get() ? "true" : "false"};
		std::copy(std::begin(value), std::end(value), output);
	}

	template <typename Iterator>
	static void read_space(Iterator &begin, Iterator end) noexcept {

		while (begin != end && std::isspace(*begin)) {
			++begin;
		}
	}

	template <typename Iterator>
	[[nodiscard]] static bool read_token(char token, Iterator &begin, Iterator end) noexcept {

		if (begin == end || *begin != token) {
			return false;
		}

		++begin;
		return true;
	}

	template <typename Iterator>
	[[nodiscard]] static bool read_null(Iterator &begin, Iterator end) noexcept {

		if (begin == end) {
			return false;
		}

		auto iterator = begin;

		if (read_token('n', iterator, end) && read_token('u', iterator, end)
		    && read_token('l', iterator, end) && read_token('l', iterator, end)) {

			begin = iterator;

			return true;
		}

		return false;
	}

	template <typename Iterator>
	[[nodiscard]] static std::optional<std::string> read_string(Iterator &begin, Iterator end) {

		if (begin == end) {
			return std::nullopt;
		}

		auto iterator = begin;

		if (!read_token('"', iterator, end)) {
			return std::nullopt;
		}

		auto result = std::string{};

		while (iterator != end && *iterator != '"') {

			if (!read_token('\\', iterator, end)) {

				result += *iterator;
				++iterator;
				continue;
			}

			if (read_token('"', iterator, end)) {
				result += '"';
			}
			else if (read_token('\\', iterator, end)) {
				result += '\\';
			}
			else if (read_token('/', iterator, end)) {
				result += '/';
			}
			else if (read_token('b', iterator, end)) {
				result += '\b';
			}
			else if (read_token('f', iterator, end)) {
				result += '\f';
			}
			else if (read_token('n', iterator, end)) {
				result += '\n';
			}
			else if (read_token('r', iterator, end)) {
				result += '\r';
			}
			else if (read_token('t', iterator, end)) {
				result += '\t';
			}
			else if (read_token('u', iterator, end)) {

				for (auto i = 0; i < 4; ++i, ++iterator) {
					if (iterator == end || !std::isxdigit(*iterator)) {
						return std::nullopt;
					}

					result += *iterator;
				}
			}
			else {
				return std::nullopt;
			}
		}

		if (iterator == end) {
			return std::nullopt;
		}

		if (!read_token('"', iterator, end)) {
			return std::nullopt;
		}

		begin = iterator;

		return std::make_optional(std::move(result));
	}

	template <typename Iterator>
	[[nodiscard]] static std::optional<object> read_object(Iterator &begin, Iterator end) {

		if (begin == end) {
			return std::nullopt;
		}

		auto iterator = begin;

		if (!read_token('{', iterator, end)) {
			return std::nullopt;
		}

		auto result = object{};

		while (true) {

			read_space(iterator, end);

			auto optional_key = read_string(iterator, end);

			if (!optional_key) {
				return std::nullopt;
			}

			read_space(iterator, end);

			if (!read_token(':', iterator, end)) {
				return std::nullopt;
			}

			read_space(iterator, end);

			if (read_null(iterator, end)) {
				result.add(std::move(*optional_key), value::null);
			}
			else if (auto optional_object = read_object(iterator, end); optional_object) {
				result.add(std::move(*optional_key), std::move(*optional_object));
			}
			else if (auto optional_array = read_array(iterator, end); optional_array) {
				result.add(std::move(*optional_key), std::move(*optional_array));
			}
			else if (auto optional_number = read_number(iterator, end); optional_number) {
				result.add(std::move(*optional_key), std::move(*optional_number));
			}
			else if (auto optional_string = read_string(iterator, end); optional_string) {
				result.add(std::move(*optional_key), std::move(*optional_string));
			}
			else if (auto optional_boolean = read_boolean(iterator, end); optional_boolean) {
				result.add(std::move(*optional_key), std::move(*optional_boolean));
			}
			else {

				if (read_token('}', iterator, end)) {
					break;
				}

				return std::nullopt;
			}

			read_space(iterator, end);

			if (read_token(',', iterator, end)) {
				continue;
			}

			if (read_token('}', iterator, end)) {
				break;
			}

			return std::nullopt;
		}

		begin = iterator;

		return std::make_optional(std::move(result));
	}

	template <typename Iterator>
	[[nodiscard]] static std::optional<array> read_array(Iterator &begin, Iterator end) {

		if (begin == end) {
			return std::nullopt;
		}

		auto iterator = begin;

		if (!read_token('[', iterator, end)) {
			return std::nullopt;
		}

		auto result = array{};

		while (true) {

			read_space(iterator, end);

			if (read_null(iterator, end)) {
				result.add(value::null);
			}
			else if (auto optional_object = read_object(iterator, end); optional_object) {
				result.add(std::move(*optional_object));
			}
			else if (auto optional_array = read_array(iterator, end); optional_array) {
				result.add(std::move(*optional_array));
			}
			else if (auto optional_number = read_number(iterator, end); optional_number) {
				result.add(std::move(*optional_number));
			}
			else if (auto optional_string = read_string(iterator, end); optional_string) {
				result.add(std::move(*optional_string));
			}
			else if (auto optional_boolean = read_boolean(iterator, end); optional_boolean) {
				result.add(std::move(*optional_boolean));
			}
			else {

				if (read_token(']', iterator, end)) {
					break;
				}

				return std::nullopt;
			}

			read_space(iterator, end);

			if (read_token(',', iterator, end)) {
				continue;
			}

			if (read_token(']', iterator, end)) {
				break;
			}

			return std::nullopt;
		}

		begin = iterator;

		return std::make_optional(std::move(result));
	}

	template <typename Iterator>
	[[nodiscard]] static std::optional<number> read_number(Iterator &begin, Iterator end) {

		if (begin == end) {
			return std::nullopt;
		}

		auto iterator = begin;

		auto is_negative = read_token('-', iterator, end);

		if (iterator == end || !std::isdigit(*iterator)) {
			return std::nullopt;
		}

		while (iterator != end && std::isdigit(*iterator)) {
			++iterator;
		}

		auto is_decimal = read_token('.', iterator, end);

		if (is_decimal) {

			while (iterator != end || std::isdigit(*iterator)) {
				++iterator;
			}
		}

		auto value = std::string{begin, iterator};

		begin = iterator;

		if (is_decimal) {
			return std::make_optional(number{std::stold(value)});
		}

		if (is_negative) {
			return std::make_optional(number{std::stoll(value)});
		}

		return std::make_optional(number{std::stoull(value)});
	}

	template <typename Iterator>
	[[nodiscard]] static std::optional<boolean> read_boolean(Iterator &begin,
	                                                         Iterator end) noexcept {

		if (begin == end) {
			return std::nullopt;
		}

		auto iterator = begin;

		if (read_token('t', iterator, end) && read_token('r', iterator, end)
		    && read_token('u', iterator, end) && read_token('e', iterator, end)) {

			begin = iterator;

			return std::make_optional(boolean{true});
		}

		iterator = begin;

		if (read_token('f', iterator, end) && read_token('a', iterator, end)
		    && read_token('l', iterator, end) && read_token('s', iterator, end)
		    && read_token('e', iterator, end)) {

			begin = iterator;

			return std::make_optional(boolean{false});
		}

		return std::nullopt;
	}

public:
	template <
	    typename Iterator, typename Type,
	    std::enable_if_t<std::is_same_v<value, implementation::remove_cvref_t<Type>>, int> = 0>
	static void write(Iterator output, const Type &argument) {
		write_value(output, argument);
	}

	template <
	    typename Iterator, typename Type,
	    std::enable_if_t<std::is_same_v<object, implementation::remove_cvref_t<Type>>, int> = 0>
	static void write(Iterator output, const Type &argument) {
		write_object(output, argument);
	}

	template <
	    typename Iterator, typename Type,
	    std::enable_if_t<std::is_same_v<array, implementation::remove_cvref_t<Type>>, int> = 0>
	static void write(Iterator output, const Type &argument) {
		write_array(output, argument);
	}

	template <
	    typename Iterator, typename Type,
	    std::enable_if_t<std::is_same_v<number, implementation::remove_cvref_t<Type>>, int> = 0>
	static void write(Iterator output, const Type &argument) {
		write_number(output, argument);
	}

	template <
	    typename Iterator, typename Type,
	    std::enable_if_t<std::is_same_v<string, implementation::remove_cvref_t<Type>>, int> = 0>
	static void write(Iterator output, const Type &argument) {
		write_string(output, argument);
	}

	template <
	    typename Iterator, typename Type,
	    std::enable_if_t<std::is_same_v<boolean, implementation::remove_cvref_t<Type>>, int> = 0>
	static void write(Iterator output, const Type &argument) {
		write_boolean(output, argument);
	}

	template <typename Type>
	static std::string write(const Type &argument) {

		auto result = std::string{};

		write(std::back_inserter(result), argument);

		return result;
	}

	template <typename Iterator>
	[[nodiscard]] static std::optional<value> read(Iterator begin, Iterator end) {

		read_space(begin, end);

		auto result = value{};

		if (read_null(begin, end)) {
			result.set(value::null);
		}
		else if (auto optional_object = read_object(begin, end); optional_object) {
			result.set(std::move(*optional_object));
		}
		else if (auto optional_array = read_array(begin, end); optional_array) {
			result.set(std::move(*optional_array));
		}
		else if (auto optional_number = read_number(begin, end); optional_number) {
			result.set(std::move(*optional_number));
		}
		else if (auto optional_string = read_string(begin, end); optional_string) {
			result.set(std::move(*optional_string));
		}
		else if (auto optional_boolean = read_boolean(begin, end); optional_boolean) {
			result.set(std::move(*optional_boolean));
		}
		else {
			return std::nullopt;
		}

		read_space(begin, end);

		if (begin != end) {
			return std::nullopt;
		}

		return std::make_optional(std::move(result));
	}

	[[nodiscard]] static std::optional<value> read(std::string_view argument) {
		return read(argument.begin(), argument.end());
	}
};


} // namespace json


#endif // JSON_HPP
