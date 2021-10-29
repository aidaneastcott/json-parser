
#include <gtest/gtest.h>

#define ENABLE_LOGIC_EXCEPTION
#define ENABLE_RUNTIME_EXCEPTION

#include "json.hpp"


TEST(JsonObjectTest, ValidType) {
	auto object = json::object{};
	EXPECT_EQ(object.type(), json::tag::object);

	auto value = json::value{object};
	EXPECT_EQ(value.type(), json::tag::object);
}

TEST(JsonArrayTest, ValidType) {
	auto array = json::array{};
	EXPECT_EQ(array.type(), json::tag::array);

	auto value = json::value{array};
	EXPECT_EQ(value.type(), json::tag::array);
}

TEST(JsonNumberTest, ValidType) {
	auto number = json::number{0};
	EXPECT_EQ(number.type(), json::tag::number);

	auto value = json::value{number};
	EXPECT_EQ(value.type(), json::tag::number);
}

TEST(JsonStringTest, ValidType) {
	auto string = json::string{""};
	EXPECT_EQ(string.type(), json::tag::string);

	auto value = json::value{string};
	EXPECT_EQ(value.type(), json::tag::string);
}

TEST(JsonBooleanTest, ValidType) {
	auto boolean = json::boolean{true};
	EXPECT_EQ(boolean.type(), json::tag::boolean);

	auto value = json::value{boolean};
	EXPECT_EQ(value.type(), json::tag::boolean);
}

TEST(JsonNullTest, ValidType) {
	auto value = json::value{json::value::null};
	EXPECT_EQ(value.type(), json::tag::null);
}
