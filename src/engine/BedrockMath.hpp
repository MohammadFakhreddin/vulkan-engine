#pragma once

#include <glm/vec4.hpp>
#include <glm/vec3.hpp>

#include <cstdlib>
#include <random>

namespace MFA::Math
{

    template<typename T>
    T constexpr PiTemplate = static_cast<T>(3.14159265358979323846264338327950288419);

    static constexpr float PiFloat = PiTemplate<float>;
    static constexpr double PiDouble = PiTemplate<double>;

    inline static const glm::vec4 ForwardVector4{ 0.0f, 0.0f, 1.0f, 0.0f };
    inline static const glm::vec4 RightVector4{ 1.0f, 0.0f, 0.0f, 0.0f };
    inline static const glm::vec4 UpVector4{ 0.0f, 1.0f, 0.0f, 0.0f };

    inline static const glm::vec3 ForwardVector3{ 0.0f, 0.0f, 1.0f};
    inline static const glm::vec3 RightVector3{ 1.0f, 0.0f, 0.0f};
    inline static const glm::vec3 UpVector3{ 0.0f, 1.0f, 0.0f};

    template<typename A, typename B, typename C>
    A Clamp(const A & value, B min_value, C max_value)
    {
        if (value < min_value)
        {
            return A(min_value);
        }
        if (value > max_value)
        {
            return A(max_value);
        }
        return A(value);
    }

    template<typename A>
    A Deg2Rad(const A & value)
    {
        static constexpr double degToRadFactor = PiDouble / 180.0;
        return A(static_cast<double>(value) * degToRadFactor);
    }

    template<typename A>
    A Rad2Deg(const A & value)
    {
        static constexpr double radToDegFactor = 180.0f / PiDouble;
        return A(static_cast<double>(value) * radToDegFactor);
    }

    template<typename NumberType>
    NumberType Min(NumberType a, NumberType b)
    {
        return a < b ? a : b;
    }

    template<typename NumberType>
    NumberType Max(NumberType a, NumberType b)
    {
        return a > b ? a : b;
    }

    template<typename genType>
    genType Epsilon()
    {
        static_assert(std::numeric_limits<genType>::is_iec559, "'epsilon' only accepts floating-point inputs");
        return std::numeric_limits<genType>::epsilon();
    }

    template<typename T>
    T Random(T min, T max)
    {
//        float const fMin = static_cast<float>(min);
//        float const fMax = static_cast<float>(max);
//        // TODO Add seed and other stuff
//        float value = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX))
//            * (fMax - fMin) + fMin;
//        return static_cast<T>(value);
        static std::uniform_real_distribution<T> distribution(min, max);
        static std::mt19937 generator {};
        return distribution(generator);
    }

};
