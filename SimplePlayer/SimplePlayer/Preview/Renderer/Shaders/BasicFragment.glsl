        #version 330 core
        in vec3  vtxColor;
        in vec2  vtxTexCoord;
        
        out vec4 FragColor;
        
        uniform sampler2D texture0;
        uniform sampler2D texture1;

        void main()
        {
            // 使用texture1进行颜色采样。
            // 纹理坐标系和OpenGL坐标系相反，因此y坐标取1-vtxTexCoord.y
            FragColor = vec4(vtxColor.rgb, 1.0);//texture(texture0, vec2(vtxTexCoord.x, 1-vtxTexCoord.y)).rgba;
        }