#include <switch.h>

#include <borealis.hpp>
#include <filesystem>
#include <json.hpp>

#include "constants.hpp"
#include "current_cfw.hpp"
#include "fs.hpp"
#include "main_frame.hpp"
#include "warning_page.hpp"
#include "utils.hpp"
#include "download.hpp"
#include "worker_page.hpp"
#include "confirm_page.hpp"

namespace i18n = brls::i18n;
using namespace i18n::literals;

//TimeServiceType __nx_time_service_type = TimeServiceType_System;

CFW CurrentCfw::running_cfw;

int main(int argc, char* argv[])
{
    // Init the app
    if (!brls::Application::init(APP_TITLE)) {
        brls::Logger::error("Unable to init Borealis application");
        return EXIT_FAILURE;
    }

    nlohmann::ordered_json languageFile = fs::parseJsonFile(LANGUAGE_JSON);
    if (languageFile.find("language") != languageFile.end())
        i18n::loadTranslations(languageFile["language"]);
    else
        i18n::loadTranslations();

        //appletInitializeGamePlayRecording();

        // Setup verbose logging on PC
#ifndef __SWITCH__
    brls::Logger::setLogLevel(brls::LogLevel::DEBUG);
#endif

    setsysInitialize();
    plInitialize(PlServiceType_User);
    nsInitialize();
    socketInitializeDefault();
    nxlinkStdio();
    pmdmntInitialize();
    pminfoInitialize();
    splInitialize();
    romfsInit();

    CurrentCfw::running_cfw = CurrentCfw::getCFW();

    fs::createTree(CONFIG_PATH);

    brls::Logger::setLogLevel(brls::LogLevel::DEBUG);
    brls::Logger::debug("Start");

    // Check for app update and present a blocking flow before main UI
    {
        std::string latestTag = util::getLatestTag(TAGS_INFO);
        const char* currentVersion = APP_VERSION;
        bool hasUpdate = !latestTag.empty() && latestTag != currentVersion;

        if (hasUpdate) {
            // Blocking staged flow: automatically start update before main UI
            brls::StagedAppletFrame* stagedFrame = new brls::StagedAppletFrame();
            stagedFrame->setTitle("menus/common/updating"_i18n);

            // Download new version
            stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/downloading"_i18n, []() {
                util::downloadArchive(APP_URL, contentType::app);
            }));

            // Extract update
            stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/extracting"_i18n, []() {
                util::extractArchive(contentType::app);
            }));

            // Final stage: show completion + instruction to relaunch
            stagedFrame->addStage(new ConfirmPage_AppUpdate(stagedFrame, "menus/common/all_done"_i18n));

            brls::Application::pushView(stagedFrame);
        } else {
            // No update: proceed to normal UI
            if (std::filesystem::exists(HIDDEN_AIO_FILE)) {
                brls::Application::pushView(new MainFrame());
            } else {
                brls::Application::pushView(new WarningPage("menus/main/launch_warning"_i18n));
            }
        }
    }

    while (brls::Application::mainLoop())
        ;

    romfsExit();
    splExit();
    pminfoExit();
    pmdmntExit();
    socketExit();
    nsExit();
    setsysExit();
    plExit();
    return EXIT_SUCCESS;
}
