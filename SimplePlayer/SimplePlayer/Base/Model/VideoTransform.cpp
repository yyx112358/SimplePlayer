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

glm::mat4 sp::VideoTransformFillmode::toMatrix() const {
    float inWidth = inSize.width, inHeight = inSize.height;
    float outWidth = outSize.width, outHeight = outSize.height;
    float scaleX = scale, scaleY = scale;
    
    float inDisplayWidth = inWidth, inDisplayHeight = inHeight;
    if (displayRotation == EDisplayRotation::Rotation90 || displayRotation == EDisplayRotation::Rotation270)
        std::swap(inDisplayWidth, inDisplayHeight);
    switch(fillmode) {
        case EFillMode::Fit:
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
            
        case EFillMode::Fill:
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

        case EFillMode::Stretch:
            scaleX *= outWidth / inDisplayWidth;
            scaleY *= outHeight / inDisplayHeight;
            break;

        case EFillMode::Origin:
            scaleX *= 1;
            scaleY *= 1;
            break;
    }
    if (flipX)
        scaleX *= -1;
    if (flipY)
        scaleY *= -1;
    
    float rotationAngle = 0;
    switch (displayRotation) {
        case EDisplayRotation::Rotation90:  rotationAngle = 90;   break;
        case EDisplayRotation::Rotation180: rotationAngle = 180;  break;
        case EDisplayRotation::Rotation270: rotationAngle = 270;  break;
        default: rotationAngle = 0;  break;
    }
    rotationAngle += freeRotation;
    rotationAngle = glm::radians(rotationAngle);

    // OpenGL是列向量，需要左乘
    glm::mat4 identity = glm::identity<glm::mat4>();
    glm::mat4 m = identity; // 模型矩阵
    m = glm::scale(identity, glm::vec3(inWidth / 2.0f, inHeight / 2.0f, 1.0f)) * m;  // 从(1, 1)缩放到输入纹理的大小
    m = glm::scale(identity, glm::vec3(scaleX, scaleY, 1.0f)) * m; // 缩放
    m = glm::rotate(identity, rotationAngle, glm::vec3(0.0f, 0.0f, 1.0f)) * m; // 旋转
    m = glm::translate(identity, glm::vec3(0.0f, 0.0f, 0.0f)) * m; // 平移
    
    glm::mat4 v = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 1.0f, 0.0f)); // 视图矩阵
    glm::mat4 p = glm::ortho(-outWidth / 2.0f, outWidth / 2.0f, -outHeight / 2.0f, outHeight / 2.0f, -2.0f, 2.0f); // 投影矩阵，投影到[-1, +1]范围

    return p * v * m;
}



glm::mat4 sp::VideoTransformAbs::toMatrix() const {

    glm::mat4 m = glm::identity<glm::mat4>();
    m = glm::translate(m, glm::vec3(transformX, transformY, 1.0f));
    m = glm::scale(m, glm::vec3(scaleX, scaleY, 1.0f));
    m = glm::rotate(m, glm::pi<float>() / 2 * rotation, glm::vec3(0.0f, 0.0f, 1.0f)); // 旋转
    
    return m;
}
