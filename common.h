#pragma once

#include <string>
#include <time.h>

static int gs_time_use_flag = 0;
static std::string gs_time_date_first;
static std::string gs_time_date_second;

static std::string gs_time_hour_first;
static std::string gs_time_hour_second;

void update_time()
{
    std::string& date = gs_time_use_flag == 0 ? gs_time_date_second : gs_time_date_first;
    // 20221224
    struct tm stm;
    time_t t = time(NULL);
    localtime_r(&t, &stm);
    char buff1[32] = {0};      
    snprintf(buff1, sizeof(buff1), "%04d%02d%02d", stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday);
    date.assign(buff1);

    std::string& hour = gs_time_use_flag == 0 ? gs_time_hour_second : gs_time_hour_first;
    // 2022122417
    char buff2[32];
    snprintf(buff2, sizeof(buff2), "%04d%02d%02d%02d", stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday, stm.tm_hour);
    hour.assign(buff2);

    gs_time_use_flag = gs_time_use_flag == 0 ? 1 : 0;
}

inline std::string& get_date()
{
    return gs_time_use_flag == 0 ? gs_time_date_first : gs_time_date_second;
}

inline std::string& get_hour() 
{
    return gs_time_use_flag == 0 ? gs_time_hour_first : gs_time_hour_second;
}

