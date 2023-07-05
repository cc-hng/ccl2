#include <fmt/core.h>

#ifdef CCL2_USE_YYJSON

#    define BOOST_HANA_CONFIG_ENABLE_STRING_UDL

#    include <optional>
#    include <string>
#    include <type_traits>
#    include <utility>
#    include <boost/hana.hpp>
#    include <ccl2/json.h>
#    include <fmt/ranges.h>

namespace hana = boost::hana;
using namespace hana::literals;
using namespace std::literals;

std::string car_json = R"(
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] }
)";

struct car_t {
    std::string make;
    std::string model;
    int year;
    std::vector<double> tire_pressure;
    // std::optional<std::string> owner;
};

BOOST_HANA_ADAPT_STRUCT(car_t, make, model, year, tire_pressure);

struct person_t {
    std::string name;
    car_t car;
    std::vector<car_t> cars;
};

BOOST_HANA_ADAPT_STRUCT(person_t, name, car, cars);

template <int i>
using Int2Type = std::integral_constant<int, i>;

#    include <boost/core/demangle.hpp>

// Using hana::type<T> as a parameter type wouldn't work, since it may be
// a dependent type, in which case template argument deduction will fail.
// Using hana::basic_type<T> is fine, since this one is guaranteed to be
// a non dependent base of hana::type<T>.
template <typename T>
std::string const& name_of(hana::basic_type<T>) {
    static std::string name = boost::core::demangle(typeid(T).name());
    return name;
}

template <class T, class Dummy = hana::when<true>>
struct hana_struct_set_impl_t : public std::false_type {};

template <class T>
struct hana_struct_set_impl_t<T, hana::when<hana::Struct<T>::value>>
  : public std::true_type {
    enum { ITER_ING = 0, ITER_END };

    template <class V>
    static void set(T& t, const std::vector<std::string>& keys, int i, const V& v) {
        if (keys.size() - 1 == (size_t)i) {
            set<V>(t, keys, i, v, Int2Type<ITER_END>{});
        } else {
            set<V>(t, keys, i, v, Int2Type<ITER_ING>{});
        }
    }

private:
    static constexpr auto keys_ = hana::transform(hana::accessors<T>(), hana::first);

    template <class V>
    static void set(T& t, const std::vector<std::string>& keys, int i, const V& v,
                    Int2Type<ITER_ING>) {
        hana::for_each(keys_, [&](const auto& key) {
            if (keys.at(i) != hana::to<const char*>(key)) {
                return;
            }

            auto& element = hana::at_key(t, key);
            using Element = std::remove_reference_t<decltype(element)>;
            if constexpr (hana_struct_set_impl_t<Element>::value) {
                hana_struct_set_impl_t<Element>::template set<V>(element, keys, i + 1, v);
            }
        });
    }

    template <class V>
    static void set(T& t, const std::vector<std::string>& keys, int i, const V& v,
                    Int2Type<ITER_END>) {
        hana::for_each(hana::keys(t), [&](const auto& key) {
            if (keys.at(i) != hana::to<const char*>(key)) {
                return;
            }

            auto& element = hana::at_key(t, key);
            using Element = std::remove_reference_t<decltype(element)>;
            if constexpr (std::is_assignable_v<Element, V>) {
                element = v;
            }
        });
    }
};

template <class T, class A>
struct hana_struct_set_impl_t<std::vector<T, A>> : public std::true_type {
    template <class V>
    static void
    set(std::vector<T, A>& t, const std::vector<std::string>& keys, int i, const V& v) {
        auto key = std::stoi(keys.at(i));
        if (t.size() < (size_t)(key + 1)) {
            t.resize(key + 1);
        }

        auto& element = t.at(key);
        using Element = std::remove_reference_t<decltype(element)>;
        if (keys.size() - 1 == (size_t)i) {
            if constexpr (std::is_assignable_v<Element, V>) {
                element = v;
            }
        } else {
            if constexpr (hana_struct_set_impl_t<Element>::value) {
                hana_struct_set_impl_t<Element>::template set<V>(element, keys, i + 1, v);
            }
        }
    }
};

template <class V, class T>
void hana_struct_set_impl(T& t, const std::vector<std::string>& keys, const V& v) {
    hana_struct_set_impl_t<T>::template set<V>(t, keys, 0, v);
}

int main() {
    if (0) {
        auto car = ccl2::json::parse<car_t>(car_json);

        constexpr auto make = hana::string_c<'m', 'a', 'k', 'e'>;
        auto m              = hana::at_key(car, make);
        fmt::print("make: {}\n", m);

        hana::at_key(car, "make"_s) = "Das Auto";
        fmt::print("make: {}\n", car.make);

        constexpr auto year_str = BOOST_HANA_STRING("year");
        [[maybe_unused]] auto y = hana::at_key(car, "year"_s);
        fmt::print("year: {}\n", hana::at_key(car, year_str));
    }

    struct A : public std::true_type {};

    if constexpr (A::value) {
        fmt::print("A::A--------------------------------------------------\n");
    }

    if (1) {
        person_t p;
        hana_struct_set_impl(p, {"car", "make"}, "Das.Auto");
        fmt::print("car.make: {}\n", p.car.make);

        hana_struct_set_impl(p, {"cars", "1", "make"}, "Das.Auto");
        fmt::print("cars.1.make: {}\n", p.cars[1].make);
    }

    return 0;
}

#else

int main() {
    fmt::print("To use json, please recompile project with `CCL2_WITH_YYJSON'\n");
    return 0;
}

#endif
