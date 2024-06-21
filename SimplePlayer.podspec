#
#  Be sure to run `pod spec lint SimplePlayer.podspec' to ensure this is a
#  valid spec and to remove all comments including this before submitting the spec.
#
#  To learn more about Podspec attributes see https://guides.cocoapods.org/syntax/podspec.html
#  To see working Podspecs in the CocoaPods repo see https://github.com/CocoaPods/Specs/
#

PROJECT_ROOT_PATH = 'proj/mac/'
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

  # spec.subspec 'Base' do |s|

    
  # end

  spec.subspec 'spdlog' do |s|
    PATH_SPDLOG_ROOT = 'thirdParty/spdlog'

    # s.script_phase = {:name => 'make spdlog', :script => "cmake -S #{PATH_SPDLOG_ROOT} -B #{BUILD_ROOT_PATH}/spdlog 
    #                                                      && cmake --build #{BUILD_ROOT_PATH}/spdlog"}

    s.source_files = "#{PATH_SPDLOG_ROOT}/include/**/*.{hpp,h,cpp}"
    s.public_header_files = "#{PATH_SPDLOG_ROOT}/include/**/*.h"

    s.vendored_libraries = "#{BUILD_ROOT_PATH}/thirdParty/spdlog/libspdlog.a" # 指定静态库，关键代码
    s.header_mappings_dir = "#{PATH_SPDLOG_ROOT}/include"    # 表示include文件的目录结构会保留，不指定的话所有header都会复制到同一个目录下
    
    s.pod_target_xcconfig = { 
      "USE_HEADERMAP": "NO",
      'GCC_PREPROCESSOR_DEFINITIONS' => 'SPDLOG_COMPILED_LIB',
      "HEADER_SEARCH_PATHS": '"${PODS_TARGET_SRCROOT}/include"'
    }
  end

  # spec.subspec 'FFMpeg' do |s|

  # end

end
