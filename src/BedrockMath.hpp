#ifndef MATH_CLASS
#define MATH_CLASS 

namespace MFA::Math {

template<typename T>
T constexpr PiTemplate = static_cast<T>(3.14159265358979323846264338327950288419);

static constexpr float PiFloat = PiTemplate<float>;
static constexpr double PiDouble = PiTemplate<double>;

template<typename A,typename B,typename C>
A Clamp(const A & value, B min_value, C max_value) {
    if (value < min_value) {
        return A(min_value);
    }
    if (value > max_value) {
        return A(max_value);
    }
    return A(value);
}

template<typename A>
A Deg2Rad(const A & value) {
    return A((static_cast<double>(value) * PiDouble) / 180);
}

template<typename NumberType>
NumberType Min(NumberType a, NumberType b) {
    return a < b ? a : b;
}

template<typename NumberType>
NumberType Max(NumberType a, NumberType b) {
    return a > b ? a : b;
}

};

#endif