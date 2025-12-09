#include "app_page.hpp"

#include <switch.h>

#include <filesystem>
#include <fstream>
#include <regex>

#include "confirm_page.hpp"
#include "current_cfw.hpp"
#include "download.hpp"
#include "download_cheats_page.hpp"
#include "extract.hpp"
#include "fs.hpp"
#include "utils.hpp"
#include "worker_page.hpp"

namespace i18n = brls::i18n;
using namespace i18n::literals;

AppPage::AppPage() : AppletFrame(true, true)
{
    list = new brls::List();
}

void AppPage::PopulatePage()
{
    this->CreateLabel();

    NsApplicationRecord* records = new NsApplicationRecord[MaxTitleCount];
    NsApplicationControlData* controlData = NULL;
    std::string name;

    s32 recordCount = 0;
    u64 controlSize = 0;
    u64 tid;

    if (!util::isApplet()) {
        if (R_SUCCEEDED(nsListApplicationRecord(records, MaxTitleCount, 0, &recordCount))) {
            for (s32 i = 0; i < recordCount; i++) {
                controlSize = 0;

                if (R_FAILED(InitControlData(&controlData))) break;

                tid = records[i].application_id;

                if (R_FAILED(GetControlData(tid, controlData, controlSize, name)))
                    continue;

                listItem = new brls::ListItem(name, "", util::formatApplicationId(tid));
                listItem->setThumbnail(controlData->icon, sizeof(controlData->icon));
                this->AddListItem(name, tid);
            }
            free(controlData);
        }
    }
    else {
        tid = GetCurrentApplicationId();
        if (R_SUCCEEDED(InitControlData(&controlData)) && R_SUCCEEDED(GetControlData(tid & 0xFFFFFFFFFFFFF000, controlData, controlSize, name))) {
            listItem = new brls::ListItem(name, "", util::formatApplicationId(tid));
            listItem->setThumbnail(controlData->icon, sizeof(controlData->icon));
            this->AddListItem(name, tid);
        }
        label = new brls::Label(brls::LabelStyle::SMALL, "menus/common/applet_mode_not_supported"_i18n, true);
        list->addView(label);
    }
    delete[] records;

    brls::Logger::debug("count {}", list->getViewsCount());

    if (!list->getViewsCount()) {
        list->addView(new brls::Label(brls::LabelStyle::DESCRIPTION, "menus/common/nothing_to_see"_i18n, true));
    }

    this->setContentView(list);
}

void AppPage::CreateDownloadAllButton()
{
    std::string text("menus/cheats/downloading"_i18n);
    std::string url = "";
    switch (CurrentCfw::running_cfw) {
        case CFW::ams:
            url += CHEATS_URL_CONTENTS;
            break;
        case CFW::rnx:
            url += CHEATS_URL_CONTENTS;
            break;
        case CFW::sxos:
            url += CHEATS_URL_CONTENTS;
            break;
    }
    text += url;
    download = new brls::ListItem("menus/cheats/dl_latest"_i18n);
    download->getClickEvent()->subscribe([url, text](brls::View* view) {
        brls::StagedAppletFrame* stagedFrame = new brls::StagedAppletFrame();
        stagedFrame->setTitle("menus/cheats/getting_cheats"_i18n);
        stagedFrame->addStage(
            new ConfirmPage(stagedFrame, text));
        stagedFrame->addStage(
            new WorkerPage(stagedFrame, "menus/common/downloading"_i18n, [url]() { util::downloadArchive(url, contentType::cheats); }));
        stagedFrame->addStage(
            new WorkerPage(stagedFrame, "menus/common/extracting"_i18n, []() { util::extractArchive(contentType::cheats); }));
        stagedFrame->addStage(
            new ConfirmPage_Done(stagedFrame, "menus/common/all_done"_i18n));
        brls::Application::pushView(stagedFrame);
    });
    list->addView(download);
}

u32 AppPage::InitControlData(NsApplicationControlData** controlData)
{
    free(*controlData);
    *controlData = (NsApplicationControlData*)malloc(sizeof(NsApplicationControlData));
    if (*controlData == NULL) {
        return 300;
    }
    else {
        memset(*controlData, 0, sizeof(NsApplicationControlData));
        return 0;
    }
}

u32 AppPage::GetControlData(u64 tid, NsApplicationControlData* controlData, u64& controlSize, std::string& name)
{
    Result rc;
    NacpLanguageEntry* langEntry = NULL;
    rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, tid, controlData, sizeof(NsApplicationControlData), &controlSize);
    if (R_FAILED(rc)) return rc;

    if (controlSize < sizeof(controlData->nacp)) return 100;

    rc = nacpGetLanguageEntry(&controlData->nacp, &langEntry);
    if (R_FAILED(rc)) return rc;

    if (!langEntry->name) return 200;

    name = langEntry->name;

    return 0;
}

void AppPage::AddListItem(const std::string& name, u64 tid)
{
    list->addView(listItem);
}

uint64_t AppPage::GetCurrentApplicationId()
{
    Result rc = 0;
    uint64_t pid = 0;
    uint64_t tid = 0;

    rc = pmdmntGetApplicationProcessId(&pid);
    if (rc == 0x20f || R_FAILED(rc)) return 0;

    rc = pminfoGetProgramId(&tid, pid);
    if (rc == 0x20f || R_FAILED(rc)) return 0;

    return tid;
}

AppPage_CheatSlips::AppPage_CheatSlips() : AppPage()
{
    this->PopulatePage();
    this->registerAction("menus/cheats/cheatslips_logout"_i18n, brls::Key::X, [this] {
        std::filesystem::remove(TOKEN_PATH);
        return true;
    });
}

void AppPage_CheatSlips::CreateLabel()
{
    this->setTitle("menus/cheats/cheatslips_title"_i18n);
    label = new brls::Label(brls::LabelStyle::DESCRIPTION, "menus/cheats/cheatslips_select"_i18n, true);
    list->addView(label);
}

void AppPage_CheatSlips::AddListItem(const std::string& name, u64 tid)
{
    listItem->getClickEvent()->subscribe([tid, name](brls::View* view) { brls::Application::pushView(new DownloadCheatsPage_CheatSlips(tid, name)); });
    list->addView(listItem);
}

AppPage_Gbatemp::AppPage_Gbatemp() : AppPage()
{
    this->PopulatePage();
    this->setIcon("romfs:/gbatemp_icon.png");
}

void AppPage_Gbatemp::CreateLabel()
{
    this->setTitle("menus/cheats/gbatemp_title"_i18n);
    label = new brls::Label(brls::LabelStyle::DESCRIPTION, "menus/cheats/cheatslips_select"_i18n, true);
    list->addView(label);
}

void AppPage_Gbatemp::AddListItem(const std::string& name, u64 tid)
{
    listItem->getClickEvent()->subscribe([tid, name](brls::View* view) { brls::Application::pushView(new DownloadCheatsPage_Gbatemp(tid, name)); });
    list->addView(listItem);
}

AppPage_Gfx::AppPage_Gfx() : AppPage()
{
    this->PopulatePage();
}

void AppPage_Gfx::CreateLabel()
{
    this->setTitle("menus/cheats/gfx_title"_i18n);
    label = new brls::Label(brls::LabelStyle::DESCRIPTION, "menus/cheats/cheatslips_select"_i18n, true);
    list->addView(label);
}

void AppPage_Gfx::AddListItem(const std::string& name, u64 tid)
{
    listItem->getClickEvent()->subscribe([tid, name](brls::View* view) { brls::Application::pushView(new DownloadCheatsPage_Gfx(tid, name)); });
    list->addView(listItem);
}


AppPage_Exclude::AppPage_Exclude() : AppPage()
{
    this->PopulatePage();
}

void AppPage_Exclude::CreateLabel()
{
    this->setTitle("menus/cheats/exclude_titles"_i18n);
    label = new brls::Label(brls::LabelStyle::DESCRIPTION, "menus/cheats/exclude_titles_desc"_i18n, true);
    list->addView(label);
}

void AppPage_Exclude::PopulatePage()
{
    this->CreateLabel();

    NsApplicationRecord* records = new NsApplicationRecord[MaxTitleCount];
    NsApplicationControlData* controlData = NULL;
    std::string name;

    s32 recordCount = 0;
    u64 controlSize = 0;
    u64 tid;

    auto titles = fs::readLineByLine(CHEATS_EXCLUDE);

    if (!util::isApplet()) {
        if (R_SUCCEEDED(nsListApplicationRecord(records, MaxTitleCount, 0, &recordCount))) {
            for (s32 i = 0; i < recordCount; i++) {
                controlSize = 0;

                if (R_FAILED(InitControlData(&controlData))) break;

                tid = records[i].application_id;

                if (R_FAILED(GetControlData(tid, controlData, controlSize, name)))
                    continue;

                brls::ToggleListItem* listItem;
                listItem = new brls::ToggleListItem(std::string(name), titles.find(util::formatApplicationId(tid)) != titles.end() ? 0 : 1);

                listItem->setThumbnail(controlData->icon, sizeof(controlData->icon));
                items.insert(std::make_pair(listItem, util::formatApplicationId(tid)));
                list->addView(listItem);
            }
            free(controlData);
        }
    }
    else {
        label = new brls::Label(brls::LabelStyle::SMALL, "menus/common/applet_mode_not_supported"_i18n, true);
        list->addView(label);
    }
    delete[] records;

    list->registerAction("menus/cheats/exclude_titles_save"_i18n, brls::Key::B, [this] {
        std::set<std::string> exclude;
        for (const auto& item : items) {
            if (!item.first->getToggleState()) {
                exclude.insert(item.second);
            }
        }
        extract::writeTitlesToFile(exclude, CHEATS_EXCLUDE);
        brls::Application::popView();
        return true;
    });

    this->registerAction("menus/cheats/exclude_all"_i18n, brls::Key::X, [this] {
        std::set<std::string> exclude;
        for (const auto& item : items) {
            exclude.insert(item.second);
        }
        extract::writeTitlesToFile(exclude, CHEATS_EXCLUDE);
        brls::Application::popView();
        return true;
    });

    this->registerAction("menus/cheats/exclude_none"_i18n, brls::Key::Y, [this] {
        extract::writeTitlesToFile({}, CHEATS_EXCLUDE);
        brls::Application::popView();
        return true;
    });

    this->setContentView(list);
}

AppPage_DownloadedCheats::AppPage_DownloadedCheats() : AppPage()
{
    GetExistingCheatsTids();
    this->PopulatePage();
}

void AppPage_DownloadedCheats::CreateLabel()
{
    this->setTitle("menus/cheats/installed"_i18n);
    label = new brls::Label(brls::LabelStyle::DESCRIPTION, "menus/cheats/label"_i18n, true);
    list->addView(label);
}

void AppPage_DownloadedCheats::AddListItem(const std::string& name, u64 tid)
{
    auto tid_str = util::formatApplicationId(tid);
    if (titles.find(tid_str) != titles.end()) {
        listItem->getClickEvent()->subscribe([tid, name](brls::View* view) { cheats_util::ShowCheatFiles(tid, name); });
        listItem->registerAction("menus/cheats/delete_cheats"_i18n, brls::Key::Y, [tid_str] {
            util::showDialogBoxInfo(extract::removeCheatsDirectory(fmt::format("{}{}", util::getContentsPath(), tid_str)) ? "menus/common/all_done"_i18n : fmt::format("menus/cheats/deletion_error"_i18n, tid_str));
            return true;
        });
        list->addView(listItem);
    }
}

void AppPage_DownloadedCheats::GetExistingCheatsTids()
{
    std::string path = util::getContentsPath();
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        std::string cheatsPath = entry.path().string() + "/cheats";
        if (std::filesystem::exists(cheatsPath) && !std::filesystem::is_empty(cheatsPath)) {
            for (const auto& cheatFile : std::filesystem::directory_iterator(cheatsPath)) {
                if (extract::isBID(cheatFile.path().filename().stem())) {
                    titles.insert(util::upperCase(cheatsPath.substr(cheatsPath.length() - 7 - 16, 16)));
                    break;
                }
            }
        }
    }
}

AppPage_OutdatedTitles::AppPage_OutdatedTitles() : AppPage()
{
    download::getRequest(LOOKUP_TABLE_URL, versions);
    if (versions.empty()) {
        list->addView(new brls::Label(brls::LabelStyle::DESCRIPTION, "menus/main/links_not_found"_i18n, true));
        this->setContentView(list);
    }
    else {
        this->PopulatePage();
    }
}

void AppPage_OutdatedTitles::AddListItem(const std::string& name, u64 tid)
{
    u32 version = cheats_util::GetVersion(tid);
    std::string tid_string = util::formatApplicationId(tid);
    if (versions.find(tid_string) != versions.end()) {
        bool has_update = false;
        bool has_missing_dlc = false;
        std::string info_text = tid_string;
        
        // Check for version update
        u32 latest = versions.at(tid_string).at("latest");
        std::string latest_build_id = "";
        if (version < latest) {
            has_update = true;
            // Get the build ID for the latest version
            std::string latest_ver_str = std::to_string(latest);
            if (versions.at(tid_string).contains(latest_ver_str)) {
                latest_build_id = versions.at(tid_string).at(latest_ver_str).get<std::string>();
            }
            info_text += fmt::format("\t|\t v{} (local) → v{} (latest)", version, latest);
        }
        
        // Check for missing DLCs
        int missing_dlc_count = 0;
        std::vector<std::pair<std::string, std::string>> missing_dlc_info;  // pair of (name, id)
        
        if (versions.at(tid_string).contains("dlc") && versions.at(tid_string).at("dlc").is_array()) {
            for (const auto& dlc : versions.at(tid_string).at("dlc")) {
                if (dlc.contains("id")) {
                    std::string dlc_id = dlc.at("id").get<std::string>();
                    uint64_t dlc_tid = std::stoull(dlc_id, nullptr, 16);
                    if (!cheats_util::IsDlcInstalled(dlc_tid)) {
                        missing_dlc_count++;
                        std::string dlc_name = "Unknown DLC";
                        if (dlc.contains("name")) {
                            dlc_name = dlc.at("name").get<std::string>();
                        }
                        missing_dlc_info.push_back({dlc_name, dlc_id});
                    }
                }
            }
        }
        
        if (missing_dlc_count > 0) {
            has_missing_dlc = true;
            if (has_update) {
                info_text += fmt::format(" + {} DLC", missing_dlc_count);
            } else {
                info_text += fmt::format("\t|\t {} ({})", "menus/tools/missing_dlc"_i18n, missing_dlc_count);
            }
        }
        
        // Only show if there's an update or missing DLC
        if (has_update || has_missing_dlc) {
            listItem->setSubLabel(info_text);
            
            // Add click handler to show details
            listItem->getClickEvent()->subscribe([name, tid_string, version, latest, has_update, latest_build_id, missing_dlc_info](brls::View* view) {
                brls::AppletFrame* appView = new brls::AppletFrame(true, true);
                brls::List* detailsList = new brls::List();
                
                // Add game name header
                brls::Label* titleLabel = new brls::Label(brls::LabelStyle::DESCRIPTION, name, true);
                detailsList->addView(titleLabel);
                
                // Add version info if outdated
                if (has_update) {
                    brls::ListItem* versionItem = new brls::ListItem("menus/tools/update_available"_i18n);
                    versionItem->setValue(fmt::format("v{} → v{} ({})", version, latest, latest_build_id));
                    detailsList->addView(versionItem);
                }
                
                // Add missing DLCs
                if (!missing_dlc_info.empty()) {
                    brls::Label* dlcHeader = new brls::Label(
                        brls::LabelStyle::DESCRIPTION, 
                        fmt::format("{} ({})", "menus/tools/missing_dlc"_i18n, missing_dlc_info.size()), 
                        true
                    );
                    detailsList->addView(dlcHeader);
                    
                    for (const auto& dlc : missing_dlc_info) {
                        brls::ListItem* dlcItem = new brls::ListItem(dlc.first);
                        dlcItem->setValue(dlc.second);
                        detailsList->addView(dlcItem);
                    }
                }
                
                appView->setContentView(detailsList);
                brls::PopupFrame::open("menus/tools/details"_i18n, appView, "", "");
            });
            
            list->addView(listItem);
        }
    }
}