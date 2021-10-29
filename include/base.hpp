
#pragma once
#ifndef BASE_HPP
#define BASE_HPP


#include <cassert>
#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <exception>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>


namespace json {


#define JSON_THROW_EXCEPTION_IF(condition, exception) \
	do {                                              \
		if (!!(condition)) {                          \
			throw(exception);                         \
		}                                             \
	} while (false)

#define JSON_NOTHROW_EXCEPTION_IF(condition, exception) assert(!(condition))


#ifdef ENABLE_LOGIC_EXCEPTION
#define JSON_THROW_LOGIC_EXCEPTION_IF(condition, exception) \
	JSON_THROW_EXCEPTION_IF(condition, exception)
inline constexpr bool logic_exception_disabled = false;
#else
#define JSON_THROW_LOGIC_EXCEPTION_IF(condition, exception) \
	JSON_NOTHROW_EXCEPTION_IF(condition, exception)
inline constexpr bool logic_exception_disabled = true;
#endif


#ifdef ENABLE_RUNTIME_EXCEPTION
#define JSON_THROW_RUNTIME_EXCEPTION_IF(condition, exception) \
	JSON_THROW_EXCEPTION_IF(condition, exception)
inline constexpr bool runtime_exception_disabled = false;
#else
#define JSON_THROW_RUNTIME_EXCEPTION_IF(condition, exception) \
	JSON_NOTHROW_EXCEPTION_IF(condition, exception)
inline constexpr bool runtime_exception_disabled = true;
#endif

inline constexpr bool all_exception_disabled =
    logic_exception_disabled && runtime_exception_disabled;

class logic_error : public std::logic_error {
public:
	using std::logic_error::logic_error;
};

class out_of_range : public logic_error {
public:
	using logic_error::logic_error;
};

class runtime_error : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

class wrong_type : public runtime_error {
public:
	using runtime_error::runtime_error;
};


enum class tag
{
	null,
	object,
	array,
	number,
	string,
	boolean
};


namespace implementation {


template <typename Type>
struct remove_cvref {
	using type = std::remove_cv_t<std::remove_reference_t<Type>>;
};

template <typename Type>
using remove_cvref_t = typename remove_cvref<Type>::type;


class abstract_node {
private:
	[[nodiscard]] virtual tag type_virtual() const noexcept = 0;

public:
	abstract_node() noexcept = default;

	virtual ~abstract_node() noexcept = default;

	abstract_node(const abstract_node &) = default;
	abstract_node(abstract_node &&) noexcept = default;

	abstract_node &operator=(const abstract_node &) = default;
	abstract_node &operator=(abstract_node &&) noexcept = default;

	[[nodiscard]] tag type() const noexcept {
		return this->type_virtual();
	}
};


template <typename Value>
class object_node final : public abstract_node {
public:
	template <typename Type>
	using is_convertible_to = std::is_same<object_node, remove_cvref_t<Type>>;

	template <typename Type>
	inline static constexpr bool is_convertible_to_v = is_convertible_to<Type>::value;

	template <typename Type>
	using is_convertible_from = std::is_same<object_node, std::remove_cv_t<Type>>;

	template <typename Type>
	inline static constexpr bool is_convertible_from_v = is_convertible_to<Type>::value;

private:
	using value_type = Value;
	using storage_type = std::map<std::string, value_type, std::less<>>;

	storage_type m_storage;

	[[nodiscard]] tag type_virtual() const noexcept override {
		return tag::object;
	}

public:
	explicit object_node() noexcept = default;

	~object_node() noexcept override = default;

	object_node(const object_node &) = default;
	object_node(object_node &&) noexcept = default;

	object_node &operator=(const object_node &) = default;
	object_node &operator=(object_node &&) noexcept = default;

	[[nodiscard]] tag type_at(std::string_view key) const noexcept(logic_exception_disabled) {

		auto iterator = m_storage.find(key);

		JSON_THROW_LOGIC_EXCEPTION_IF(iterator == m_storage.end(), out_of_range{"invalid key"});

		return iterator->second.type();
	}

	[[nodiscard]] const value_type &get(std::string_view key) const
	    noexcept(logic_exception_disabled) {

		auto iterator = m_storage.find(key);

		JSON_THROW_LOGIC_EXCEPTION_IF(iterator == m_storage.end(), out_of_range{"invalid key"});

		return iterator->second;
	}

	[[nodiscard]] value_type &get(std::string_view key) noexcept(logic_exception_disabled) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast): Legal use of const_cast
		return const_cast<value_type &>(std::as_const(*this).get(key));
	}

	template <typename Type>
	[[nodiscard]] decltype(auto) get(std::string_view key) const noexcept(all_exception_disabled) {
		return get(key).template get<Type>();
	}

	template <typename Type>
	[[nodiscard]] decltype(auto) get(std::string_view key) noexcept(all_exception_disabled) {
		return get(key).template get<Type>();
	}

	[[nodiscard]] const value_type &operator[](std::string_view key) const
	    noexcept(logic_exception_disabled) {
		return get(key);
	}

	[[nodiscard]] value_type &operator[](std::string_view key) noexcept(logic_exception_disabled) {
		return get(key);
	}

	template <typename Type>
	void add(std::string key, Type &&argument) {

		JSON_THROW_LOGIC_EXCEPTION_IF(m_storage.find(key) != m_storage.end(),
		                              out_of_range{"invalid key"});

		m_storage.emplace(std::move(key), std::forward<Type>(argument));
	}

	template <typename Type>
	void set(std::string_view key, Type &&argument) {

		auto iterator = m_storage.find(key);

		JSON_THROW_LOGIC_EXCEPTION_IF(iterator == m_storage.end(), out_of_range{"invalid key"});

		iterator->second.set(std::forward<Type>(argument));
	}

	[[nodiscard]] const value_type &pop(std::string_view key) noexcept(logic_exception_disabled) {

		auto iterator = m_storage.find(key);

		JSON_THROW_LOGIC_EXCEPTION_IF(iterator == m_storage.end(), out_of_range{"invalid key"});

		auto result = std::move(iterator->second);

		m_storage.erase(iterator);

		return result;
	}

	template <typename Type>
	[[nodiscard]] auto pop(std::string_view key) noexcept(logic_exception_disabled) {

		auto iterator = m_storage.find(key);

		JSON_THROW_LOGIC_EXCEPTION_IF(iterator == m_storage.end(), out_of_range{"invalid key"});

		auto result = std::move(iterator->second.template get<Type>());

		m_storage.erase(iterator);

		return result;
	}

	void remove(std::string_view key) noexcept(logic_exception_disabled) {

		auto iterator = m_storage.find(key);

		JSON_THROW_LOGIC_EXCEPTION_IF(iterator == m_storage.end(), out_of_range{"invalid key"});

		m_storage.erase(iterator);
	}

	void rename(std::string_view first_key, std::string second_key) {

		auto iterator = m_storage.find(first_key);

		JSON_THROW_LOGIC_EXCEPTION_IF(iterator == m_storage.end(), out_of_range{"invalid key"});

		JSON_THROW_LOGIC_EXCEPTION_IF(m_storage.find(second_key) == m_storage.end(),
		                              out_of_range{"invalid key"});

		auto value = std::move(iterator->second);

		m_storage.erase(iterator);

		m_storage.emplace(std::move(second_key), std::move(value));
	}

	[[nodiscard]] bool contains(std::string_view key) const noexcept {
		return m_storage.find(key) != m_storage.end();
	}

	void clear() noexcept {
		m_storage.clear();
	}

	[[nodiscard]] bool empty() const noexcept {
		return m_storage.empty();
	}

	[[nodiscard]] std::size_t size() const noexcept {
		return m_storage.size();
	}

	[[nodiscard]] typename storage_type::const_iterator begin() const noexcept {
		return m_storage.begin();
	}

	[[nodiscard]] typename storage_type::iterator begin() noexcept {
		return m_storage.begin();
	}

	[[nodiscard]] typename storage_type::const_iterator cbegin() const noexcept {
		return m_storage.cbegin();
	}

	[[nodiscard]] typename storage_type::const_iterator end() const noexcept {
		return m_storage.end();
	}

	[[nodiscard]] typename storage_type::iterator end() noexcept {
		return m_storage.end();
	}

	[[nodiscard]] typename storage_type::const_iterator cend() const noexcept {
		return m_storage.cend();
	}

	[[nodiscard]] typename storage_type::const_iterator rbegin() const noexcept {
		return m_storage.rbegin();
	}

	[[nodiscard]] typename storage_type::iterator rbegin() noexcept {
		return m_storage.rbegin();
	}

	[[nodiscard]] typename storage_type::const_iterator crbegin() const noexcept {
		return m_storage.crbegin();
	}

	[[nodiscard]] typename storage_type::const_iterator rend() const noexcept {
		return m_storage.rend();
	}

	[[nodiscard]] typename storage_type::iterator rend() noexcept {
		return m_storage.rend();
	}

	[[nodiscard]] typename storage_type::const_iterator crend() const noexcept {
		return m_storage.crend();
	}
};


template <typename Value>
class array_node final : public abstract_node {
public:
	template <typename Type>
	using is_convertible_to = std::is_same<array_node, remove_cvref_t<Type>>;

	template <typename Type>
	inline static constexpr bool is_convertible_to_v = is_convertible_to<Type>::value;

	template <typename Type>
	using is_convertible_from = std::is_same<array_node, std::remove_cv_t<Type>>;

	template <typename Type>
	inline static constexpr bool is_convertible_from_v = is_convertible_to<Type>::value;

private:
	using value_type = Value;
	using storage_type = std::vector<value_type>;

	storage_type m_storage;

	[[nodiscard]] tag type_virtual() const noexcept override {
		return tag::array;
	}

public:
	explicit array_node() noexcept = default;

	~array_node() noexcept override = default;

	array_node(const array_node &) = default;
	array_node(array_node &&) noexcept = default;

	array_node &operator=(const array_node &) = default;
	array_node &operator=(array_node &&) noexcept = default;

	[[nodiscard]] tag type_at(std::size_t index) const noexcept(logic_exception_disabled) {

		JSON_THROW_LOGIC_EXCEPTION_IF(index >= m_storage.size(), out_of_range{"invalid index"});

		return m_storage[index].type();
	}

	[[nodiscard]] const value_type &get(std::size_t index) const
	    noexcept(logic_exception_disabled) {

		JSON_THROW_LOGIC_EXCEPTION_IF(index >= m_storage.size(), out_of_range{"invalid index"});

		return m_storage[index];
	}

	[[nodiscard]] value_type &get(std::size_t index) noexcept(logic_exception_disabled) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast): Legal use of const_cast
		return const_cast<value_type &>(std::as_const(*this).get(index));
	}

	template <typename Type>
	[[nodiscard]] decltype(auto) get(std::size_t index) const noexcept(all_exception_disabled) {
		return get(index).template get<Type>();
	}

	template <typename Type>
	[[nodiscard]] decltype(auto) get(std::size_t index) noexcept(all_exception_disabled) {
		return get(index).template get<Type>();
	}

	[[nodiscard]] const value_type &operator[](std::size_t index) const
	    noexcept(logic_exception_disabled) {
		return get(index);
	}

	[[nodiscard]] value_type &operator[](std::size_t index) noexcept(logic_exception_disabled) {
		return get(index);
	}

	[[nodiscard]] const value_type &front() const noexcept(logic_exception_disabled) {
		return get(0);
	}

	[[nodiscard]] value_type &front() noexcept(logic_exception_disabled) {
		return get(0);
	}

	template <typename Type>
	[[nodiscard]] decltype(auto) front() const noexcept(all_exception_disabled) {
		return front().template get<Type>();
	}

	template <typename Type>
	[[nodiscard]] decltype(auto) front() noexcept(all_exception_disabled) {
		return front().template get<Type>();
	}

	[[nodiscard]] const value_type &back() const noexcept(logic_exception_disabled) {

		JSON_THROW_LOGIC_EXCEPTION_IF(m_storage.empty(), out_of_range{"invalid index"});

		return m_storage.back();
	}

	[[nodiscard]] value_type &back() noexcept(logic_exception_disabled) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast): Legal use of const_cast
		return const_cast<value_type &>(std::as_const(*this).back());
	}

	template <typename Type>
	[[nodiscard]] decltype(auto) back() const noexcept(all_exception_disabled) {
		return back().template get<Type>();
	}

	template <typename Type>
	[[nodiscard]] decltype(auto) back() noexcept(all_exception_disabled) {
		return back().template get<Type>();
	}

	template <typename Type>
	void add(std::size_t index, Type &&argument) {

		JSON_THROW_LOGIC_EXCEPTION_IF(index > m_storage.size(), out_of_range{"invalid index"});

		m_storage.emplace(m_storage.begin() + index, std::forward<Type>(argument));
	}

	template <typename Type>
	void add(Type &&argument) {

		m_storage.emplace(m_storage.end(), std::forward<Type>(argument));
	}

	template <typename Type>
	void set(std::size_t index, Type &&argument) {

		JSON_THROW_LOGIC_EXCEPTION_IF(index >= m_storage.size(), out_of_range{"invalid index"});

		m_storage[index].set(std::forward<Type>(argument));
	}

	[[nodiscard]] const value_type &pop(std::size_t index) noexcept(logic_exception_disabled) {

		JSON_THROW_LOGIC_EXCEPTION_IF(index >= m_storage.size(), out_of_range{"invalid index"});

		auto result = std::move(m_storage[index]);

		m_storage.erase(m_storage.begin() + index);

		return result;
	}

	[[nodiscard]] value_type &pop() noexcept(logic_exception_disabled) {

		JSON_THROW_LOGIC_EXCEPTION_IF(m_storage.empty(), out_of_range{"invalid index"});

		return m_storage.pop_back();
	}

	template <typename Type>
	[[nodiscard]] auto pop(std::size_t index) noexcept(logic_exception_disabled) {

		JSON_THROW_LOGIC_EXCEPTION_IF(index >= m_storage.size(), out_of_range{"invalid index"});

		auto result = std::move(m_storage[index].template get<Type>());

		m_storage.erase(m_storage.begin() + index);

		return result;
	}

	template <typename Type>
	[[nodiscard]] auto pop() noexcept(logic_exception_disabled) {

		JSON_THROW_LOGIC_EXCEPTION_IF(m_storage.empty(), out_of_range{"invalid index"});

		return m_storage.pop_back().template get<Type>();
	}

	void remove(std::size_t index) noexcept(logic_exception_disabled) {

		JSON_THROW_LOGIC_EXCEPTION_IF(index >= m_storage.size(), out_of_range{"invalid index"});

		m_storage.erase(m_storage.begin() + index);
	}

	template <typename Functor>
	void sort(Functor functor) {
		std::sort(begin(), end(), functor);
	}

	template <typename Type, typename Functor>
	void sort(Functor functor) {

		std::sort(begin(), end(), [&functor](value_type &lhs, value_type &rhs) {
			return functor(lhs.template get<Type>(), rhs.template get<Type>());
		});
	}

	template <typename Type>
	void sort() {
		std::sort(begin(), end(), [](value_type &lhs, value_type &rhs) {
			return lhs.template get<Type>() < rhs.template get<Type>();
		});
	}

	void clear() noexcept {
		m_storage.clear();
	}

	[[nodiscard]] bool empty() const noexcept {
		return m_storage.empty();
	}

	[[nodiscard]] std::size_t size() const noexcept {
		return m_storage.size();
	}

	[[nodiscard]] typename storage_type::const_iterator begin() const noexcept {
		return m_storage.begin();
	}

	[[nodiscard]] typename storage_type::iterator begin() noexcept {
		return m_storage.begin();
	}

	[[nodiscard]] typename storage_type::const_iterator cbegin() const noexcept {
		return m_storage.cbegin();
	}

	[[nodiscard]] typename storage_type::const_iterator end() const noexcept {
		return m_storage.end();
	}

	[[nodiscard]] typename storage_type::iterator end() noexcept {
		return m_storage.end();
	}

	[[nodiscard]] typename storage_type::const_iterator cend() const noexcept {
		return m_storage.cend();
	}

	[[nodiscard]] typename storage_type::const_iterator rbegin() const noexcept {
		return m_storage.rbegin();
	}

	[[nodiscard]] typename storage_type::iterator rbegin() noexcept {
		return m_storage.rbegin();
	}

	[[nodiscard]] typename storage_type::const_iterator crbegin() const noexcept {
		return m_storage.crbegin();
	}

	[[nodiscard]] typename storage_type::const_iterator rend() const noexcept {
		return m_storage.rend();
	}

	[[nodiscard]] typename storage_type::iterator rend() noexcept {
		return m_storage.rend();
	}

	[[nodiscard]] typename storage_type::const_iterator crend() const noexcept {
		return m_storage.crend();
	}
};


class number_node final : public abstract_node {
private:
	template <typename Type>
	using is_signed_integer = std::conjunction<std::is_integral<Type>, std::is_signed<Type>>;

	template <typename Type>
	inline static constexpr bool is_signed_integer_v = is_signed_integer<Type>::value;

	template <typename Type>
	using is_unsigned_integer =
	    std::conjunction<std::is_integral<Type>, std::is_unsigned<Type>,
	                     std::negation<std::is_same<bool, std::remove_cv_t<Type>>>>;

	template <typename Type>
	inline static constexpr bool is_unsigned_integer_v = is_unsigned_integer<Type>::value;

	template <typename Type>
	using is_number = std::disjunction<is_signed_integer<Type>, is_unsigned_integer<Type>,
	                                   std::is_floating_point<Type>>;

	template <typename Type>
	inline static constexpr bool is_number_v = is_number<Type>::value;

	using signed_integer_type = signed long long int;
	using unsigned_integer_type = unsigned long long int;
	using floating_point_type = long double;

public:
	template <typename Type>
	using is_convertible_to = std::disjunction<std::is_same<number_node, remove_cvref_t<Type>>,
	                                           is_number<remove_cvref_t<Type>>>;

	template <typename Type>
	inline static constexpr bool is_convertible_to_v = is_convertible_to<Type>::value;

	template <typename Type>
	using is_convertible_from = std::disjunction<std::is_same<number_node, std::remove_cv_t<Type>>,
	                                             is_number<std::remove_cv_t<Type>>>;

	template <typename Type>
	inline static constexpr bool is_convertible_from_v = is_convertible_to<Type>::value;

private:
	using storage_type =
	    std::variant<signed_integer_type, unsigned_integer_type, floating_point_type>;

	storage_type m_storage;

	[[nodiscard]] tag type_virtual() const noexcept override {
		return tag::number;
	}

public:
	number_node() noexcept = delete;

	template <typename Type,
	          std::enable_if_t<is_signed_integer_v<std::remove_reference_t<Type>>, int> = 0>
	explicit number_node(Type argument) noexcept :
	    m_storage{static_cast<signed_integer_type>(argument)} {}

	template <typename Type,
	          std::enable_if_t<is_unsigned_integer_v<std::remove_reference_t<Type>>, int> = 0>
	explicit number_node(Type argument) noexcept :
	    m_storage{static_cast<unsigned_integer_type>(argument)} {}

	template <typename Type,
	          std::enable_if_t<std::is_floating_point_v<std::remove_reference_t<Type>>, int> = 0>
	explicit number_node(Type argument) noexcept :
	    m_storage{static_cast<floating_point_type>(argument)} {}

	~number_node() noexcept override = default;

	number_node(const number_node &) noexcept = default;
	number_node(number_node &&) noexcept = default;

	number_node &operator=(const number_node &) noexcept = default;
	number_node &operator=(number_node &&) noexcept = default;

	template <typename Functor>
	decltype(auto) visit(Functor &&functor) const noexcept {
		return std::visit(std::forward<Functor>(functor), m_storage);
	}

	template <typename Type,
	          std::enable_if_t<is_signed_integer_v<std::remove_reference_t<Type>>, int> = 0>
	void set(Type argument) noexcept {
		m_storage = static_cast<signed_integer_type>(argument);
	}

	template <typename Type,
	          std::enable_if_t<is_unsigned_integer_v<std::remove_reference_t<Type>>, int> = 0>
	void set(Type argument) noexcept {
		m_storage = static_cast<unsigned_integer_type>(argument);
	}

	template <typename Type,
	          std::enable_if_t<std::is_floating_point_v<std::remove_reference_t<Type>>, int> = 0>
	void set(Type argument) noexcept {
		m_storage = static_cast<floating_point_type>(argument);
	}

	template <typename Type, std::enable_if_t<is_number_v<std::remove_reference_t<Type>>, int> = 0>
	number_node &operator=(Type argument) noexcept {
		set(argument);
		return *this;
	}

	template <typename Type, std::enable_if_t<is_convertible_from_v<Type>, int> = 0>
	[[nodiscard]] Type get() const noexcept {
		return std::visit(
		    [](auto argument) -> Type {
			    return static_cast<Type>(argument);
		    },
		    m_storage);
	}
};


class string_node final : public abstract_node {
public:
	template <typename Type>
	using is_convertible_to =
	    std::disjunction<std::is_same<string_node, remove_cvref_t<Type>>,
	                     std::is_same<std::string, remove_cvref_t<Type>>,
	                     std::is_constructible<std::string, remove_cvref_t<Type>>>;

	template <typename Type>
	inline static constexpr bool is_convertible_to_v = is_convertible_to<Type>::value;

	template <typename Type>
	using is_convertible_from = std::disjunction<std::is_same<string_node, std::remove_cv_t<Type>>,
	                                             std::is_same<std::string, std::remove_cv_t<Type>>>;

	template <typename Type>
	inline static constexpr bool is_convertible_from_v = is_convertible_to<Type>::value;

private:
	std::string m_storage;

	[[nodiscard]] tag type_virtual() const noexcept override {
		return tag::string;
	}

public:
	string_node() noexcept = delete;

	explicit string_node(std::string argument) noexcept : m_storage{std::move(argument)} {};

	~string_node() noexcept override = default;

	string_node(const string_node &) = default;
	string_node(string_node &&) noexcept = default;

	string_node &operator=(const string_node &) = default;
	string_node &operator=(string_node &&) noexcept = default;

	void set(std::string argument) noexcept {
		m_storage = std::move(argument);
	}

	string_node &operator=(std::string argument) noexcept {
		m_storage = std::move(argument);
		return *this;
	}

	[[nodiscard]] const std::string &get() const noexcept {
		return m_storage;
	}

	[[nodiscard]] std::string &get() noexcept {
		return m_storage;
	}
};


class boolean_node final : public abstract_node {
public:
	template <typename Type>
	using is_convertible_to = std::disjunction<std::is_same<boolean_node, remove_cvref_t<Type>>,
	                                           std::is_same<bool, remove_cvref_t<Type>>>;

	template <typename Type>
	inline static constexpr bool is_convertible_to_v = is_convertible_to<Type>::value;

	template <typename Type>
	using is_convertible_from = std::disjunction<std::is_same<boolean_node, std::remove_cv_t<Type>>,
	                                             std::is_same<bool, std::remove_cv_t<Type>>>;

	template <typename Type>
	inline static constexpr bool is_convertible_from_v = is_convertible_to<Type>::value;

private:
	bool m_storage;

	[[nodiscard]] tag type_virtual() const noexcept override {
		return tag::boolean;
	}

public:
	boolean_node() noexcept = delete;

	explicit boolean_node(bool argument) noexcept : m_storage{argument} {};

	~boolean_node() noexcept override = default;

	boolean_node(const boolean_node &) noexcept = default;
	boolean_node(boolean_node &&) noexcept = default;

	boolean_node &operator=(const boolean_node &) noexcept = default;
	boolean_node &operator=(boolean_node &&) noexcept = default;

	void set(bool argument) noexcept {
		m_storage = argument;
	}

	boolean_node &operator=(bool argument) noexcept {
		m_storage = argument;
		return *this;
	}

	[[nodiscard]] bool get() const noexcept {
		return m_storage;
	}
};


class value_wrapper final {
public:
	using object_type = object_node<value_wrapper>;
	using array_type = array_node<value_wrapper>;
	using number_type = number_node;
	using string_type = string_node;
	using boolean_type = boolean_node;


	template <typename, typename = void>
	struct convertible_to_tag;

	template <typename Type>
	struct convertible_to_tag<Type, std::enable_if_t<object_type::is_convertible_to_v<Type>>> :
	    std::integral_constant<tag, tag::object> {};

	template <typename Type>
	struct convertible_to_tag<Type, std::enable_if_t<array_type::is_convertible_to_v<Type>>> :
	    std::integral_constant<tag, tag::array> {};

	template <typename Type>
	struct convertible_to_tag<Type, std::enable_if_t<number_type::is_convertible_to_v<Type>>> :
	    std::integral_constant<tag, tag::number> {};

	template <typename Type>
	struct convertible_to_tag<Type, std::enable_if_t<string_type::is_convertible_to_v<Type>>> :
	    std::integral_constant<tag, tag::string> {};

	template <typename Type>
	struct convertible_to_tag<Type, std::enable_if_t<boolean_type::is_convertible_to_v<Type>>> :
	    std::integral_constant<tag, tag::boolean> {};

	template <typename Type>
	inline static constexpr tag convertible_to_tag_v = convertible_to_tag<Type>::value;


	template <typename, typename = void>
	struct convertible_from_tag;

	template <typename Type>
	struct convertible_from_tag<Type, std::enable_if_t<object_type::is_convertible_from_v<Type>>> :
	    std::integral_constant<tag, tag::object> {};

	template <typename Type>
	struct convertible_from_tag<Type, std::enable_if_t<array_type::is_convertible_from_v<Type>>> :
	    std::integral_constant<tag, tag::array> {};

	template <typename Type>
	struct convertible_from_tag<Type, std::enable_if_t<number_type::is_convertible_from_v<Type>>> :
	    std::integral_constant<tag, tag::number> {};

	template <typename Type>
	struct convertible_from_tag<Type, std::enable_if_t<string_type::is_convertible_from_v<Type>>> :
	    std::integral_constant<tag, tag::string> {};

	template <typename Type>
	struct convertible_from_tag<Type, std::enable_if_t<boolean_type::is_convertible_from_v<Type>>> :
	    std::integral_constant<tag, tag::boolean> {};

	template <typename Type>
	inline static constexpr tag convertible_from_tag_v = convertible_from_tag<Type>::value;


	template <typename, typename = void>
	struct convertible_to_class;

	template <typename Type>
	struct convertible_to_class<Type, std::enable_if_t<object_type::is_convertible_to_v<Type>>> {
		using type = typename value_wrapper::object_type;
	};

	template <typename Type>
	struct convertible_to_class<Type, std::enable_if_t<array_type::is_convertible_to_v<Type>>> {
		using type = typename value_wrapper::array_type;
	};

	template <typename Type>
	struct convertible_to_class<Type, std::enable_if_t<number_type::is_convertible_to_v<Type>>> {
		using type = typename value_wrapper::number_type;
	};

	template <typename Type>
	struct convertible_to_class<Type, std::enable_if_t<string_type::is_convertible_to_v<Type>>> {
		using type = typename value_wrapper::string_type;
	};

	template <typename Type>
	struct convertible_to_class<Type, std::enable_if_t<boolean_type::is_convertible_to_v<Type>>> {
		using type = typename value_wrapper::boolean_type;
	};

	template <typename Type>
	using convertible_to_class_t = typename convertible_to_class<Type>::type;


	template <typename, typename = void>
	struct convertible_from_class;

	template <typename Type>
	struct convertible_from_class<Type,
	                              std::enable_if_t<object_type::is_convertible_from_v<Type>>> {
		using type = typename value_wrapper::object_type;
	};

	template <typename Type>
	struct convertible_from_class<Type, std::enable_if_t<array_type::is_convertible_from_v<Type>>> {
		using type = typename value_wrapper::array_type;
	};

	template <typename Type>
	struct convertible_from_class<Type,
	                              std::enable_if_t<number_type::is_convertible_from_v<Type>>> {
		using type = typename value_wrapper::number_type;
	};

	template <typename Type>
	struct convertible_from_class<Type,
	                              std::enable_if_t<string_type::is_convertible_from_v<Type>>> {
		using type = typename value_wrapper::string_type;
	};

	template <typename Type>
	struct convertible_from_class<Type,
	                              std::enable_if_t<boolean_type::is_convertible_from_v<Type>>> {
		using type = typename value_wrapper::boolean_type;
	};

	template <typename Type>
	using convertible_from_class_t = typename convertible_from_class<Type>::type;

	template <typename Type>
	using is_convertible_to_any =
	    std::disjunction<object_type::is_convertible_to<Type>, array_type::is_convertible_to<Type>,
	                     number_type::is_convertible_to<Type>, string_type::is_convertible_to<Type>,
	                     boolean_type::is_convertible_to<Type>>;

	template <typename Type>
	inline static constexpr bool is_convertible_to_any_v = is_convertible_to_any<Type>::value;

	template <typename Type>
	using is_convertible_from_any = std::disjunction<
	    object_type::is_convertible_from<Type>, array_type::is_convertible_from<Type>,
	    number_type::is_convertible_from<Type>, string_type::is_convertible_from<Type>,
	    boolean_type::is_convertible_from<Type>>;

	template <typename Type>
	inline static constexpr bool is_convertible_from_any_v = is_convertible_from_any<Type>::value;

	struct null_type {};
	inline static constexpr null_type null = null_type{};

private:
	using storage_type = std::unique_ptr<abstract_node>;

	storage_type m_storage;

	template <typename Functor, typename Pointer>
	static decltype(auto) visit(Functor &&functor, Pointer &pointer) {

		assert(pointer != nullptr);

		switch (pointer->type()) {
		case tag::object: {
			auto &value = static_cast<object_type &>(*pointer);
			return std::invoke(std::forward<Functor>(functor), value);
		}
		case tag::array: {
			auto &value = static_cast<array_type &>(*pointer);
			return std::invoke(std::forward<Functor>(functor), value);
		}
		case tag::number: {
			auto &value = static_cast<number_type &>(*pointer);
			return std::invoke(std::forward<Functor>(functor), value);
		}
		case tag::string: {
			auto &value = static_cast<string_type &>(*pointer);
			return std::invoke(std::forward<Functor>(functor), value);
		}
		case tag::boolean: {
			auto &value = static_cast<boolean_type &>(*pointer);
			return std::invoke(std::forward<Functor>(functor), value);
		}
		case tag::null:
		default:
			assert(false);
		}

#ifdef _MSC_VER
		__assume(false);
#endif
	}

	template <typename Type, typename = void>
	struct has_template_get : std::true_type {};

	template <typename Type>
	struct has_template_get<Type, std::void_t<decltype(std::declval<Type>().get())>> :
	    std::false_type {};

	template <typename Type>
	inline static constexpr bool has_template_get_v = has_template_get<Type>::value;

	template <typename Type, typename Node, std::enable_if_t<has_template_get_v<Node>, int> = 0>
	[[nodiscard]] static decltype(auto) get_template(Node &&argument) {
		return std::forward<Node>(argument).template get<Type>();
	}

	template <typename Type, typename Node,
	          std::enable_if_t<std::negation_v<has_template_get<Node>>, int> = 0>
	[[nodiscard]] static decltype(auto) get_template(Node &&argument) {
		return std::forward<Node>(argument).get();
	}

public:
	explicit value_wrapper() = default;

	explicit value_wrapper(null_type) noexcept {}

	template <typename Type, std::enable_if_t<is_convertible_to_any_v<Type>, int> = 0>
	// NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
	explicit value_wrapper(Type &&argument) {
		m_storage = std::make_unique<convertible_to_class_t<Type>>(std::forward<Type>(argument));
	}

	~value_wrapper() noexcept = default;

	value_wrapper(const value_wrapper &other) {

		if (this == std::addressof(other) || other.m_storage == nullptr) {
			return;
		}

		m_storage = visit(
		    [](const auto &value) -> storage_type {
			    using value_type = remove_cvref_t<decltype(value)>;
			    return std::make_unique<value_type>(value);
		    },
		    other.m_storage);
	}

	value_wrapper(value_wrapper &&) noexcept = default;

	value_wrapper &operator=(const value_wrapper &other) {

		if (this == std::addressof(other)) {
			return *this;
		}

		if (other.m_storage == nullptr) {
			m_storage.reset();
			return *this;
		}

		m_storage = visit(
		    [](const auto &value) -> storage_type {
			    using value_type = remove_cvref_t<decltype(value)>;
			    return std::make_unique<value_type>(value);
		    },
		    other.m_storage);

		return *this;
	}

	value_wrapper &operator=(value_wrapper &&) noexcept = default;

	[[nodiscard]] tag type() const noexcept {

		if (m_storage == nullptr) {
			return tag::null;
		}

		return m_storage->type();
	}

	template <typename Functor>
	decltype(auto) visit(Functor &&functor) const
	    noexcept(noexcept(visit(std::declval<Functor>(), m_storage))) {
		return visit(std::forward<Functor>(functor), m_storage);
	}

	template <typename Functor>
	decltype(auto)
	visit(Functor &&functor) noexcept(noexcept(visit(std::declval<Functor>(), m_storage))) {
		return visit(std::forward<Functor>(functor), m_storage);
	}

	void set(null_type) noexcept {
		m_storage.reset();
	}

	template <
	    typename Type,
	    std::enable_if_t<
	        std::conjunction_v<is_convertible_from_any<Type>,
	                           std::negation<std::is_same<Type, convertible_from_class_t<Type>>>>,
	        int> = 0>
	void set(Type &&argument) {

		if (m_storage == nullptr || m_storage->type() != convertible_from_tag_v<Type>) {
			m_storage =
			    std::make_unique<convertible_to_class_t<Type>>(std::forward<Type>(argument));
			return;
		}

		auto &reference = static_cast<convertible_from_class_t<Type> &>(*m_storage);

		reference.set(std::forward<Type>(argument));
	}

	template <
	    typename Type,
	    std::enable_if_t<std::conjunction_v<is_convertible_from_any<Type>,
	                                        std::is_same<Type, convertible_from_class_t<Type>>>,
	                     int> = 0>
	void set(Type &&argument) {

		if (m_storage == nullptr || m_storage->type() != convertible_from_tag_v<Type>) {
			m_storage =
			    std::make_unique<convertible_to_class_t<Type>>(std::forward<Type>(argument));
			return;
		}

		auto &reference = static_cast<convertible_from_class_t<Type> &>(*m_storage);

		reference = std::forward<Type>(argument);
	}

	value_wrapper &operator=(null_type) noexcept {
		m_storage.reset();
		return *this;
	}

	template <typename Type, std::enable_if_t<is_convertible_from_any_v<Type>, int> = 0>
	value_wrapper &operator=(Type &&argument) {
		set(std::forward<Type>(argument));
		return *this;
	}

	template <
	    typename Type,
	    std::enable_if_t<std::conjunction_v<is_convertible_from_any<Type>,
	                                        std::is_same<Type, convertible_from_class_t<Type>>>,
	                     int> = 0>
	[[nodiscard]] decltype(auto) get() const noexcept(runtime_exception_disabled) {

		JSON_THROW_RUNTIME_EXCEPTION_IF(m_storage->type() != convertible_from_tag_v<Type>,
		                                wrong_type{"mismatched types"});

		return static_cast<const convertible_from_class_t<Type> &>(*m_storage);
	}

	template <
	    typename Type,
	    std::enable_if_t<
	        std::conjunction_v<is_convertible_from_any<Type>,
	                           std::negation<std::is_same<Type, convertible_from_class_t<Type>>>>,
	        int> = 0>
	[[nodiscard]] decltype(auto) get() const noexcept(runtime_exception_disabled) {

		JSON_THROW_RUNTIME_EXCEPTION_IF(m_storage->type() != convertible_from_tag_v<Type>,
		                                wrong_type{"mismatched types"});

		return get_template<Type>(static_cast<const convertible_from_class_t<Type> &>(*m_storage));
	}

	template <
	    typename Type,
	    std::enable_if_t<std::conjunction_v<is_convertible_from_any<Type>,
	                                        std::is_same<Type, convertible_from_class_t<Type>>>,
	                     int> = 0>
	[[nodiscard]] decltype(auto) get() noexcept(runtime_exception_disabled) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast): Legal use of const_cast
		return const_cast<convertible_from_class_t<Type> &>(std::as_const(*this).get<Type>());
	}

	template <
	    typename Type,
	    std::enable_if_t<
	        std::conjunction_v<is_convertible_from_any<Type>,
	                           std::negation<std::is_same<Type, convertible_from_class_t<Type>>>>,
	        int> = 0>
	[[nodiscard]] decltype(auto) get() noexcept(runtime_exception_disabled) {
		const auto &const_reference = std::as_const(*this).get<convertible_from_class_t<Type>>();
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast): Legal use of const_cast
		return get_template<Type>(const_cast<convertible_from_class_t<Type> &>(const_reference));
	}
};


} // namespace implementation


using value = implementation::value_wrapper;
using object = value::object_type;
using array = value::array_type;
using number = value::number_type;
using string = value::string_type;
using boolean = value::boolean_type;


} // namespace json


#endif // BASE_HPP
