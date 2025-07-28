#pragma once

#include "havoc.hpp"
#include "peglib.h"

#include <fstream>
#include <variant>

namespace gd
{
	struct constructable;
	struct value;

	using dictionary_t = std::unordered_map<std::string, value>;
	using array_t = std::vector<value>;
	using numeric_t = std::variant<float, int>;

	using value_t = havoc::one_of<constructable, dictionary_t, array_t, bool, std::string, numeric_t>;

	struct value : value_t
	{
		using value_t::one_of;
		using value_t::operator=;
	};

	struct field
	{
		std::string name;
		struct value value;
	};

	namespace detail
	{
		struct fields
		{
			std::vector<field> fields;
		};

		struct assignments
		{
			std::vector<field> fields;
		};
	}

	struct tag
	{
		std::string identifier;
		std::vector<field> fields;
		std::vector<field> assignments;
	};

	struct constructable
	{
		std::string identifier;
		std::vector<value> arguments;
	};

	struct file
	{
		std::vector<tag> tags;
	};

	gd::file parse(std::istream& stream)
	{
		constexpr auto grammar = R"(
File <- Tag+

Tag <- '[' Identifier Fields ']' Assignments?

Fields <- Field*
Assignments <- Field+

Field <- Identifier '=' Value
Property <- String ':' Value

Value <- Constructable / Dictionary / Array / Boolean / String / Numeric

Numeric <- Float / Integer

Integer <- <'-'? Number>
Float <- <Integer '.' Number ('e' Integer)?>
String <- '"' <[^"]*> '"'
Array <- '[' List(Value) ']'
Dictionary <- '{' List(Property) '}'
Constructable <- Identifier '(' List(Value) ')'

Number <- [0-9]+
Identifier <- <[a-zA-Z.:_0-9]+>
Boolean <- 'true' | 'false'

List(T) <- (T (',' T)*)?

%whitespace <- [ \t\n\r]*
    )";

		peg::parser parser(grammar);

		parser["File"] = [](peg::SemanticValues values) {
			gd::file file;

			std::ranges::transform(values, back_inserter(file.tags), [](auto value) {
				return std::any_cast<gd::tag>(value);
			});

			return file;
		};

		parser["Fields"] = [](peg::SemanticValues values) -> detail::fields {
			detail::fields fields;

			std::transform(begin(values), end(values), back_inserter(fields.fields), [](auto value) {
				return std::any_cast<gd::field>(value);
			});

			return fields;
		};

		parser["Assignments"] = [](peg::SemanticValues values) -> detail::assignments {
			detail::assignments assignments;

			std::transform(begin(values), end(values), back_inserter(assignments.fields), [](auto value) {
				return std::any_cast<gd::field>(value);
			});

			return assignments;
		};

		parser["Tag"] = [](peg::SemanticValues values) {
			gd::tag tag {
				.identifier = std::any_cast<std::string>(values[0]),
			};

			for (auto i = 1; i < values.size(); i++)
			{
				if (auto fields = std::any_cast<detail::fields>(&values[i]))
				{
					tag.fields = fields->fields;
				}

				if (auto assignments = std::any_cast<detail::assignments>(&values[i]))
				{
					tag.assignments = assignments->fields;
				}
			}

			return tag;
		};

		parser["Constructable"] = [](peg::SemanticValues values) -> gd::constructable {
			gd::constructable constructable {
				.identifier = std::any_cast<std::string>(values[0]),
			};

			std::transform(begin(values) + 1, end(values), back_inserter(constructable.arguments), [](auto value) {
				return std::any_cast<gd::value>(value);
			});

			return constructable;
		};

		parser["Property"] = [](peg::SemanticValues values) {
			return std::make_pair(std::any_cast<std::string>(values[0]), std::any_cast<gd::value>(values[1]));
		};

		parser["Dictionary"] = [](peg::SemanticValues values) {
			std::unordered_map<std::string, gd::value> dictionary;

			std::ranges::transform(values, inserter(dictionary, begin(dictionary)), [](auto value) {
				return std::any_cast<std::pair<std::string, gd::value>>(value);
			});

			return dictionary;
		};

		parser["Array"] = [](peg::SemanticValues values) {
			gd::array_t array;

			std::ranges::transform(values, back_inserter(array), [](auto value) {
				return std::any_cast<gd::value>(value);
			});

			return array;
		};

		parser["Field"] = [](peg::SemanticValues values) -> gd::field {
			return {
				.name = std::any_cast<std::string>(values[0]),
				.value = std::any_cast<gd::value>(values[1]),
			};
		};

		parser["String"] = [](peg::SemanticValues values) {
			return values.token_to_string();
		};

		parser["Identifier"] = [](peg::SemanticValues values) {
			return values.token_to_string();
		};

		parser["Integer"] = [](peg::SemanticValues values) {
			return values.token_to_number<int>();
		};

		parser["Float"] = [](peg::SemanticValues values) {
			return values.token_to_number<float>();
		};

		parser["Boolean"] = [](peg::SemanticValues values) {
			return values.token_to_string() == "true";
		};

		parser["Numeric"] = [](peg::SemanticValues values) -> gd::numeric_t {
			switch (values.choice())
			{
			case 0:
				return std::any_cast<float>(values[0]);
			case 1:
				return std::any_cast<int>(values[0]);
			default:
				return {};
			}
		};

		parser["Value"] = [](peg::SemanticValues values) -> gd::value {
			switch (values.choice())
			{
			case 0:
				return std::any_cast<gd::constructable>(values[0]);
			case 1:
				return std::any_cast<gd::dictionary_t>(values[0]);
			case 2:
				return std::any_cast<gd::array_t>(values[0]);
			case 3:
				return std::any_cast<bool>(values[0]);
			case 4:
				return std::any_cast<std::string>(values[0]);
			case 5:
				return std::any_cast<gd::numeric_t>(values[0]);
			default:
				return {};
			}
		};

		parser.set_logger([](auto file, auto column, auto message) {
			std::cerr << file << ":" << column << ": " << message << std::endl;
		});

		std::stringstream stringstream;
		stringstream << stream.rdbuf();

		gd::file file;

		parser.parse(stringstream.str(), file);

		return file;
	}
}
