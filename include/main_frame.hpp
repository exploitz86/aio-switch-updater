#pragma once

#include <borealis.hpp>

class ToolsTab;

class MainFrame : public brls::TabFrame
{
private:
    //RefreshTask *refreshTask;
    ToolsTab* toolsTab = nullptr;

public:
    MainFrame();
};
