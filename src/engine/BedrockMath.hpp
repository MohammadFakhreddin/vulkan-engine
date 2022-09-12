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

    inline static constexpr glm::vec4 ForwardVec4W1{ 0.0f, 0.0f, 1.0f, 1.0f };
    inline static constexpr glm::vec4 RightVec4W1{ 1.0f, 0.0f, 0.0f, 1.0f };
    inline static constexpr glm::vec4 UpVec4W1{ 0.0f, 1.0f, 0.0f, 1.0f };

    inline static constexpr glm::vec4 ForwardVec4W0{ 0.0f, 0.0f, 1.0f, 0.0f };
    inline static constexpr glm::vec4 RightVec4W0{ 1.0f, 0.0f, 0.0f, 0.0f };
    inline static constexpr glm::vec4 UpVec4W0{ 0.0f, 1.0f, 0.0f, 0.0f };

    inline static constexpr glm::vec3 ForwardVec3{ 0.0f, 0.0f, 1.0f };
    inline static constexpr glm::vec3 RightVec3{ 1.0f, 0.0f, 0.0f };
    inline static constexpr glm::vec3 UpVec3{ 0.0f, 1.0f, 0.0f };

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
    
    template<typename genType>
    genType Infinity()
    {
        return std::numeric_limits<genType>::infinity();
    }

    template<typename T>
    T Random(T min, T max)
    {
        float const fMin = static_cast<float>(min);
        float const fMax = static_cast<float>(max);
        float value = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX))
           * (fMax - fMin) + fMin;
        return static_cast<T>(value);

        // For some strange reason this implementation breaks the particle fire
        //static std::uniform_real_distribution<T> distribution(min, max);
        //static std::mt19937 generator {};
        //return distribution(generator);
    }

    [[nodiscard]]
    inline float ACosSafe(float const value)
    {
        if (value <= -1.0f)
        {
            return PiFloat;
        }

        if (value >= 1.0f)
        {
            return 0.0f;
        }
        
        return std::acos(value);
    }

};
