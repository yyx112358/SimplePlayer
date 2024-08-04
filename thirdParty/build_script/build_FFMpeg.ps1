echo "Build FFMpeg..."

$THIRDPARTY_ROOT_PATH = Convert-Path ..
$BUILD_PATH = Join-Path $THIRDPARTY_ROOT_PATH "build"

echo $BUILD_PATH

# 检查是不是纯净安装
if ((Test-Path $BUILD_PATH) -and (-not $args.Contains("--clean-install")))
{
    $CLEAN_INSTALL = $false
    echo "not clean-install, use cached ${BUILD_PATH}"
}
else
{
    echo "clean install..."
    Remove-Item "${BUILD_PATH}/vcpkg" -Recurse -Force
    mkdir $BUILD_PATH
}

# 安装vcpkg
$VCPKG_DIR_PATH = Join-Path $BUILD_PATH "vcpkg"
if (!(Test-Path $VCPKG_DIR_PATH))
{
    echo "vcpkg not existed in ${VCPKG_DIR_PATH}"
    cd $BUILD_PATH
    try
    {
        git clone git@github.com:microsoft/vcpkg.git
        cd $VCPKG_DIR_PATH
    }
    catch
    {
        Write-Host "Error: Failed to clone vcpkg repository."
        exit 1
    }
}
else
{
    echo "vcpkg exised"
}

# 使用vcpkg安装FFMpeg
$VCPKG_PATH = Join-Path $VCPKG_DIR_PATH "vcpkg.exe"
if (!(Test-Path $VCPKG_PATH))
{
    echo "vcpkg.exe not existed in ${VCPKG_PATH}"
    try
    {
        cd $VCPKG_DIR_PATH
        .\bootstrap-vcpkg.bat
    }
    catch
    {
        Write-Host "Failed to initilze vcpkg"
        exit 1
    }
}

& $VCPKG_PATH install ffmpeg:x64-windows-static
