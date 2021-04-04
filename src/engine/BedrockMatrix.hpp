#pragma once

#include "BedrockAssert.hpp"
#include "BedrockMath.hpp"

#include <cmath>
#include <cstring>
#include <iostream>

#ifdef far
#undef far
#endif

#ifdef near
#undef near
#endif

// Like GLM we can have union to access both linear and 2d or eve

// Note: This class is column major from now on
namespace MFA {
//TODO Write unit tests for project
//TODO For specific situation matrices use static methods
template <typename T,unsigned int width,unsigned int height>
class _Matrix {

public:

  static constexpr unsigned int matrixSize = width * height;

  T cells[width * height] = { 0 };

public:

  _Matrix()
  {};

  _Matrix(T defaultValue)
  {
    if (defaultValue != 0) {
      std::fill_n(cells, matrixSize, defaultValue);
    }
  };
  //Use static generator methods instead
  _Matrix(const T& x, const T& y)
  {
    MFA_ASSERT(width == 2);
    MFA_ASSERT(height == 1);
    cells[0] = x;
    cells[1] = y;
  }

  _Matrix(const T& x, const T& y, const T& z)
  {
    MFA_ASSERT(width == 3);
    MFA_ASSERT(height == 1);
    cells[0] = x;
    cells[1] = y;
    cells[2] = z;
  }

  _Matrix(const T x, const T y, const T z, const T w)
  {
    MFA_ASSERT(width == 4);
    MFA_ASSERT(height == 1);
    cells[0] = x;
    cells[1] = y;
    cells[2] = z;
    cells[3] = w;
  }

  template <typename A>
  _Matrix(const uint32_t itemCount, A const * items) {
      MFA_ASSERT(itemCount == width * height);
      for (uint32_t i = 0; i < itemCount; ++i) {
          cells[i] = static_cast<T>(items[i]);
      }
  }

  template <typename A>
  _Matrix<A, width, height>& operator=(const _Matrix<A, width, height>& rhs) = delete;

  static void ExtractRotationAndScaleMatrix(
    _Matrix<float, 4, 4> const & transformMat, 
    _Matrix<float, 4, 4> & destinationMat
  ) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            destinationMat.set(i, j, transformMat.get(i, j));
        }
    }
    destinationMat.set(3, 0, 0.0f);
    destinationMat.set(3, 1, 0.0f);
    destinationMat.set(3, 2, 0.0f);
    destinationMat.set(0, 3, 0.0f);
    destinationMat.set(1, 3, 0.0f);
    destinationMat.set(2, 3, 0.0f);
    destinationMat.set(3, 3, 1.0f);
       
  }

  template <typename A>
  _Matrix(const _Matrix<A, width, height>& other) = delete;

  _Matrix(_Matrix<T,width,height>&& other) noexcept {
    std::memcpy(cells, other.cells, matrixSize * sizeof(T));
  }; // move constructor

  _Matrix& operator=(const _Matrix& other) = delete;// copy assignment
  
  _Matrix& operator=(_Matrix&& other) = delete; // move assignment

  void sum(const _Matrix<T, width, height>& rhs) {
    for (int i = 0; i < matrixSize; i++) {
      cells[i] += rhs.cells[i];
    }
  }

  template <typename A>
  void operator+=(_Matrix<A, width, height> rhs) = delete;

  template <typename A>
  void operator+(_Matrix<A, width, height> rhs) = delete;


  void minus(const _Matrix<T, width, height>& rhs) {
    for (int i = 0; i < matrixSize; i++) {
      cells[i] -= rhs.cells[i];
    }
  }

  template <typename A>
  void operator-=(_Matrix<A,width,height> rhs) = delete;

  template <typename A>
  void operator-(_Matrix<A,width,height> rhs) = delete;

  void multiply(const T& rhs) {
    int i = 0;
    for (i = 0; i < matrixSize; i++) {
      cells[i] *= rhs;
    }
  }

  template <typename A>
  void operator*=(_Matrix<A,width,height> rhs) = delete;

  template <typename A>
  void operator*(_Matrix<A,width,height> rhs) = delete;

    bool equal(const _Matrix<T, width, height>& rhs) {
        return Equal(*this, rhs);
    }

    template <typename funcType, uint32_t funcWidth, uint32_t funcHeight>
    [[nodiscard]]
    static bool Equal(const _Matrix<funcType, funcWidth, funcHeight>& a, const _Matrix<funcType, funcWidth, funcHeight>& b) {
        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                if (a.get(i, j) != b.get(i, j)) {
                  return false;
                }
            }
        }
        return true;
    }

  template <typename A>
  bool operator==(_Matrix<A, width, height>& rhs) = delete;
  
  bool unEqual(const _Matrix<T, width, height>& rhs) {
    return !(this->equal(rhs));
  }

  template <typename A>
  bool operator!=(_Matrix<A,width,height>& rhs) = delete;
  
  void print(char const * matName = "Unknown") const {
    std::cout<<"---Printing matrix----"<<std::endl;
    std::cout<<"MatName:" << matName << std::endl;
    std::cout<<"Width:"<<width<<std::endl;
    std::cout<<"Height:"<<height<<std::endl;
    std::string line = "";
    for (int i = 0; i < height; i++) {
      line = "";
      for (int j = 0; j < width; j++) {
        line += " " + std::to_string(cells[i * width + j]) + " ";
      }
      std::cout<<line<<std::endl;
    }
    std::cout<<"-----------------------"<<std::endl;
  }
  
  const T& get(const unsigned int& x, const unsigned int& y) const {
    MFA_ASSERT(x < width);
    MFA_ASSERT(y < height);
    //return cells[x * height + y];
    return cells[y * width + x];
  }

    void set(const unsigned int& x, const unsigned int& y, const T& value) {
        MFA_ASSERT(x < width);
        MFA_ASSERT(y < height);
        //cells[x * height + y] = value;
        cells[y * width + x] = value;
    }

  const T& getX() const {
    MFA_ASSERT(width == 2 || width == 3 || width == 4);
    MFA_ASSERT(height == 1);
    return cells[0];
  }

  const T& getY() const {
    MFA_ASSERT(width == 2 || width == 3 || width == 4);
    MFA_ASSERT(height == 1);
    return cells[1];
  }

  const T& getZ() const {
    MFA_ASSERT(width == 3 || width == 4);
    MFA_ASSERT(height == 1);
    return cells[2];
  }

  const T& getW() const {
    MFA_ASSERT(width == 4);
    MFA_ASSERT(height == 1);
    return cells[3];
  }

  //TODO Define separate classes for each matrix
  const T& getR() const {
    MFA_ASSERT(width == 3 || width == 4);
    MFA_ASSERT(height == 1);
    return cells[0];
  }

  const T& getG() const {
    MFA_ASSERT(width == 3 || width == 4);
    MFA_ASSERT(height == 1);
    return cells[1];
  }

  const T& getB() const {
    MFA_ASSERT(width == 3 || width == 4);
    MFA_ASSERT(height == 1);
    return cells[2];
  }

  void setX(const T& value) {
    MFA_ASSERT(width == 2 || width == 3 || width == 4);
    MFA_ASSERT(height == 1);
    cells[0] = value;
  }

  void setY(const T& value) {
    MFA_ASSERT(width == 2 || width == 3 || width == 4);
    MFA_ASSERT(height == 1);
    cells[1] = value;
  }

  void setZ(const T& value) {
    MFA_ASSERT(width == 3 || width == 4);
    MFA_ASSERT(height == 1);
    cells[2] = value;
    MFA_ASSERT(std::isnan(cells[2])==false);
  }

  void setW(const T& value) {
    MFA_ASSERT(width == 4);
    MFA_ASSERT(height == 1);
    cells[3] = value;
    MFA_ASSERT(std::isnan(cells[3])==false);
  }

  void setR(const T& value) {
    MFA_ASSERT(width == 3 || width == 4);
    MFA_ASSERT(height == 1);
    cells[0] = value;
    MFA_ASSERT(std::isnan(cells[0])==false);
  }

  void setG(const T& value) {
    MFA_ASSERT(width == 3 || width == 4);
    MFA_ASSERT(height == 1);
    cells[1] = value;
    MFA_ASSERT(std::isnan(cells[1])==false);
  }

  void setB(const T& value) {
    MFA_ASSERT(width == 3 || width == 4);
    MFA_ASSERT(height == 1);
    cells[2] = value;
    MFA_ASSERT(std::isnan(cells[2])==false);
  }

  const T& getDirect(const unsigned int& index) const {
    MFA_ASSERT(index < matrixSize);
    return cells[index];
  }

  void setDirect(const unsigned int& index, const T& value) {
    MFA_ASSERT(index < matrixSize);
    cells[index] = value;
  }

  template<typename A, typename B>
  void setXY(const A& x, const B& y) {
    MFA_ASSERT(width >= 2);
    MFA_ASSERT(height == 1);
    cells[0] = T(x);
    cells[1] = T(y);
    MFA_ASSERT(std::isnan(cells[0])==false);
    MFA_ASSERT(std::isnan(cells[1])==false);
  }

  template<typename A, typename B,typename C>
  void setXYZ(const A& x, const B& y,const C& z) {
    MFA_ASSERT(width >= 3);
    MFA_ASSERT(height == 1);
    cells[0] = T(x);
    cells[1] = T(y);
    cells[2] = T(z);
    MFA_ASSERT(std::isnan(cells[0])==false);
    MFA_ASSERT(std::isnan(cells[1])==false);
    MFA_ASSERT(std::isnan(cells[2])==false);
  }

  template<typename A, typename B, typename C,typename D>
  void setXYZW(const A& x, const B& y, const C& z,const D& w) {
    MFA_ASSERT(width == 4);
    MFA_ASSERT(height == 1);
    cells[0] = T(x);
    cells[1] = T(y);
    cells[2] = T(z);
    cells[3] = T(w);
    MFA_ASSERT(std::isnan(cells[0])==false);
    MFA_ASSERT(std::isnan(cells[1])==false);
    MFA_ASSERT(std::isnan(cells[2])==false);
    MFA_ASSERT(std::isnan(cells[3])==false);
  }

  template<typename A>
  A squareSize() const {
    return A(
      cells[0] * cells[0] +
      cells[1] * cells[1] +
      cells[2] * cells[2]
    );
  }

  template<typename A>
  A size() const {
    return sqrt(squareSize<A>());
  }

  // working correctly
  static void AssignOrthographicProjection(
    _Matrix<T, 4, 4> & matrix,
    const float left,
    const float right,
    const float top,
    const float bottom,
    const float near,
    const float far
  ) {
    MFA_ASSERT(far != near);
    MFA_ASSERT(right != left);
    MFA_ASSERT(bottom != top);
    matrix.set(0, 0, 2 / (right - left));
    MFA_ASSERT(0 == matrix.get(0, 1));
    MFA_ASSERT(0 == matrix.get(0, 2));
    matrix.set(0, 3, right/(left - right));
    MFA_ASSERT(matrix.get(1, 0) == 0);
    matrix.set(1, 1, 2 / (top - bottom));
    MFA_ASSERT(0 == matrix.get(1, 2));
    matrix.set(1, 3, top/(bottom - top));
    MFA_ASSERT(0 == matrix.get(2, 0));
    MFA_ASSERT(0 == matrix.get(2, 1));
    matrix.set(2, 2, 1 / (near - far));
    matrix.set(2, 3, far/(far - near));
    MFA_ASSERT(0 == matrix.get(3,0));
    MFA_ASSERT(0 == matrix.get(3,1));
    MFA_ASSERT(0 == matrix.get(3,2));
    matrix.set(3,3,1);
  }
  // Broken
  static void AssignPerspectiveProjection(
    _Matrix<T, 4, 4> & matrix,
    float const left,
    float const right,
    float const top,
    float const bottom,
    float const near,
    float const far
  ) {
    MFA_ASSERT(far != near);
    MFA_ASSERT(right != left);
    MFA_ASSERT(bottom != top);
    matrix.set(0, 0, 2 / (right - left));
    MFA_ASSERT(0 == matrix.get(0, 1));
    MFA_ASSERT(0 == matrix.get(0, 2));
    matrix.set(0, 3, right/(left - right));
    MFA_ASSERT(matrix.get(1, 0) == 0);
    matrix.set(1, 1, 2 / (top - bottom));
    MFA_ASSERT(0 == matrix.get(1, 2));
    matrix.set(1, 3, top/(bottom - top));
    MFA_ASSERT(0 == matrix.get(2, 0));
    MFA_ASSERT(0 == matrix.get(2, 1));
    matrix.set(2, 2, 1 / (near - far));
    matrix.set(2, 3, far/(far - near));
    MFA_ASSERT(0 == matrix.get(3,0));
    MFA_ASSERT(0 == matrix.get(3,1));
    MFA_ASSERT(0 == matrix.get(3,2));
    matrix.set(3,3,1);
  }
  // Working correctly
  static void PreparePerspectiveProjectionMatrix(
    _Matrix<T, 4, 4> & matrix,
    float const aspect_ratio,
    float const field_of_view,
    float const near_plane,
    float const far_plane 
  ) {
    float const inv_tan = 1.0f / tan(Math::Deg2Rad( 0.5f * field_of_view ));
    float const inv_depth_diff = 1.0f / (near_plane - far_plane);

    matrix.set(0,0,inv_tan);
    matrix.set(1,1,inv_tan * aspect_ratio);
    matrix.set(2,2,1.0f * far_plane * inv_depth_diff);
    matrix.set(2,3,1.0f * near_plane * far_plane * inv_depth_diff);
    matrix.set(3,2,-1.0f);
  }

  static void AssignScale(
    _Matrix<T, 4, 4>& matrix,
    const float value
  ) {
      AssignScale(matrix, value, value, value);
  }

  static void AssignScale(
    _Matrix<T, 4, 4>& matrix,
    const float valueX,
    const float valueY,
    const float valueZ
  ) {
    matrix.set(0, 0, valueX);
    MFA_ASSERT(matrix.get(0, 1) == 0);
    MFA_ASSERT(matrix.get(0, 2) == 0);
    MFA_ASSERT(matrix.get(1, 0) == 0);
    matrix.set(1, 1, valueY);
    MFA_ASSERT(matrix.get(1, 2) == 0);
    MFA_ASSERT(matrix.get(2, 0) == 0);
    MFA_ASSERT(matrix.get(2, 1) == 0);
    matrix.set(2, 2, valueZ);
    MFA_ASSERT(matrix.get(0, 3) == 0);
    MFA_ASSERT(matrix.get(1, 3) == 0);
    MFA_ASSERT(matrix.get(2, 3) == 0);
    matrix.set(3, 3, 1);
    MFA_ASSERT(matrix.get(3, 0) == 0);
    MFA_ASSERT(matrix.get(3, 1) == 0);
    MFA_ASSERT(matrix.get(3, 2) == 0);
  }

  static void AssignTranslation(
    _Matrix<T,4,4>& matrix,
    const T& x,
    const T& y,
    const T& z
  ) {
    matrix.set(0, 0, T(1));
    MFA_ASSERT(matrix.get(0, 1) == 0);
    MFA_ASSERT(matrix.get(0, 2) == 0);
    matrix.set(0, 3, x);
    MFA_ASSERT(matrix.get(1, 0) == 0);
    matrix.set(1, 1, 1);
    MFA_ASSERT(matrix.get(1, 2) == 0);
    matrix.set(1, 3, y);
    MFA_ASSERT(matrix.get(2, 0) == 0);
    MFA_ASSERT(matrix.get(2, 1) == 0);
    matrix.set(2, 2, 1);
    matrix.set(2, 3, z);
    MFA_ASSERT(matrix.get(3, 0) == 0);
    MFA_ASSERT(matrix.get(3, 1) == 0);
    MFA_ASSERT(matrix.get(3, 2) == 0);
    matrix.set(3, 3, T(1));
  }

  // https://www.brainvoyager.com/bv/doc/UsersGuide/CoordsAndTransforms/SpatialTransformationMatrices.html
  static void AssignRotationX(_Matrix<T, 4, 4>& matrix, const T& degree) {
    matrix.set(0, 0, 1);
    MFA_ASSERT(matrix.get(0, 1) == 0);
    MFA_ASSERT(matrix.get(0, 2) == 0);
    MFA_ASSERT(matrix.get(1, 0) == 0);
    matrix.set(1, 1, cosf(degree));
    matrix.set(1, 2, sinf(degree));
    MFA_ASSERT(matrix.get(2, 0) == 0);
    matrix.set(2, 1, -sinf(degree));
    matrix.set(2, 2, cosf(degree));
    MFA_ASSERT(matrix.get(3, 0) == 0.0f);
    MFA_ASSERT(matrix.get(3, 1) == 0.0f);
    MFA_ASSERT(matrix.get(3, 2) == 0.0f);
    MFA_ASSERT(matrix.get(0, 3) == 0.0f);
    MFA_ASSERT(matrix.get(1, 3) == 0.0f);
    MFA_ASSERT(matrix.get(2, 3) == 0.0f);
    matrix.set(3, 3, 1.0f);
  }

  static void AssignRotationY(_Matrix<T, 4, 4>& matrix, const T& degree) {
    matrix.set(0, 0, cosf(degree));
    MFA_ASSERT(matrix.get(0, 1) == 0);
    matrix.set(0, 2, sinf(degree));
    MFA_ASSERT(matrix.get(1, 0) == 0);
    matrix.set(1, 1, 1);
    MFA_ASSERT(matrix.get(1, 2) == 0);
    matrix.set(2, 0, -sinf(degree));
    MFA_ASSERT(matrix.get(2, 1) == 0);
    matrix.set(2, 2, cosf(degree));
    MFA_ASSERT(matrix.get(3, 0) == 0.0f);
    MFA_ASSERT(matrix.get(3, 1) == 0.0f);
    MFA_ASSERT(matrix.get(3, 2) == 0.0f);
    MFA_ASSERT(matrix.get(0, 3) == 0.0f);
    MFA_ASSERT(matrix.get(1, 3) == 0.0f);
    MFA_ASSERT(matrix.get(2, 3) == 0.0f);
    matrix.set(3, 3, 1.0f);
  }

  static void AssignRotationZ(_Matrix<T, 4, 4>& matrix, const T& degree) {
    matrix.set(0, 0, cosf(degree));
    matrix.set(0, 1, -sinf(degree));
    MFA_ASSERT(matrix.get(0, 2) == 0);
    matrix.set(1, 0, sinf(degree));
    matrix.set(1, 1, cosf(degree));
    MFA_ASSERT(matrix.get(1, 2) == 0);
    MFA_ASSERT(matrix.get(2, 0) == 0);
    MFA_ASSERT(matrix.get(2, 1) == 0);
    matrix.set(2, 2, 1);
    MFA_ASSERT(matrix.get(3, 0) == 0.0f);
    MFA_ASSERT(matrix.get(3, 1) == 0.0f);
    MFA_ASSERT(matrix.get(3, 2) == 0.0f);
    MFA_ASSERT(matrix.get(0, 3) == 0.0f);
    MFA_ASSERT(matrix.get(1, 3) == 0.0f);
    MFA_ASSERT(matrix.get(2, 3) == 0.0f);
    matrix.set(3, 3, 1.0f);
  }

    // Based on https://automaticaddison.com/how-to-convert-a-quaternion-to-a-rotation-matrix/ and GLM source
    static void AssignRotation(
        _Matrix<T, 4, 4>& matrix,
        const _Matrix<T, 4, 1>& quaternion
    ) {
        T const qw = quaternion.get(0, 0);
        T const qx = quaternion.get(1, 0);
        T const qy = quaternion.get(2, 0);
        T const qz = quaternion.get(3, 0);

        T qxx(qx * qx);
        T qyy(qy * qy);
        T qzz(qz * qz);
        T qxz(qx * qz);
        T qxy(qx * qy);
        T qyz(qy * qz);
        T qwx(qw * qx);
        T qwy(qw * qy);
        T qwz(qw * qz);

        matrix.set(0, 0, T(1) - T(2) * (qyy +  qzz));
        matrix.set(0, 1, T(2) * (qxy + qwz));
        matrix.set(0, 2, T(2) * (qxz - qwy));

        matrix.set(1, 0, T(2) * (qxy - qwz));
        matrix.set(1, 1, T(1) - T(2) * (qxx +  qzz));
        matrix.set(1, 2, T(2) * (qyz + qwx));

        matrix.set(2, 0, T(2) * (qxz + qwy));
        matrix.set(2, 1, T(2) * (qyz - qwx));
        matrix.set(2, 2, T(1) - T(2) * (qxx +  qyy));

        MFA_ASSERT(matrix.get(0, 3) == 0.0f);
        MFA_ASSERT(matrix.get(1, 3) == 0.0f);
        MFA_ASSERT(matrix.get(2, 3) == 0.0f);
        MFA_ASSERT(matrix.get(3, 0) == 0.0f);
        MFA_ASSERT(matrix.get(3, 1) == 0.0f);
        MFA_ASSERT(matrix.get(3, 2) == 0.0f);

        matrix.set(3, 3, 1.0f);
    }

    static void AssignRotation(
        _Matrix<T, 4, 4>& matrix,
        const _Matrix<T, 3, 1>& eulerAnglesInRad
    ) {
        AssignRotation(matrix, ToQuaternion(eulerAnglesInRad));
    }

    static void AssignRotation(
        _Matrix<T, 4, 4>& matrix,
        const T& xDegree,
        const T& yDegree,
        const T& zDegree
    ) {
        _Matrix<T, 3, 1> rotationMat {};
        rotationMat.set(0, 0, xDegree);
        rotationMat.set(1, 0, yDegree);
        rotationMat.set(2, 0, zDegree);
        AssignRotation(matrix, rotationMat);
        /*matrix.set(0, 0, cosf(yDegree) * cosf(zDegree));
        matrix.set(0, 1, cosf(yDegree) * (-sinf(zDegree)));
        matrix.set(0, 2, -sinf(yDegree));
        matrix.set(1, 0, ((-sinf(xDegree)) * sinf(yDegree) * cosf(zDegree)) + (cosf(xDegree) * sinf(zDegree)));
        matrix.set(1, 1, (sinf(xDegree) * sinf(yDegree) * sinf(zDegree)) + (cosf(xDegree) * cosf(zDegree)));
        matrix.set(1, 2, (-sinf(xDegree)) * cosf(yDegree));
        matrix.set(2, 0, (cosf(xDegree) * sinf(yDegree) * cosf(zDegree)) + (sinf(xDegree) * sinf(zDegree)));
        matrix.set(2, 1, (cosf(xDegree) * sinf(yDegree) * (-1 * sinf(zDegree))) + (sinf(xDegree) * cosf(zDegree)));
        matrix.set(2, 2, cosf(xDegree) * cosf(yDegree));
        MFA_ASSERT(matrix.get(3, 0) == 0.0f);
        MFA_ASSERT(matrix.get(3, 1) == 0.0f);
        MFA_ASSERT(matrix.get(3, 2) == 0.0f);
        MFA_ASSERT(matrix.get(0, 3) == 0.0f);
        MFA_ASSERT(matrix.get(1, 3) == 0.0f);
        MFA_ASSERT(matrix.get(2, 3) == 0.0f);
        matrix.set(3, 3, 1.0f);*/
    }

  // https://www.brainvoyager.com/bv/doc/UsersGuide/CoordsAndTransforms/SpatialTransformationMatrices.html
  static void AssignRotationX(_Matrix<T, 3, 3>& matrix, const T& degree) {
    matrix.set(0, 0, 1);
    MFA_ASSERT(matrix.get(0, 1) == 0);
    MFA_ASSERT(matrix.get(0, 2) == 0);
    MFA_ASSERT(matrix.get(1, 0) == 0);
    matrix.set(1, 1, cosf(degree));
    matrix.set(1, 2, sinf(degree));
    MFA_ASSERT(matrix.get(2, 0) == 0);
    matrix.set(2, 1, -sinf(degree));
    matrix.set(2, 2, cosf(degree));
  }

  static void AssignRotationY(_Matrix<T, 3, 3>& matrix, const T& degree) {
    matrix.set(0, 0, cosf(degree));
    MFA_ASSERT(matrix.get(0, 1) == 0);
    matrix.set(0, 2, sinf(degree));
    MFA_ASSERT(matrix.get(1, 0) == 0);
    matrix.set(1, 1, 1);
    MFA_ASSERT(matrix.get(1, 2) == 0);
    matrix.set(2, 0, -sinf(degree));
    MFA_ASSERT(matrix.get(2, 1) == 0);
    matrix.set(2, 2, cosf(degree));
  }

  static void AssignRotationZ(_Matrix<T,3,3>& matrix, const T& degree) {
    matrix.set(0, 0, cosf(degree));
    matrix.set(0, 1, -sinf(degree));
    MFA_ASSERT(matrix.get(0, 2) == 0);
    matrix.set(1, 0, sinf(degree));
    matrix.set(1, 1, cosf(degree));
    MFA_ASSERT(matrix.get(1, 2) == 0);
    MFA_ASSERT(matrix.get(2, 0) == 0);
    MFA_ASSERT(matrix.get(2, 1) == 0);
    matrix.set(2, 2, 1);
  }

  static void AssignRotation(
    _Matrix<T,3,3>& matrix,
    const T& xDegree,
    const T& yDegree,
    const T& zDegree
  ) {
    matrix.set(0, 0, cosf(yDegree) * cosf(zDegree));
    matrix.set(0, 1, cosf(yDegree) * (-sinf(zDegree)));
    matrix.set(0, 2, -sinf(yDegree));
    matrix.set(1, 0, ((-sinf(xDegree)) * sinf(yDegree) * cosf(zDegree)) + (cosf(xDegree) * sinf(zDegree)));
    matrix.set(1, 1, (sinf(xDegree) * sinf(yDegree) * sinf(zDegree)) + (cosf(xDegree) * cosf(zDegree)));
    matrix.set(1, 2, (-sinf(xDegree)) * cosf(yDegree));
    matrix.set(2, 0, (cosf(xDegree) * sinf(yDegree) * cosf(zDegree)) + (sinf(xDegree) * sinf(zDegree)));
    matrix.set(2, 1, (cosf(xDegree) * sinf(yDegree) * (-1 * sinf(zDegree))) + (sinf(xDegree) * cosf(zDegree)));
    matrix.set(2, 2, cosf(xDegree) * cosf(yDegree));
  }

  static void AssignScale(
    _Matrix<T,3,3>& matrix,
    const float& value
  ) {
    matrix.set(0, 0, matrix.get(0, 0) + value);
    MFA_ASSERT(matrix.get(0, 1) == 0);
    MFA_ASSERT(matrix.get(0, 2) == 0);
    MFA_ASSERT(matrix.get(1, 0) == 0);
    matrix.set(1, 1, matrix.get(1, 1) + value);
    MFA_ASSERT(matrix.get(1, 2) + value);
    MFA_ASSERT(matrix.get(2, 0) + value);
    MFA_ASSERT(matrix.get(2, 1) + value);
    matrix.set(2, 2, matrix.get(2, 2) + value);
  }

  static _Matrix<T,width,height> Identity()
  {
    _Matrix<T,width,height> matrix {};
    _Matrix<T,width,height>::identity(matrix);
    return matrix;
  }

  static void identity(_Matrix<T,width,height> & matrix)
  {
    int i = 0;
    int j = 0;
    for(i = 0 ; i < width ; i ++ )
    {
        for(j = 0 ; j < height ; j ++)
        {
            if(i==j)
            {
                matrix.cells[i * height + j] = T(1);
            } else
            {
                matrix.cells[i * height + j] = T(0);
            }
        }
    }    
  }

  //TODO Write unit tests for project
  void crossProduct(const _Matrix<T, 4, 1>& mat1, const _Matrix<T, 4, 1>& mat2) {
    _crossProduct(mat1.cells, 4, 1, mat2.cells, 4, 1);
    cells[3] = T(1);
  }

  void crossProduct(const _Matrix<T, 3, 1>& mat1, const _Matrix<T, 3, 1>& mat2) {
    return _crossProduct(
      mat1.cells, 3, 1,
      mat2.cells, 3, 1
    );
  }

  void hat(_Matrix<T, 4, 1>& matrix) const {
    _hat(matrix.cells, 4, 1);
    matrix.cells[3] = T(1);
  }

  void hat(_Matrix<T, 3, 1>& matrix) const {
    return _hat(matrix.cells, 3, 1);
  }

  template <typename A>
  void castAssign(const _Matrix<A, width, height>& rhs) {
    castAssign(rhs.cells);
    _assign(rhs.cells, width * height);    
  }

    template <typename A>
    void castAssign(const A * cells_) {
        MFA_ASSERT(cells_ != nullptr);
        for (uint32_t i = 0; i < width * height; ++i) {
          cells[i] = static_cast<T>(cells_[i]);    
        }
    }

    template <unsigned int rhsWidth,unsigned int rhsHeight>
    void assign(const _Matrix<T, rhsWidth, rhsHeight>& rhs) {
        if (rhsWidth == width && rhsHeight == height) {
            _assign(rhs.cells, matrixSize);
        }
        else if (rhsWidth == 3 && width == 4 && rhsHeight == 1 && height == 1) {
            _assign(rhs.cells, rhs.matrixSize);
            cells[3] = T(1);
        }
        else if (rhsWidth == 4 && width == 3 && rhsHeight == 1 && height == 1)
        {
            _assign(rhs.cells, rhs.matrixSize);
        }
        else {
            MFA_CRASH("Unhandled assign in matrixTemplate");
        }
    }

    void assign(const T * cells) {
        MFA_ASSERT(cells != nullptr);
        _assign(cells, width * height);
    }

    void assign(const T * cells, uint32_t elementCount) {
        MFA_ASSERT(cells != nullptr);
        MFA_ASSERT(elementCount > 0);
        _assign(cells, elementCount);
    }

    void assign(const T& value) {
        std::fill_n(cells, matrixSize, value);
    }

    [[nodiscard]]
    T dotProduct(const _Matrix<T, 3, 1>& rhs) const {
        return _dotProduct(rhs.cells, 3, 1);
    }

    [[nodiscard]]
    T dotProduct(const _Matrix<T, 4, 1>& rhs) const {
        return _dotProduct(rhs.cells, 4, 1);
    }

    void multiply(
        const _Matrix<T, width, width>& matrix
    ) {
        return _multiply(matrix.cells);
    }

    [[nodiscard]]
    static _Matrix<T, 3, 1> QuaternionToEulerAnglesRadian(_Matrix<T, 4, 1> const& q)
    {
        return _Matrix<T, 3, 1>(Pitch(q), Yaw(q), Roll(q));
    }

    [[nodiscard]]
    static _Matrix<T, 3, 1> QuaternionToEulerAnglesDegree(_Matrix<T, 4, 1> const& q)
    {
        auto const eulerAngleRadian = QuaternionToEulerAnglesRadian(q);
        _Matrix<T, 3, 1> eulerAngleDegree {
            Math::Rad2Deg(eulerAngleRadian.get(0, 0)),
            Math::Rad2Deg(eulerAngleRadian.get(1, 0)),
            Math::Rad2Deg(eulerAngleRadian.get(2, 0))
        };
        return eulerAngleDegree;
    }

    [[nodiscard]]
    static T Roll(_Matrix<T, 4, 1> const& q)
    {
        auto const qw = q.get(0, 0);
        auto const qx = q.get(1, 0);
        auto const qy = q.get(2, 0);
        auto const qz = q.get(3, 0);
        return static_cast<T>(atan2(static_cast<T>(2) * (qx * qy + qw * qz), qw * qw + qx * qx - qy * qy - qz * qz));
    }

    [[nodiscard]]
    static T Pitch(_Matrix<T, 4, 1> const& q)
    {
        auto const qw = q.get(0, 0);
        auto const qx = q.get(1, 0);
        auto const qy = q.get(2, 0);
        auto const qz = q.get(3, 0);
        
        //return T(atan(T(2) * (q.y * q.z + q.w * q.x), q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z));
        T const y = static_cast<T>(2) * (qy * qz + qw * qx);
        T const x = qw * qw - qx * qx - qy * qy + qz * qz;
        // TODO Refactor, Replace epsilon with correct value
        if(Equal(_Matrix<T, 2, 1>(x, y), _Matrix<T, 2, 1>(0))) { //avoid atan2(0,0) - handle singularity - Matiis
            return static_cast<T>(static_cast<T>(2) * atan2(qx, qw));
        }
        return static_cast<T>(atan2(y, x));
    }

    [[nodiscard]]
    static T Yaw(_Matrix<T, 4, 1> const& q)
    {
        auto const qw = q.get(0, 0);
        auto const qx = q.get(1, 0);
        auto const qy = q.get(2, 0);
        auto const qz = q.get(3, 0);
        return asin(Math::Clamp(static_cast<T>(-2) * (qx * qz - qw * qy), static_cast<T>(-1), static_cast<T>(1)));
    }

    [[nodiscard]]
    static _Matrix<T, 4, 1> ToQuaternion(const _Matrix<T, 3, 1> & degreeInRad) { 
        return ToQuaternion(
            degreeInRad.getZ(),
            degreeInRad.getY(),
            degreeInRad.getX()
        );
    }

    [[nodiscard]]
    static _Matrix<T, 4, 1> ToQuaternion(T yaw, T pitch, T roll) // yaw (Z), pitch (Y), roll (X) in rad
    {
        // Abbreviations for the various angular functions
        T cy = static_cast<T>(cos(yaw * 0.5));
        T sy = static_cast<T>(sin(yaw * 0.5));
        T cp = static_cast<T>(cos(pitch * 0.5));
        T sp = static_cast<T>(sin(pitch * 0.5));
        T cr = static_cast<T>(cos(roll * 0.5));
        T sr = static_cast<T>(sin(roll * 0.5));

        _Matrix<T, 4, 1> q {};
        q.set(0, 0, cr * cp * cy + sr * sp * sy);
        q.set(1, 0, sr * cp * cy - cr * sp * sy);
        q.set(2, 0, cr * sp * cy + sr * cp * sy);
        q.set(3, 0, cr * cp * sy - sr * sp * cy);

        return q;
    }

private:
  //Hint rhsHeight == width && rhsWidth == width
  //TODO write tests
  void _multiply(
    const T* rhsCells
  ) {
    T placeholderCells[width * height] = { 0 };
    for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
        auto const cellIndex = j * height + i;
        placeholderCells[cellIndex] = 0;
        for (int k = 0; k < height; k++) {
          placeholderCells[cellIndex] += cells[k * width + i] * T(rhsCells[j * height + k]);
        }
      }
    }
    std::memcpy(cells, placeholderCells, matrixSize * sizeof(T));
  }

  template<typename A>
  T _dotProduct(
    const A* rhsCells,
    const unsigned int& rhsWidth,
    const unsigned int& rhsHeight
  ) const {
    MFA_ASSERT(rhsWidth == 3 || rhsWidth == 4);
    MFA_ASSERT(rhsHeight == 1);
    MFA_ASSERT(width == 3 || width == 4);
    MFA_ASSERT(height == 1);
    return
      T((double(cells[0]) * double(rhsCells[0])) +
        (double(cells[1]) * double(rhsCells[1])) +
        (double(cells[2])) * double(rhsCells[2]));
  }

    //TODO Write unit tests for project
    template<typename A, typename B>
    void _crossProduct(
        const A* mat1Cells, const unsigned int& mat1Width, const unsigned int& mat1Height,
        const B* mat2Cells, const unsigned int& mat2Width, const unsigned int& mat2Height
    ) {
        MFA_ASSERT(mat1Width == 3 || mat1Width == 4);
        MFA_ASSERT(mat1Height == 1);
        MFA_ASSERT(mat2Width == 3 || mat2Width == 4);
        MFA_ASSERT(mat2Height == 1);
        MFA_ASSERT(width == 3 || width == 4);
        MFA_ASSERT(height == 1);
        this->set(0, 0,
          (T(mat1Cells[1]) * T(mat2Cells[2]))
          - (T(mat1Cells[2]) * T(mat2Cells[1]))
        );
        this->set(1, 0,
          (T(mat1Cells[2]) * T(mat2Cells[0]))
          - (T(mat1Cells[0]) * T(mat2Cells[2]))
        );
        this->set(2, 0,
          (T(mat1Cells[0]) * T(mat2Cells[1]))
          - (T(mat1Cells[1]) * T(mat2Cells[0]))
        );
    }

    template<typename A>
    void _hat(A* rhsCells, const unsigned int& rhsWidth, const unsigned int& rhsHeight) const {
        MFA_ASSERT(rhsWidth == 3 || rhsWidth == 4);
        MFA_ASSERT(rhsHeight == 1);
        MFA_ASSERT(width == 3 || width == 4);
        MFA_ASSERT(height == 1);
        const A vectorSize = size<A>();
        for (unsigned short i = 0; i < 3; i++) {
          rhsCells[i] = A(cells[i]) / vectorSize;
        }
    }

    /*
    *
    * Because elementsCount is related to matrixSize and it is a constexpr
    * It cannot be referenced
    * 
    */
    void _assign(const T * rhsCells, const unsigned int& elementsCount) {
        std::memcpy(cells, rhsCells, elementsCount * sizeof(T));
    }

};

using Matrix4X4Int = _Matrix<int, 4, 4>;
using Matrix4X4Float = _Matrix<float, 4, 4>;
static_assert(sizeof(Matrix4X4Float) == sizeof(float[16]));
using Matrix4X4Double = _Matrix<double, 4, 4>;

using Matrix4X1Int = _Matrix<int, 4, 1>;
using Matrix4X1Float = _Matrix<float, 4, 1>;
using Matrix4X1Double = _Matrix<double, 4, 1>;

using Matrix3X1Int = _Matrix<int, 3, 1>;
using Matrix3X1Float = _Matrix<float, 3, 1>;
using Matrix3X1Double = _Matrix<double, 3, 1>;

using Matrix3X3Int = _Matrix<int, 3, 3>;
using Matrix3X3Float = _Matrix<float, 3, 3>;
using Matrix3X3Double = _Matrix<double, 3, 3>;

using Matrix2X1Int = _Matrix<int, 2, 1>;
using Matrix2X1Float = _Matrix<float, 2, 1>;
using Matrix2X1Double = _Matrix<double, 2, 1>;

using Vector2Int = _Matrix<int, 2, 1>;
using Vector2Float = _Matrix<float, 2, 1>;
using Vector2Double = _Matrix<double, 2, 1>;

using Vector3Int = _Matrix<int, 3, 1>;
using Vector3Float = _Matrix<float, 3, 1>;
using Vector3Double = _Matrix<double, 3, 1>;

using Vector4Int = _Matrix<int, 4, 1>;
using Vector4Float = _Matrix<float, 4, 1>;
using Vector4Double = _Matrix<double, 4, 1>;

using QuaternionInt = _Matrix<int, 4, 1>;
using QuaternionFloat = _Matrix<float, 4, 1>;
using QuaternionDouble = _Matrix<double, 4, 1>;

}