1. 安装Visual Studio 2008到C盘默认路径，

  1.1 安装Visual Studio 2008 SP1---非常重要，否则编译出来的版本会崩溃。

  1.2 安装Windows SDK，推荐的版本是Windows 7 SDK

  1.3 安装DirectX SDK，推荐的版本是DXSDK_Jun10.exe

  1.4 文件夹里面有好些乱七八糟的无关文件，可无视，也可能在里面寻宝，比如好几个版本的MVC3D解码器？

  1.5 作者的操作系统是Windows7 x64 SP1，如果有不明编译问题可尝试

2. 将yasm.exe拷贝到C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin

3. 编译BaseClasses的Release版本

4. 以debug 编译一下renderer_prototype整个解决方案， 会报若干错，无视它，只要shader编译正常即可

5. 以Release编译一下report_server整个解决方案.

6. 以release filter编译一下my12doomSource的BaseVideoFilter子项目

7. 运行build_bo3d_oem2.bat （其他几个版本的批处理未修复，可自行修复）

  7.1 有时需要再运行一下bat