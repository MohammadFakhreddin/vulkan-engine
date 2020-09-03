#ifndef MATRIX_TEMPLATE_CLASS
#define MATRIX_TEMPLATE_CLASS

#include <cassert>
#include <memory>
#include <cmath>
#include <cstring>
#include <iostream>

#include "MMath.h"

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
    assert(width == 2);
    assert(height == 1);
    cells[0] = x;
    cells[1] = y;
  }

  _Matrix(const T& x, const T& y, const T& z)
  {
    assert(width == 3);
    assert(height == 1);
    cells[0] = x;
    cells[1] = y;
    cells[2] = z;
  }

  _Matrix(const T x, const T y, const T z, const T w)
  {
    assert(width == 4);
    assert(height == 1);
    cells[0] = x;
    cells[1] = y;
    cells[2] = z;
    cells[3] = w;
  }

  template <typename A>
  _Matrix<A, width, height>& operator=(const _Matrix<A, width, height>& rhs) = delete;

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
    for (int i = 0; i < width; i++) {
      for (int j = 0; j < height; j++) {
        if (rhs.get(i, j) != get(i, j)) {
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
  
  void print() {
    std::cout<<"---Printing matrix----"<<std::endl;
    std::cout<<"Width:"<<width<<std::endl;
    std::cout<<"Height:"<<height<<std::endl;
    std::string line = "";
    for (int i = 0; i < height; i++) {
      line = "";
      for (int j = 0; j < width; j++) {
        line += " " + std::to_string(cells[i * height + j]) + " ";
      }
      std::cout<<line<<std::endl;
    }
    std::cout<<"-----------------------"<<std::endl;
  }
  
  const T& get(const unsigned int& x, const unsigned int& y) const {
    assert(x < width);
    assert(y < height);
    return cells[x * height + y];
  }

  const T& getX() const {
    assert(width == 2 || width == 3 || width == 4);
    assert(height == 1);
    return cells[0];
  }

  const T& getY() const {
    assert(width == 2 || width == 3 || width == 4);
    assert(height == 1);
    return cells[1];
  }

  const T& getZ() const {
    assert(width == 3 || width == 4);
    assert(height == 1);
    return cells[2];
  }

  const T& getW() const {
    assert(width == 4);
    assert(height == 1);
    return cells[3];
  }

  //TODO Define separate classes for each matrix
  const T& getR() const {
    assert(width == 3 || width == 4);
    assert(height == 1);
    return cells[0];
  }

  const T& getG() const {
    assert(width == 3 || width == 4);
    assert(height == 1);
    return cells[1];
  }

  const T& getB() const {
    assert(width == 3 || width == 4);
    assert(height == 1);
    return cells[2];
  }

  void setX(const T& value) {
    assert(width == 2 || width == 3 || width == 4);
    assert(height == 1);
    cells[0] = value;
  }

  void setY(const T& value) {
    assert(width == 2 || width == 3 || width == 4);
    assert(height == 1);
    cells[1] = value;
  }

  void setZ(const T& value) {
    assert(width == 3 || width == 4);
    assert(height == 1);
    cells[2] = value;
    assert(std::isnan(cells[2])==false);
  }

  void setW(const T& value) {
    assert(width == 4);
    assert(height == 1);
    cells[3] = value;
    assert(std::isnan(cells[3])==false);
  }

  void setR(const T& value) {
    assert(width == 3 || width == 4);
    assert(height == 1);
    cells[0] = value;
    assert(std::isnan(cells[0])==false);
  }

  void setG(const T& value) {
    assert(width == 3 || width == 4);
    assert(height == 1);
    cells[1] = value;
    assert(std::isnan(cells[1])==false);
  }

  void setB(const T& value) {
    assert(width == 3 || width == 4);
    assert(height == 1);
    cells[2] = value;
    assert(std::isnan(cells[2])==false);
  }

  const T& getDirect(const unsigned int& index) const {
    assert(index < matrixSize);
    return cells[index];
  }
  
  void set(const unsigned int& x, const unsigned int& y, const T& value) {
    assert(x < width);
    assert(y < height);
    cells[x * height + y] = value;
  }

  void setDirect(const unsigned int& index, const T& value) {
    assert(index < matrixSize);
    cells[index] = value;
  }

  template<typename A, typename B>
  void setXY(const A& x, const B& y) {
    assert(width >= 2);
    assert(height == 1);
    cells[0] = T(x);
    cells[1] = T(y);
    assert(std::isnan(cells[0])==false);
    assert(std::isnan(cells[1])==false);
  }

  template<typename A, typename B,typename C>
  void setXYZ(const A& x, const B& y,const C& z) {
    assert(width >= 3);
    assert(height == 1);
    cells[0] = T(x);
    cells[1] = T(y);
    cells[2] = T(z);
    assert(std::isnan(cells[0])==false);
    assert(std::isnan(cells[1])==false);
    assert(std::isnan(cells[2])==false);
  }

  template<typename A, typename B, typename C,typename D>
  void setXYZW(const A& x, const B& y, const C& z,const D& w) {
    assert(width == 4);
    assert(height == 1);
    cells[0] = T(x);
    cells[1] = T(y);
    cells[2] = T(z);
    cells[3] = T(w);
    assert(std::isnan(cells[0])==false);
    assert(std::isnan(cells[1])==false);
    assert(std::isnan(cells[2])==false);
    assert(std::isnan(cells[3])==false);
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

  //Based on http://www.songho.ca/opengl/gl_projectionmatrix.html
  static void assignProjection(
    _Matrix<T, 4, 4>& matrix,
    const float & left,
    const float & right,
    const float & top,
    const float & bottom,
    const float & near,
    const float & far
  ) {
    assert(far != near);
    assert(right != left);
    assert(bottom != top);
    matrix.set(0, 0, (2 * near) / (right - left));
    assert(matrix.get(0, 1) == 0);
    matrix.set(0, 2, (right + left) / (right - left));
    assert(matrix.get(0, 3) == 0);
    assert(matrix.get(1, 0) == 0);
    matrix.set(1, 1, (2 * near) / (top - bottom));
    matrix.set(1, 2, (bottom + top) / (top - bottom));
    assert(matrix.get(1, 3) == 0);
    assert(matrix.get(2, 0) == 0);
    assert(matrix.get(2, 1) == 0);
    matrix.set(2, 2, -(far + near) / (far - near));
    matrix.set(2, 3, -(2 * far * near) / (far - near));
    assert(matrix.get(3, 0) == 0);
    assert(matrix.get(3, 1) == 0);
    //matrix.set(3, 2, -1);
    assert(matrix.get(3, 3) == 0);
  }

  //https://github.com/PacktPublishing/Vulkan-Cookbook/blob/master/Library/Source%20Files/10%20Helper%20Recipes/04%20Preparing%20a%20perspective%20projection%20matrix.cpp
  static void preparePerspectiveProjectionMatrix(
    _Matrix<T, 4, 4>& matrix,
    float aspect_ratio,
    float field_of_view,
    float near_plane,
    float far_plane 
  ) {
    auto radian = Math::deg2Rad( 0.5f * field_of_view );
    float f = 1.0f / tan( radian );

    matrix.cells[0] = static_cast<T>(f / aspect_ratio);
    matrix.cells[1] = static_cast<T>(0.0f);
    matrix.cells[2] = static_cast<T>(0.0f);
    matrix.cells[3] = static_cast<T>(0.0f);
    matrix.cells[4] = static_cast<T>(0.0f);
    matrix.cells[5] = static_cast<T>(-f);
    matrix.cells[6] = static_cast<T>(0.0f);
    matrix.cells[7] = static_cast<T>(0.0f);
    matrix.cells[8] = static_cast<T>(0.0f);
    matrix.cells[9] = static_cast<T>(0.0f);
    matrix.cells[10] = static_cast<T>(far_plane / (near_plane - far_plane));
    matrix.cells[11] = static_cast<T>(-1.0f);
    matrix.cells[12] = static_cast<T>(0.0f);
    matrix.cells[13] = static_cast<T>(0.0f);
    matrix.cells[14] = static_cast<T>((near_plane * far_plane) / (near_plane - far_plane));
    matrix.cells[15] = static_cast<T>(0.0f);
  }

  static void assignScale(
    _Matrix<T, 4, 4>& matrix,
    const float& value
  ) {
    matrix.set(0, 0, matrix.get(0, 0) + value);
    assert(matrix.get(0, 1) == 0);
    assert(matrix.get(0, 2) == 0);
    assert(matrix.get(1, 0) == 0);
    matrix.set(1, 1, matrix.get(1, 1) + value);
    assert(matrix.get(1, 2) + value);
    assert(matrix.get(2, 0) + value);
    assert(matrix.get(2, 1) + value);
    matrix.set(2, 2, matrix.get(2, 2) + value);
    assert(matrix.get(0, 3) == 0);
    assert(matrix.get(1, 3) == 0);
    assert(matrix.get(2, 3) == 0);
    matrix.set(3, 3, 1);
    assert(matrix.get(3, 0) == 0);
    assert(matrix.get(3, 1) == 0);
    assert(matrix.get(3, 2) == 0);
  }

  static void assignTransformation(
    _Matrix<T,4,4>& matrix,
    const T& x,
    const T& y,
    const T& z
  ) {
    matrix.set(0, 0, T(1));
    assert(matrix.get(0, 1) == 0);
    assert(matrix.get(0, 2) == 0);
    matrix.set(0, 3, x);
    assert(matrix.get(1, 0) == 0);
    matrix.set(1, 1, 1);
    assert(matrix.get(1, 2) == 0);
    matrix.set(1, 3, y);
    assert(matrix.get(2, 0) == 0);
    assert(matrix.get(2, 1) == 0);
    matrix.set(2, 2, 1);
    matrix.set(2, 3, z);
    assert(matrix.get(3, 0) == 0);
    assert(matrix.get(3, 1) == 0);
    assert(matrix.get(3, 2) == 0);
    matrix.set(3, 3, T(1));
  }

  // https://www.brainvoyager.com/bv/doc/UsersGuide/CoordsAndTransforms/SpatialTransformationMatrices.html
  static void assignRotationX(_Matrix<T, 4, 4>& matrix, const T& degree) {
    matrix.set(0, 0, 1);
    assert(matrix.get(0, 1) == 0);
    assert(matrix.get(0, 2) == 0);
    assert(matrix.get(1, 0) == 0);
    matrix.set(1, 1, cosf(degree));
    matrix.set(1, 2, sinf(degree));
    assert(matrix.get(2, 0) == 0);
    matrix.set(2, 1, -sinf(degree));
    matrix.set(2, 2, cosf(degree));
    assert(matrix.get(3, 0) == 0.0f);
    assert(matrix.get(3, 1) == 0.0f);
    assert(matrix.get(3, 2) == 0.0f);
    assert(matrix.get(0, 3) == 0.0f);
    assert(matrix.get(1, 3) == 0.0f);
    assert(matrix.get(2, 3) == 0.0f);
    matrix.set(3, 3, 1.0f);
  }

  static void assignRotationY(_Matrix<T, 4, 4>& matrix, const T& degree) {
    matrix.set(0, 0, cosf(degree));
    assert(matrix.get(0, 1) == 0);
    matrix.set(0, 2, sinf(degree));
    assert(matrix.get(1, 0) == 0);
    matrix.set(1, 1, 1);
    assert(matrix.get(1, 2) == 0);
    matrix.set(2, 0, -sinf(degree));
    assert(matrix.get(2, 1) == 0);
    matrix.set(2, 2, cosf(degree));
    assert(matrix.get(3, 0) == 0.0f);
    assert(matrix.get(3, 1) == 0.0f);
    assert(matrix.get(3, 2) == 0.0f);
    assert(matrix.get(0, 3) == 0.0f);
    assert(matrix.get(1, 3) == 0.0f);
    assert(matrix.get(2, 3) == 0.0f);
    matrix.set(3, 3, 1.0f);
  }

  static void assignRotationZ(_Matrix<T, 4, 4>& matrix, const T& degree) {
    matrix.set(0, 0, cosf(degree));
    matrix.set(0, 1, -sinf(degree));
    assert(matrix.get(0, 2) == 0);
    matrix.set(1, 0, sinf(degree));
    matrix.set(1, 1, cosf(degree));
    assert(matrix.get(1, 2) == 0);
    assert(matrix.get(2, 0) == 0);
    assert(matrix.get(2, 1) == 0);
    matrix.set(2, 2, 1);
    assert(matrix.get(3, 0) == 0.0f);
    assert(matrix.get(3, 1) == 0.0f);
    assert(matrix.get(3, 2) == 0.0f);
    assert(matrix.get(0, 3) == 0.0f);
    assert(matrix.get(1, 3) == 0.0f);
    assert(matrix.get(2, 3) == 0.0f);
    matrix.set(3, 3, 1.0f);
  }

  static void assignRotationXYZ(
    _Matrix<T, 4, 4>& matrix,
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
    assert(matrix.get(3, 0) == 0.0f);
    assert(matrix.get(3, 1) == 0.0f);
    assert(matrix.get(3, 2) == 0.0f);
    assert(matrix.get(0, 3) == 0.0f);
    assert(matrix.get(1, 3) == 0.0f);
    assert(matrix.get(2, 3) == 0.0f);
    matrix.set(3, 3, 1.0f);
  }

  // https://www.brainvoyager.com/bv/doc/UsersGuide/CoordsAndTransforms/SpatialTransformationMatrices.html
  static void assignRotationX(_Matrix<T, 3, 3>& matrix, const T& degree) {
    matrix.set(0, 0, 1);
    assert(matrix.get(0, 1) == 0);
    assert(matrix.get(0, 2) == 0);
    assert(matrix.get(1, 0) == 0);
    matrix.set(1, 1, cosf(degree));
    matrix.set(1, 2, sinf(degree));
    assert(matrix.get(2, 0) == 0);
    matrix.set(2, 1, -sinf(degree));
    matrix.set(2, 2, cosf(degree));
  }

  static void assignRotationY(_Matrix<T, 3, 3>& matrix, const T& degree) {
    matrix.set(0, 0, cosf(degree));
    assert(matrix.get(0, 1) == 0);
    matrix.set(0, 2, sinf(degree));
    assert(matrix.get(1, 0) == 0);
    matrix.set(1, 1, 1);
    assert(matrix.get(1, 2) == 0);
    matrix.set(2, 0, -sinf(degree));
    assert(matrix.get(2, 1) == 0);
    matrix.set(2, 2, cosf(degree));
  }

  static void assignRotationZ(_Matrix<T,3,3>& matrix, const T& degree) {
    matrix.set(0, 0, cosf(degree));
    matrix.set(0, 1, -sinf(degree));
    assert(matrix.get(0, 2) == 0);
    matrix.set(1, 0, sinf(degree));
    matrix.set(1, 1, cosf(degree));
    assert(matrix.get(1, 2) == 0);
    assert(matrix.get(2, 0) == 0);
    assert(matrix.get(2, 1) == 0);
    matrix.set(2, 2, 1);
  }

  static void assignRotationXYZ(
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

  static void assignScale(
    _Matrix<T,3,3>& matrix,
    const float& value
  ) {
    matrix.set(0, 0, matrix.get(0, 0) + value);
    assert(matrix.get(0, 1) == 0);
    assert(matrix.get(0, 2) == 0);
    assert(matrix.get(1, 0) == 0);
    matrix.set(1, 1, matrix.get(1, 1) + value);
    assert(matrix.get(1, 2) + value);
    assert(matrix.get(2, 0) + value);
    assert(matrix.get(2, 1) + value);
    matrix.set(2, 2, matrix.get(2, 2) + value);
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
      //Logger::exception("Unhandled assign in matrixTemplate");
    }
  }

  void assign(const T& value) {
    std::fill_n(cells, matrixSize, value);
  }

  T dotProduct(const _Matrix<T, 3, 1>& rhs) const {
    return _dotProduct(rhs.cells, 3, 1);
  }

  T dotProduct(const _Matrix<T, 4, 1>& rhs) const {
    return _dotProduct(rhs.cells, 4, 1);
  }

  void multiply(
    const _Matrix<T, width, width>& matrix
  ) {
    return _multiply(matrix.cells);
  }

private:
  //Hint rhsHeight == width && rhsWidth == width
  //TODO write tests
  void _multiply(
    const T* rhsCells
  ) {
    T placeholderCells[width * height] = { 0 };
    int index;
    int i;
    int j;
    int k;
    int rhsRowValue;
    int rowValue;
    for (i = 0; i < width; i++) {
      rhsRowValue = i * width;
      rowValue = i * height;
      for (j = 0; j < height; j++) {
        index = rowValue + j;
        placeholderCells[rowValue] = 0;
        for (k = 0; k < width; k++) {
          placeholderCells[rowValue] += cells[k * height + j] * T(rhsCells[rhsRowValue + k]);
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
    assert(rhsWidth == 3 || rhsWidth == 4);
    assert(rhsHeight == 1);
    assert(width == 3 || width == 4);
    assert(height == 1);
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
    assert(mat1Width == 3 || mat1Width == 4);
    assert(mat1Height == 1);
    assert(mat2Width == 3 || mat2Width == 4);
    assert(mat2Height == 1);
    assert(width == 3 || width == 4);
    assert(height == 1);
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
    assert(rhsWidth == 3 || rhsWidth == 4);
    assert(rhsHeight == 1);
    assert(width == 3 || width == 4);
    assert(height == 1);
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

#endif