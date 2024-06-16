#include <spdlog/spdlog.h>

int main(int argc, char *argv[]) {
    spdlog::info("Welcome to spdlog!");
    spdlog::set_level(spdlog::level::debug);
    SPDLOG_INFO("test 中文测试");
    return 0;
}