//
//  VideoTransform.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/2/26.
//

#include "VideoTransform.hpp"
#include "glm/gtc/matrix_transform.hpp"

using namespace sp;

glm::mat4 sp::VideoTransformFillmode::toMatrix() const {
    float inWidth = inSize.width, inHeight = inSize.height;
    float outWidth = outSize.width, outHeight = outSize.height;
    if (displayRotation == EDisplayRotation::Rotation90 || displayRotation == EDisplayRotation::Rotation270)
        std::swap(inWidth, inHeight);
    float scaleX = scale, scaleY = scale;
    switch(fillmode) {
        case EFillMode::Fit:
        default:
            if (float scale = outWidth / inWidth; inHeight * scale <= outHeight) {
                scaleX = scale;
                scaleY = scale;
            } else {
                scaleX = outHeight / inHeight;
                scaleY = outHeight / inHeight;
            }
            break;
            
        case EFillMode::Stretch:
            scaleX = outWidth / inWidth;
            scaleY = outHeight / inHeight;
            break;
            
        case EFillMode::Fill:
            if (float scale = outWidth / inWidth; inHeight * scale >= outHeight) {
                scaleX = scale;
                scaleY = scale;
            } else {
                scaleX = outHeight / inHeight;
                scaleY = outHeight / inHeight;
            }
            break;
            
        case EFillMode::Origin:
            scaleX = 1;
            scaleY = 1;
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
    
    glm::mat4 m = glm::identity<glm::mat4>();
    m = glm::scale(m, glm::vec3(scaleX, scaleY, 1.0f));   // 缩放到preview区域内
    m = glm::scale(m, glm::vec3(inWidth / outWidth, inHeight / outHeight, 1.0f)); // 归一化
    m = glm::rotate(m, glm::pi<float>() / 2 * (int)rotationAngle, glm::vec3(0.0f, 0.0f, 1.0f)); // 旋转
    
    return m;
}



glm::mat4 sp::VideoTransformAbs::toMatrix() const {

    glm::mat4 m = glm::identity<glm::mat4>();
    m = glm::translate(m, glm::vec3(transformX, transformY, 1.0f));
    m = glm::scale(m, glm::vec3(scaleX, scaleY, 1.0f));
    m = glm::rotate(m, glm::pi<float>() / 2 * rotation, glm::vec3(0.0f, 0.0f, 1.0f)); // 旋转
    
    return m;
}
