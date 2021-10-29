# JSON Parser

## Overview
A JSON library for converting between raw text and data types usable by C++.

## Features
- Converting between JSON formatted text and a specialized JSON-like C++ data structure
- Automatic parsing of JSON numbers into max width integral and floating point types
- Implemented using C++ Standard Library types such as `std::map`, `std::vector`, `std::unique_ptr`, and `std::variant`
- Dynamic memory is managed safely and automatically using RAII design patterns
- Move semantics are used wherever possible for performance and flexibility
- Polymorphism and inheritance are used under the hood to achieve type erasure
- However, the library interface is entirely non-virtual and does not expose any raw pointers to the user
- Extracting JSON values to C++ data types is done through templated functions
- Types are automatically deduced wherever possible using SFINAE and no explicit type is needed to insert data from C++
- An easy interface is provided to check the data type of any JSON value
- Both logic error and runtime exceptions can be optionally replaced with assert calls using macros if desired

## Examples

### Converting between raw JSON formatted text and `json::value`
```cpp
// Parse the JSON data from a raw string literal into json::value
std::optional<json::value> input = json::io::read(R"({"name":"John Smith"})");

if (input) {
  // The json::value can be extracted from std::optional if parsing succeeded
  json::value value = *input;
}
```
If the input is provided is invalid, the returned `std::optional` does not have a value.

```cpp
// Some function returning a json::value
json::value value = some_get_value();

// Write the JSON data from json::value directly to std::string
std::string output = json::io::write(value);
```
Outputting to `std::string` returns valid minified JSON formatted text.

### Extracting C++ fundamental types from `json::value`
```cpp
json::value value = some_get_value();

// Verify the underlying type of the json::value
if (value.type() == json::tag::number) {

  // Extract the JSON number as a C++ int
  int real_value = value.get<int>();
}
else if (value.type() == json::tag::string) {

  // Extract the JSON number as a C++ std::string
  std::string &real_value = value.get<std::string>();
}
```
The templated `get` function attempts to convert the type erased value to the requested type and returns it. Checking the type beforehand ensures that the function will succeed. If the value cannot be converted to the requested type and exceptions are enabled, a runtime exception is thrown. Otherwise, if exceptions are disabled, an assertion is triggered in debug mode.

### Extracting JSON library types from `json::value`
```cpp
json::value value = some_get_value();

// Verify the underlying type of the json::value
if (value.type() == json::tag::number) {

  // Extract the JSON number as a json::number
  json::number &number_value = value.get<json::number>();
}
```
The templated `get` function is also able to convert from `json::value` to the explicitly typed library classes. The `json::value` class is a wrapper whose only data member is a `std::unique_ptr` to the base class in an inheritance hierarchy. This is so that a general value type with a friendly interface can be passed around without exposing raw pointers to the user or requiring them to manager a `std::unique_ptr` instance. The underlying explicitly typed classes (`json::object`, `json::array`, `json::number`, `json::string`, `json::boolean`) are all accessible and able to be interfaced with directly if desired. A JSON null is represented as the `std::unique_ptr` set to nullptr.

### Setting `json::value` to C++ fundamental types
```cpp
json::value value = some_get_value();

// Set the json::value with std::string
value.set("Converted to std::string automatically!");
// Set the json::value with bool
value.set(true);
// Set the json::value with null
value.set(json::value::null);
```
The templated `set` function will convert the parameter to the appropriate JSON type and replace the underlying value. The function uses SFINAE to automatically deduce the type to convert to, so no explicit specialization is required. The `json::value::null` tag type can be used to insert a null value.

### Creating JSON objects programmatically in C++
```cpp
// Initialize an explicitly typed json::object variable directly
json::object object = json::object{};

// Add some key-value pairs into the json::object
object.add("name", "John Smith");
object.add("age", 25);
object.add("is_admin", true);
object.add("last_login", json::value::null);
```
The templated `add` function is used for inserting values into the `json::object` using a key-pair combination. The function uses SFINAE to automatically deduce the type to convert the value to.

### Extracting C++ fundamental types from `json::object`
```cpp
json::object object = some_get_object();

// Verify the underlying type of value at the specified key in the json::object
if (object.type_at("name") == json::tag::string) {

  // Extract the value at the specified key as a std::string
  std::string &real_value = object.get<std::string>("name");
}
```
The `type` function that accepts a parameter checks the type of the value at that key in the JSON object. The `get` function that accepts a parameter attempts to convert the type erased value at that key to the requested type and returns it.

### Additional `json::object` functions
```cpp
json::object object = some_get_object();

// Set the value at the specified key
object.set("name", "John Smith");

// Pop the value at the specified key
int real_value = object.pop<int>("age");

// Remove the value at the specified key
object.remove("address");

// Check if the object is empty
if (object.empty()) {
  std::cout << "Object is empty!\n";
}

// Clear the object contents
object.clear();

// Change the name of a specified key to a new value
object.rename("current_login", "last_login");

// Get begin and end iterators to the underlying std::map
auto begin = object.begin();
auto end = object.end();

// Loop through the entire object contents
while (; begin != end; ++begin) {
  json::value &value = *begin;
  if (value.type() == json::tag::number) {
    real_value = value.get<int>()
  }
}
```
The `json::object` class contains miscellaneous functions that provide a similar interface to the underlying `std::map`. Iterators to the underlying `std::map` are returned through the `begin` and `end` family functions, so the class can be used directly with range-based for loops or standard algorithms that take iterators. The expression `*(object.begin())` returns the type erased `json::value` class.

### Creating JSON arrays programmatically in C++
```cpp
// Initialize an explicitly typed json::array variable directly
json::array array = json::array{};

// Add some values into the json::array
array.add(10);
array.add(20);
array.add(30);
```
Similarly to `json::object`, the templated `add` function is used for inserting values into the `json::array`. The function uses SFINAE to automatically deduce the type to convert the value to.

### Extracting C++ fundamental types from `json::array`
```cpp
json::array array = some_get_array();

// Verify the underlying type of the index in the json::object
if (array.size() > 5 && array.type_at(5) == json::tag::number) {

  // Extract the value at the specified index as std::uint32_t
  std::uint32_t &real_value = array.get<std::uint32_t>(5);
}
```
Similarly to `json::object`, the `type` function that accepts a parameter checks the type of the value at that index in the JSON array. The `get` function that accepts a parameter attempts to convert the type erased value at that index to the requested type and returns it.

### Additional `json::array` functions
```cpp
json::array array = some_get_array();

// Access the last element in the array
bool real_value = array.back<bool>();
// Access the first element in the array
real_value = array.front<bool>();

// Set the value at the specified index
array.set(3, true);

// Pop the value at the specified index
real_value = array.pop<bool>(10);
// Pop the value at the back of the array
real_value = array.pop<bool>();

// Remove the value at the specified index
array.remove(10);

// Check if the array is empty
if (array.empty()) {
  std::cout << "Array is empty!\n";
}

// Clear the array contents
array.clear();

// Get begin and end iterators to the underlying std::vector
auto begin = array.begin();
auto end = array.end();

// Loop through the entire array contents
while (; begin != end; ++begin) {
  real_value = begin->get<bool>();
}
```
The `json::array` class contains miscellaneous functions that provide a similar interface to the underlying `std::vector`. Iterators to the underlying `std::vector` are returned through the `begin` and `end` family functions, so the class can be used directly with range-based for loops or standard algorithms that take iterators. The expression `*(array.begin())` returns the type erased `json::value` class.

### Sorting `json::array`
```cpp
json::array array = some_get_array();

// Sort the underlying types as int using operator<()
array.sort<int>();

// Sort the underlying types as int using a provided lambda
array.sort<int>([](auto lhs, auto rhs) { return lhs > rhs; });
```
Sorting can be done with the default `operator<()`, or by providing comparison functor. If the type is explicitly specified, the values are converted to that type before being passed to the functor. If no type is specified, the type erased `json::value` is used instead.

### Type conversion with `json::number`
```cpp
// Initialize the json::number with a double value
json::number number = number{3.14159};

{
  // Casts the underlying value stored in json::number to int
  int real_value = number.get<int>();

  // Possible output: 3
  std::cout << real_value << '\n';
}

{
  // Casts the underlying value stored in json::number to double
  double real_value = number.get<double>();

  // Possible output: 3.14159
  std::cout << real_value << '\n';
}
```
The underlying storage type of `json::number` is a `std::variant` of `signed long long int`, `unsigned long long int`, and `long double`. The type used by the `std::variant` to is chosen based on whether the type passed to it is signed integral, unsigned integral, or floating point, respectively. When a value is extracted from the `json::number`, the value is always converted from the underlying type to the requested type using `static_cast`.

### Enabling logic error and runtime exceptions
```cpp

#define ENABLE_LOGIC_EXCEPTION
#define ENABLE_RUNTIME_EXCEPTION

#include "json.hpp"

```
Macro constants `ENABLE_LOGIC_EXCEPTION` and `ENABLE_RUNTIME_EXCEPTION` can be used to change the behavior on error between throwing an exception and calling assert. All errors from the library can be checked for and validated against beforehand, however this can require significant boilerplate code if the JSON data is large or complex. Exceptions can be enabled to protect against bad access to the JSON data, which allows for shorter and less complex code at the expense of having many functions be potentially throwing.

## Todo
- Add clang-format and clang-tidy config files to the repository

## Project Requirements
C++17 language version.

## Additional notes
Tested with GCC 10.2.1, Clang 11.0.1, and MSVC v14.28. The project compiles on all three platforms with -Wall, -Wextra, -Wpedantic and -Werror compiler flags set (or their equivalent with MSVC).

## License
Licensed under [MIT](LICENSE).
