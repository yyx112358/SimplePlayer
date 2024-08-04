#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>
extern "C" {
#include <libavformat/avformat.h>
}


#include "GUI/main_win32.h"

int main(int argc, char *argv[]) 
{
    spdlog::info("Welcome to spdlog!");
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("%2H:%2M:%2S.%3e %!:%# [SimplePlayer] %v");
    SPDLOG_DEBUG(avformat_configuration());
    

    return main_win32(argc, argv);
}