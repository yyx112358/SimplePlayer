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
            int posX = int(aPos.x * texWidth), posY = int((1.0 - aPos.y) * texHeight);
            int left = posX / charWidth * charWidth, top = posY / charHeight * charHeight;
            int right = left + charWidth <= texWidth ? left + charWidth : texWidth;
            int bottom = top + charHeight <= texHeight ? top + charHeight : texHeight;
        
            gl_Position = transform * vec4(aPos.x, aPos.y, 0.0, 1.0);

            float sumR = 0, sumG = 0, sumB = 0;
            int x, y;
            for (y = top; y < bottom; y++)
            {
                for (x = left; x < right; x++)
                {
                    vec4 color = texture(texture0, vec2(float(x) / texWidth, float(y) / texHeight));
                    sumR += color.r;
                    sumG += color.g;
                    sumB += color.b;
                }
            }
            int amount = (bottom - top) * (right - left);
            vtxColor = vec3(sumR / amount, sumG / amount, sumB / amount);
            
            vtxTexCoord = aTexCoord;
        }