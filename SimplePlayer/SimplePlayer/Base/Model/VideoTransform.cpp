//
//  VideoTransform.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/2/26.
//

#include "VideoTransform.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <string>

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
//    if (displayRotation == EDisplayRotation::Rotation90 || displayRotation == EDisplayRotation::Rotation270)
//        std::swap(inWidth, inHeight);
    float scaleX = scale, scaleY = scale;
    switch(fillmode) {
        case EFillMode::Fit:
        default:
            if (float whRatio = outWidth / inWidth; inHeight * whRatio <= outHeight) {
                scaleX *= whRatio;
                scaleY *= whRatio;
            } else {
                scaleX = outHeight / inHeight;
                scaleY = outHeight / inHeight;
            }
            break;
            
        case EFillMode::Stretch:
            scaleX *= outWidth / inWidth;
            scaleY *= outHeight / inHeight;
            break;
            
        case EFillMode::Fill:
            if (float whRatio = outWidth / inWidth; inHeight * whRatio >= outHeight) {
                scaleX *= whRatio;
                scaleY *= whRatio;
            } else {
                scaleX *= outHeight / inHeight;
                scaleY *= outHeight / inHeight;
            }
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

    glm::mat4 m = glm::identity<glm::mat4>();
    m = glm::translate(m, glm::vec3(0.0f, 0.0f, 0.0f)); // 平移
    // 这里为什么要变换到1:1没理解
    m = glm::scale(m, glm::vec3(inHeight / inWidth, 1.0f, 1.0f));   // 变换为1:1
    std::string s = to_string(m);
    m = glm::rotate(m, rotationAngle, glm::vec3(0.0f, 0.0f, 1.0f)); // 旋转
    s = to_string(m);
    m = glm::scale(m, glm::vec3(inWidth / inHeight, 1.0f, 1.0f));   // 变回原比例
    s = to_string(m);
    m = glm::scale(m, glm::vec3(scaleX, scaleY, 1.0f));   // 缩放到preview区域内
    m = glm::scale(m, glm::vec3(inWidth / outWidth, inHeight / outHeight, 1.0f)); // 归一化
//    m = glm::scale(m, glm::vec3(inWidth, inHeight, 1.0f));
    
    glm::mat4 v = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 1.0f, 0.0f));
    glm::mat4 p = glm::ortho(-outWidth / 2, outWidth / 2, -outHeight / 2, outHeight / 2, -2.0f, 2.0f);
//    m = m * v * p;
    
//    printf("[%2f, %2f, %2f, %f2\n %2f, %2f, %2f, %2f\n %2f, %2f, %2f, %2f\n %2f, %2f, %2f, %2f]",
//             m[0][0], m[0][1], m[0][2], m[0][3], m[1][0], m[1][1], m[1][2], m[1][3], m[2][0], m[2][1], m[2][2], m[2][3], m[3][0], m[3][1], m[3][2], m[3][3]);
    return m;
}



glm::mat4 sp::VideoTransformAbs::toMatrix() const {

    glm::mat4 m = glm::identity<glm::mat4>();
    m = glm::translate(m, glm::vec3(transformX, transformY, 1.0f));
    m = glm::scale(m, glm::vec3(scaleX, scaleY, 1.0f));
    m = glm::rotate(m, glm::pi<float>() / 2 * rotation, glm::vec3(0.0f, 0.0f, 1.0f)); // 旋转
    
    return m;
}
