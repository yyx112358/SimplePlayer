//
//  ImageWriterBmp.h
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/29.
//

#ifndef ImageWriterBmp_h
#define ImageWriterBmp_h


extern "C" {

bool writeBMP2File(const char* filename, const void* data, const int w, const int h, const int bytesPerPixel);

}


#endif /* ImageWriterBmp_h */
