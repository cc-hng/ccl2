#include <fmt/core.h>

#ifdef CCL2_USE_YYJSON

#    include <optional>
#    include <string>
#    include <ccl2/json.h>
#    include <fmt/ranges.h>

std::string car_json = R"(
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] }
)";

struct car_t {
    std::string make;
    std::string model;
    int year;
    std::vector<double> tire_pressure;
    std::optional<std::string> owner;
};

BOOST_HANA_ADAPT_STRUCT(car_t, make, model, year, tire_pressure, owner);

inline void print_car(const car_t& car) {
    fmt::print("car: make: {}, model: {}, year: {}, tire pressure:{}\n",
               car.make,
               car.model,
               car.year,
               car.tire_pressure);
}

void test_obj_deserialize() {
    fmt::print("\n === test_obj_deserialize === \n");
    auto car = ccl2::json::parse<car_t>(car_json);
    print_car(car);
}

void test_obj_serialize() {
    fmt::print("\n === test_obj_serialize === \n");
    car_t car;
    car.make          = "Toyota";
    car.model         = "Carmy";
    car.year          = 2016;
    car.tire_pressure = {40.1, 39.9, 37.7, 40.4};
    car.owner         = std::make_optional("Jack");

    fmt::print("car: {}\n", ccl2::json::dump(car));
}

// clang-format off
template <class T>
struct common_msg_t {
    BOOST_HANA_DEFINE_STRUCT(common_msg_t,
            (int, code),
            (std::string, msg),
            (T, data));
};
// clang-format on

namespace ns {

// clang-format off
struct person_t {
    BOOST_HANA_DEFINE_STRUCT(person_t,
            (int, age),
            (std::string, name));
};
// clang-format on

}  // namespace ns

int main() {
    test_obj_serialize();
    test_obj_deserialize();

    fmt::print("\n === test struct === \n");
    ns::person_t p  = {23, "cc"};
    auto person_str = ccl2::json::dump(p);
    fmt::print("\nperson: {}\n", person_str);

    common_msg_t<ns::person_t> a;
    a.code = 200;
    a.msg  = "success";
    a.data = p;
    fmt::print("\ncommon msg: {}\n", ccl2::json::dump(a));

    std::string result;
    ccl2::json::parse<yyjson_val*>(person_str, [&](yyjson_val* root) {
        common_msg_t<yyjson_val*> m2;
        m2.code = 400;
        m2.msg  = "failed";
        m2.data = root;
        result  = ccl2::json::dump(m2);
    });
    fmt::print("\ncommon msg: {}\n", result);

    return 0;
}

#else

int main() {
    fmt::print("To use json, please recompile project with `CCL2_WITH_YYJSON'\n");
    return 0;
}

#endif
