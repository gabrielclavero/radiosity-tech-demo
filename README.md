# README

## radiosity-tech-demo

A technical demonstration of the radiosity algorithm achieved with GPGPU computing, Direct3D 11 and C++. 

### 1 Requirements
#### 1.1 To run:

Windows 7  
Visual C++ Redistributable for Visual Studio 2012 (x86) [http://www.microsoft.com/en-us/download/details.aspx?id=30679](http://www.microsoft.com/en-us/download/details.aspx?id=30679)  
Other required DLLs are in the Bin folder. Therefore the program must be run from that location or copy the DLLs in C:\Windows\SysWOW64  
Compatible videocard with Directx 11 or greater.  

#### 1.2 To compile:

Windows 7  
Visual Studio 2012  
Windows SDK [http://msdn.microsoft.com/en-us/windows/desktop/bg162891.aspx](http://msdn.microsoft.com/en-us/windows/desktop/bg162891.aspx)  
    
### 2 Directories
#### 2.1 Bin

Program executable  
Required dynamic libraries that are not shipped with Windows 7  
Assets directory with shaders and test scenes  
    
#### 2.2 Dependencies

Libraries and .h files required to compile the program and that are not shipped with Windows SDK.
    
#### 2.3 RadiosityTechDemo

Solution and Project of VS2012  
C++ source code in Source\ and Source\Engine  
Shaders source code, configuration file of Profiler in Engine\Shaders  
    
### 3 How to improve quality of shadow maps

If you have a video card with a decent amount of RAM it is possible to improve the quality of the shadows (specially in the test scene 2) with the following steps:  
  
Increase the value of the shadowmapsize parameter in the .txt file of the scene you want to test  
Use this same value in the SMAP_SIZE constant in RadiosityTechDemo\Source\Engine\Shaders\shadowFunctions.fx  
Compile RadiosityTechDemo\Source\Engine\Shaders\commonMaterialShader.fx by running RadiosityTechDemo\Source\Engine\Shaders\compileNormalShader.bat and copy the output .fxo file to Bin\Assets\Shaders  
    
### 4 Create other test scenes

You can use AutoDesk 3ds Max modeling software and export the scene to the .OBJ format and .MTL. The format of the Faces must be set to Triangles and the Flip YZ-Axis checkbox must be checked if the "up" axis of your modeling software is the +z axis.  
The images of diffuse maps must be in the .jpg format.  
The normal maps must be created in the .bmp 24 bits format. To create a normal map from an image file you can use Nvidia's plugin for Photoshop [https://developer.nvidia.com/content/nvidia-plug-adobe-photoshop-64-bit](https://developer.nvidia.com/content/nvidia-plug-adobe-photoshop-64-bit)
    
### 5 Third parties licenses

This software makes use of the FW1FontWrapper software available in [http://fw1.codeplex.com/](http://fw1.codeplex.com/) This is its license:  
  
The MIT License (MIT)  
  
Copyright (c) 2011 Erik Rufelt  
  
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:  
  
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

### 6 License

You can find the RadiosityTechDemo software license in the LICENSE file in this repository.