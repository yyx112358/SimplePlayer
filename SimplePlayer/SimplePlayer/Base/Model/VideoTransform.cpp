//
//  VideoTransform.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/2/26.
//

#include "VideoTransform.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <string>

using namespace std;
using namespace sp;

std::string to_string(const glm::mat4 &m) {
    char buf[1024];
    snprintf(buf, 1024, "[%f, %f, %f, %f\n %f, %f, %f, %f\n %f, %f, %f, %f\n %f, %f, %f, %f]",
             m[0][0], m[0][1], m[0][2], m[0][3], m[1][0], m[1][1], m[1][2], m[1][3], m[2][0], m[2][1], m[2][2], m[2][3], m[3][0], m[3][1], m[3][2], m[3][3]);
    return buf;
}

IVideoTransform *sp::VideoTransform2D::clone() const {
    return new VideoTransform2D(*this);
}

glm::mat4 sp::VideoTransform2D::toMatrix() const {
    double inWidth = _inSize.width, inHeight = _inSize.height;
    double outWidth = _outSize.width, outHeight = _outSize.height;
    double scaleX = _scaleX, scaleY = _scaleY;
    
    double inDisplayWidth = inWidth, inDisplayHeight = inHeight;
    if (_displayRotation == EDisplayRotation::Rotation90 || _displayRotation == EDisplayRotation::Rotation270)
        std::swap(inDisplayWidth, inDisplayHeight);
    switch(_fillmode) {
        case EDisplayFillMode::Fit:
        default:
            if (float whRatio = inDisplayWidth / inDisplayHeight;outHeight * whRatio * outHeight <= outWidth / whRatio * outWidth) {
                // 若宽高比为whRatio的等高内接矩形（outputHeight = canvasHeight）总面积更小
                scaleX *= outHeight / inDisplayHeight;
                scaleY *= outHeight / inDisplayHeight;
            } else {
                // 若若宽高比为whRatio的等宽内接矩形（outputWidth = canvasWidth）总面积更小
                scaleX *= outWidth / inDisplayWidth;
                scaleY *= outWidth / inDisplayWidth;
            }
            break;
            
        case EDisplayFillMode::Fill:
            if (float whRatio = inDisplayWidth / inDisplayHeight;outHeight * whRatio * outHeight >= outWidth / whRatio * outWidth) {
                // 若宽高比为whRatio的等高内接矩形（outputHeight = canvasHeight）总面积更小
                scaleX *= outHeight / inDisplayHeight;
                scaleY *= outHeight / inDisplayHeight;
            } else {
                // 若若宽高比为whRatio的等宽内接矩形（outputWidth = canvasWidth）总面积更小
                scaleX *= outWidth / inDisplayWidth;
                scaleY *= outWidth / inDisplayWidth;
            }
            break;

        case EDisplayFillMode::Stretch:
            scaleX *= outWidth / inDisplayWidth;
            scaleY *= outHeight / inDisplayHeight;
            break;

        case EDisplayFillMode::Origin:
            scaleX *= 1;
            scaleY *= 1;
            break;
    }
    if (_flipX)
        scaleX *= -1;
    if (_flipY)
        scaleY *= -1;
    
    float rotationAngle = 0;
    switch (_displayRotation) {
        case EDisplayRotation::Rotation90:  rotationAngle = 90;   break;
        case EDisplayRotation::Rotation180: rotationAngle = 180;  break;
        case EDisplayRotation::Rotation270: rotationAngle = 270;  break;
        default: rotationAngle = 0;  break;
    }
    rotationAngle += _freeRotation;
    rotationAngle = glm::radians(rotationAngle);

    // OpenGL是列向量，需要左乘
    glm::mat4 identity = glm::identity<glm::mat4>();
    glm::mat4 m = identity; // 模型矩阵
    m = glm::scale<float>(identity, glm::vec3(inWidth / 2.0f, inHeight / 2.0f, 1.0f)) * m;  // 从(1, 1)缩放到输入纹理的大小
    m = glm::scale<float>(identity, glm::vec3(scaleX, scaleY, 1.0f)) * m; // 缩放
    m = glm::rotate<float>(identity, rotationAngle, glm::vec3(0.0f, 0.0f, 1.0f)) * m; // 旋转
    m = glm::translate<float>(identity, glm::vec3(_transX, _transY, 0.0f)) * m; // 平移
    
    const static glm::mat4 v = glm::lookAt<float>(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 1.0f, 0.0f)); // 视图矩阵
    glm::mat4 p = glm::ortho<float>(-outWidth / 2.0f, outWidth / 2.0f, -outHeight / 2.0f, outHeight / 2.0f, -2.0f, 2.0f); // 投影矩阵，投影到[-1, +1]范围

    m = p * v * m;
    return m;
}

