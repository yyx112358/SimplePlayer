//
//  ImageWriterUIImage.h
//  SimplePlayer
//
//  Created by YangYixuan on 2024/2/2.
//

#pragma once
#include "SPNSObjectHolder.hpp"

SPNSObjectHolder writeRGBA2UIImage(const void* data, const int w, const int h, const int bytesPerPixel, bool swapRB);
