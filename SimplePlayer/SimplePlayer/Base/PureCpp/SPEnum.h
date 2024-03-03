//
//  SPEnum.h
//  SimplePlayer
//
//  Created by YangYixuan on 2023/11/3.
//

namespace sp {

/// 填充模式
enum class EDisplayFillMode
{
    Fit,        // 【默认】适应，缩放至刚好不超出屏幕，可能有空隙。
    Stretch,    // 拉伸，填满屏幕。可能改变长宽比。
    Fill,       // 填充，缩放至刚好填满屏幕。不改变长宽比。
    Origin,     // 保持原始尺寸，不缩放。
};

/// 旋转，逆时针为正方向
enum class EDisplayRotation
{
    Rotation0,
    Rotation90,
    Rotation180,
    Rotation270,
};




}
