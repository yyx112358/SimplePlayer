        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;

        uniform mat4 transform;
        uniform sampler2D texture0;
        uniform sampler2D texture1;
        uniform int texWidth, texHeight;
        uniform int charWidth, charHeight;

        out vec3 vtxColor;
        out vec2 vtxTexCoord;

        void main()
        {
            gl_Position = transform * vec4(aPos.x, aPos.y, 0.0, 1.0);
        
            // 计算采样矩形坐标范围
            int posX = int((aPos.x + 1) / 2 * texWidth), posY = int((1 - (1.0 + aPos.y) / 2) * texHeight);
            if (aTexCoord.x == 1 || aTexCoord.x == 4 || aTexCoord.x == 5) // 靠右顶点，X坐标减去charWidth
                posX -= charWidth;
            if (aTexCoord.x == 2 || aTexCoord.x == 3 || aTexCoord.x == 5) // 靠下顶点，Y坐标减去charHeight
                posY -= charHeight;
            
            int left = posX / charWidth * charWidth, top = posY / charHeight * charHeight;
            int right = left + charWidth <= texWidth ? left + charWidth : texWidth;
            int bottom = top + charHeight <= texHeight ? top + charHeight : texHeight;
            
            // 计算平均灰度
            float sumR = 0, sumG = 0, sumB = 0;
            int xstep = 4, ystep = 4; // 不需要每一个点取值，取一部分就可以
            for (int y = top; y < bottom; y += ystep)
            {
                for (int x = left; x < right; x += xstep)
                {
                    vec4 color = texture(texture0, vec2(float(x) / texWidth, float(y) / texHeight));
                    sumR += color.r;
                    sumG += color.g;
                    sumB += color.b;
                }
            }
            int amount = (bottom - top) * (right - left) / xstep / ystep;
            float gray = 0.299f * sumR / amount + 0.587f * sumG / amount + 0.114 * sumB / amount;
            vtxColor = vec3(gray, gray, gray);
            
            // 计算对应的字符纹理坐标。字符纹理从左到右划分为256个charWidth * charHeight矩形，第n个矩形平均灰度值为n。
            int normGray = int(gray * 255);
            int charTexX = normGray * charWidth, charTexY = 0;
            if (aTexCoord.x == 1 || aTexCoord.x == 4 || aTexCoord.x == 5)
                charTexX += charWidth;
            if (aTexCoord.x == 2 || aTexCoord.x == 3 || aTexCoord.x == 5)
                charTexY += charHeight;
            vtxTexCoord = vec2(float(charTexX) / charWidth / 256, float(charTexY) / charHeight);

        }