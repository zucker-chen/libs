
# SDL 
SDL（Simple DirectMedia Layer）是一套开放源代码的跨平台多媒体开发库，使用C语言写成。  
SDL提供了数种控制图像、声音、输出入的函数，让开发者只要用相同或是相似的代码就可以开发出跨多个平台（Linux、Windows、Mac OS X等）的应用软件。目前SDL多用于开发游戏、模拟器、媒体播放器等多媒体应用领域。
本用例主要用其处理TTF字体显示功能（OSD）

## 下载安装编译方法
`sh build-SDL.sh`  --> libsdl库编译(SDL基础库, libsdl_ttf编译需要)  
`sh build-freetype.sh`  --> libfreetype库编译(libsdl_ttf需要)  
`sh build-SDL_ttf.sh`  --> libsdl_ttf库编译(需要最后编译，依赖libsdl libfreetype库)  
默认pc编译，如果要交叉编译则修改脚本中：    
`enable_cross_compile="enable"`  

## 交叉编译选项配置
`sh configure --host=arm-linux CC=arm-linux-uclibcgnueabi-gcc --prefix=$(pwd)/build`  

## 测试用用例使用说明"sh test/make.sh"
```
# make.sh内容
ext_dir="ext"
# SDL
tmp_dir=$ext_dir/sdl/include
[ ! -d $tmp_dir ] && mkdir -p $tmp_dir && cp -r ../SDL-*/build/include/SDL2/* $tmp_dir && cp -r ../SDL_*/build/include/SDL2/*.h $tmp_dir
tmp_dir=$ext_dir/sdl/lib
[ ! -d $tmp_dir ] && mkdir -p $tmp_dir  && cp ../SDL-*/build/lib/*.a $tmp_dir && cp ../SDL_*/build/lib/*.a $tmp_dir
# freetype
tmp_dir=$ext_dir/freetype/include
[ ! -d $tmp_dir ] && mkdir -p $tmp_dir && cp -r ../freetype2*/build/include/freetype2/* $tmp_dir
tmp_dir=$ext_dir/freetype/lib
[ ! -d $tmp_dir ] && mkdir -p $tmp_dir && cp ../freetype2*/build/lib/*.a $tmp_dir
# cmake build...
```

## 运行
```
test/build/ttf_test
生成 save.bmp图片，图片内容是"Hello 世界！"
```
