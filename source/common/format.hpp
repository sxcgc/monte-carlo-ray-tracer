#pragma once

#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace Format
{
    inline std::string date(const std::chrono::time_point<std::chrono::system_clock>& date)
    {
        std::time_t now = std::chrono::system_clock::to_time_t(date);
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);

        std::string s(26, ' ');
        std::strftime(s.data(), s.size(), "%Y-%m-%d %H:%M", &timeinfo);

        auto p = s.find_last_not_of(' ');
        s.erase(p, s.size() - p);

        return s;
    }

    inline std::string timeDuration(size_t msec_duration)
    {
        size_t hours = msec_duration / 3600000;
        size_t minutes = (msec_duration % 3600000) / 60000;
        size_t seconds = (msec_duration % 60000) / 1000;

        std::stringstream ss;
        ss << std::setfill('0') << std::setw(2) << hours << ":"
            << std::setfill('0') << std::setw(2) << minutes << ":"
            << std::setfill('0') << std::setw(2) << seconds;

        return ss.str();
    }

    inline std::string progress(double progress)
    {
        std::string d_str = std::to_string(progress);
        size_t dot_pos = d_str.find('.');
        std::string l(""), r("");
        if (dot_pos != std::string::npos)
        {
            l = d_str.substr(0, dot_pos);
            r = d_str.substr(dot_pos + 1);
        }
        else
        {
            l = d_str;
        }
        size_t r_len = (4 - l.length());
        std::string r_n = r.length() >= r_len ? r.substr(0, r_len) : r + std::string(' ', static_cast<int>(r_len - r.length()));

        return l + "." + r_n + "%";
    }

    inline std::string largeNumber(size_t n)
    {
        std::string int_string = std::to_string(n);
        size_t pos = int_string.length() - 3;
        while (pos > 0 && pos < int_string.length())
        {
            int_string.insert(pos, " ");
            pos -= 3;
        }
        return int_string;
    }
}