#
#  Be sure to run `pod spec lint SimplePlayer.podspec' to ensure this is a
#  valid spec and to remove all comments including this before submitting the spec.
#
#  To learn more about Podspec attributes see https://guides.cocoapods.org/syntax/podspec.html
#  To see working Podspecs in the CocoaPods repo see https://github.com/CocoaPods/Specs/
#

PROJECT_ROOT_PATH = 'proj/mac'
THIRDPARTY_ROOT_PATH = 'thirdParty'
BUILD_ROOT_PATH = 'build'

Pod::Spec.new do |spec|

  spec.name         = "SimplePlayer"
  spec.version      = "0.0.1"
  spec.summary      = "学习用的简易跨平台播放器"

  spec.homepage     = "https://github.com/yyx112358/SimplePlayer"
  spec.license      = "MIT"
  spec.author       = { "yyx112358" => "yyx112358@163.com" }

  spec.ios.deployment_target = "12.0"
  spec.osx.deployment_target = "13.0"

  spec.source       = { :git => "https://github.com/yyx112358/SimplePlayer", :branch => "mac" }

  spec.pod_target_xcconfig = {
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17'
  }

  ################################# Base ############################################
  spec.subspec 'Base' do |s|
    s.dependency 'SimplePlayer/FFMpeg'
    s.source_files = "src/Base/**/*.{hpp,h,cpp,c,m,mm}"
  end
  
  ################################# Ability ############################################
  spec.subspec 'Ability' do |s|
    s.dependency 'SimplePlayer/Base'
    s.source_files = "src/Ability/**/*.{hpp,h,cpp,c,m,mm}"
  end
  
  ################################# Engine ############################################
  spec.subspec 'Engine' do |s|
    s.dependency 'SimplePlayer/Ability'
    s.source_files = "src/Engine/**/*.{hpp,h,cpp,c,m,mm}"
  end
  
  ################################# Interface ############################################
  spec.subspec 'Interface' do |s|
    s.dependency 'SimplePlayer/Engine'
    s.source_files = "inc/**/*.{h,hpp}"
  end


  ################################# Third Party ############################################

  spec.subspec 'spdlog' do |s|
    PATH_SPDLOG_ROOT = 'thirdParty/spdlog'

    # script_phase 是用在编译阶段执行的，可以执行任意脚本。但不是用在pod install时候的。
    # s.script_phase = {:name => 'make spdlog', :script => "cmake -S #{PATH_SPDLOG_ROOT} -B #{BUILD_ROOT_PATH}/spdlog 
    #                                                      && cmake --build #{BUILD_ROOT_PATH}/spdlog"}

    s.source_files = "#{PATH_SPDLOG_ROOT}/include/**/*.{hpp,h,cpp}"
    s.public_header_files = "#{PATH_SPDLOG_ROOT}/include/**/*.h"

    s.vendored_libraries = "build/thirdParty/spdlog/libspdlog.a" # 指定静态库，关键代码
    s.header_mappings_dir = "#{PATH_SPDLOG_ROOT}/include"    # 表示include文件的目录结构会保留，不指定的话所有header都会复制到同一个目录下
    
    s.pod_target_xcconfig = { 
      "USE_HEADERMAP": "NO",
      'GCC_PREPROCESSOR_DEFINITIONS' => 'SPDLOG_COMPILED_LIB',
      # "HEADER_SEARCH_PATHS": '"${PODS_TARGET_SRCROOT}/include"'
    }
  end

  spec.subspec 'FFMpeg' do |s|
    PATH_FFMPEG_ROOT = 'build/thirdParty/FFMpeg'

    s.source_files = "#{PATH_FFMPEG_ROOT}/include/**/*.h"
    s.header_mappings_dir = "#{PATH_FFMPEG_ROOT}/include" 

    s.vendored_libraries = "#{PATH_FFMPEG_ROOT}/lib/*.a"
    s.frameworks = 'AVFoundation'

    # s.pod_target_xcconfig = { 
    #   "USE_HEADERMAP": "NO",
    #   # 'GCC_PREPROCESSOR_DEFINITIONS' => 'SPDLOG_COMPILED_LIB',
    #   "HEADER_SEARCH_PATHS": '"${PODS_TARGET_SRCROOT}/build/thirdParty/FFMpeg/include"'
    # }
  end

  spec.subspec 'glm' do |s|
    PATH_GLM_ROOT = 'thirdParty/glm'

    s.source_files = "#{PATH_GLM_ROOT}/**/*.{hpp,h,inl}"
    s.header_mappings_dir = "#{PATH_GLM_ROOT}"

  end

end
