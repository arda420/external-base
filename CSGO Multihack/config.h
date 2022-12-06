#include <iostream>
#include <string>
#include <fstream>
#include "options.h"

using namespace std;

void save()
{
    ofstream fw("c:\\csgomultihackconfig.txt", std::ofstream::out);

    if (fw.is_open())
    {
        fw << to_string(g_Options.aim[0]) << "\n";
        fw << to_string(g_Options.aim_smooth[0]) << "\n";
        fw << to_string(g_Options.aim_smooth_after_shot[0]) << "\n";
        fw << to_string(g_Options.aim_fov[0]) << "\n";
        fw << to_string(g_Options.recoil[0]) << "\n";
        fw << to_string(g_Options.recoil_smooth_x[0]) << "\n";
        fw << to_string(g_Options.recoil_smooth_y[0]) << "\n";
        fw << to_string(g_Options.bhop[0]) << "\n";
        fw << to_string(g_Options.trigger[0]) << "\n";
        fw << to_string(g_Options.trigger_delay[0]) << "\n";
        fw << to_string(g_Options.glow[0]) << "\n";
        fw << to_string(g_Options.ak_skin[0]) << "\n";
        fw << to_string(g_Options.m4_skin[0]) << "\n";
        fw << to_string(g_Options.awp_skin[0]) << "\n";
        fw << to_string(g_Options.radar[0]) << "\n";
        fw << to_string(g_Options.m4_silencer_skin[0]) << "\n";
        fw << to_string(g_Options.bhop_chance[0]) << "\n";
        fw << to_string(g_Options.glow_color[0].r()) << "\n";
        fw << to_string(g_Options.glow_color[0].g()) << "\n";
        fw << to_string(g_Options.glow_color[0].b()) << "\n";
        fw << to_string(g_Options.glow_color[0].a()) << "\n";
        fw.close();
    }
}

void load()
{
    ifstream file("c:\\csgomultihackconfig.txt");
    string str;

    int cfg[100] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int total_lines = 0;

    while (getline(file, str)) {
        cfg[total_lines] = std::stoi(str);
        total_lines++;
    }

    g_Options.aim[0] = cfg [0];
    g_Options.aim_smooth[0] = cfg [1];
    g_Options.aim_smooth_after_shot[0] = cfg [2];
    g_Options.aim_fov[0] = cfg [3];
    g_Options.recoil[0] = cfg [4];
    g_Options.recoil_smooth_x[0] = cfg [5];
    g_Options.recoil_smooth_y[0] = cfg [6];
    g_Options.bhop[0] = cfg [7];
    g_Options.trigger[0] = cfg [8];
    g_Options.trigger_delay[0] = cfg [9];
    g_Options.glow[0] = cfg [10];
    g_Options.ak_skin[0] = cfg [11];
    g_Options.m4_skin[0] = cfg [12];
    g_Options.awp_skin[0] = cfg [13];
    g_Options.radar[0] = cfg [14];
    g_Options.m4_silencer_skin[0] = cfg [15];
    g_Options.bhop_chance[0] = cfg [16];
    g_Options.glow_color[0] = Color(cfg[17], cfg[18], cfg[19], cfg[20]);
}