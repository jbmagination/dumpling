#include "titlelist.h"
#include "titles.h"
#include "dumping.h"
#include "menu.h"
#include "navigation.h"
#include "gui.h"
#include "cfw.h"
#include "progress.h"

struct listEntry {
    bool queued;
    std::shared_ptr<titleEntry> listedEntry;
};

void showTitleList(const char* message, dumpingConfig config) {
    // Filter and create the list of titles that are displayed
    std::vector<listEntry> printTitles = {};
    for (auto& title : installedTitles) {
        // Check if the type is available
        if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::GAME) && !title->base) continue;
        else if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::UPDATE) && !title->update) continue;
        else if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::DLC) && !title->dlc) continue;
        else if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::SYSTEM_APP) && !title->base) continue;
        else if (HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::SAVES) && !title->saves) continue;

        // Differentiate between game and system app
        if (title->base && HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::GAME) && !isBase(title->base->type)) continue;
        if (title->base && HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::SYSTEM_APP) && !isSystemApp(title->base->type)) continue;

        // Prevent the disc copy from showing in the title lists
        if (title->base && HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::GAME) && title->base->location == TITLE_LOCATION::DISC) continue;

        // Prevent a game that doesn't have any contents in their common or any user saves
        if (title->saves && HAS_FLAG(config.filterTypes, DUMP_TYPE_FLAGS::SAVES) && (!title->saves->commonSave && title->saves->userSaves.empty())) continue;

        printTitles.emplace_back(listEntry{
            .queued = false,
            .listedEntry = title
        });
    }

    // Create a basic draw/input loop
    size_t selectedEntry = 0;
    size_t listOffset = 0;
    size_t listSize = WHBLogFreetypeScreenSize()-6;
    if (listSize > printTitles.size()) listSize = printTitles.size();

    bool startQueueDump = false;
    while(!startQueueDump) {
        // Print selection header
        WHBLogFreetypeStartScreen();
        WHBLogFreetypePrint(message);
        WHBLogFreetypePrint("===============================");

        // Print message when no title is found
        if (printTitles.empty()) {
            WHBLogFreetypePrint("Nothing was found!");
            WHBLogFreetypePrint("Make sure that everything is installed.");
        }

        // Print range of titles
        for (size_t i=listOffset; i<listOffset+listSize; i++) {
            WHBLogFreetypePrintf("%c %s %.30s", i == selectedEntry ? '>' : ' ', config.queue ? (printTitles[i].queued ? "\u25A0": "\u25A1") : "", printTitles[i].listedEntry->shortTitle.c_str());
        }
        WHBLogFreetypeScreenPrintBottom("===============================");
        WHBLogFreetypeScreenPrintBottom("\uE045 Button = Start Dumping");
        WHBLogFreetypeScreenPrintBottom("\uE000 Button = Select Title");
        WHBLogFreetypeScreenPrintBottom("\uE001 Button = Back to Main Menu");
        WHBLogFreetypeDrawScreen();

        // Loop until there's new input
        sleep_for(200ms);
        updateInputs();
        while(!startQueueDump) {
            updateInputs();

            if (navigatedUp()) {
                if (selectedEntry > 0) selectedEntry--;
                if (selectedEntry < listOffset) listOffset--;
                break;
            }
            if (navigatedDown()) {
                if (selectedEntry < printTitles.size()-1) selectedEntry++;
                if (listOffset+listSize == selectedEntry) listOffset++;
                break;
            }
            if (pressedOk()) {
                printTitles[selectedEntry].queued = !printTitles[selectedEntry].queued;
                if (!config.queue) startQueueDump = true;
                break;
            }
            if (pressedStart() && config.queue) {
                startQueueDump = true;
                break;
            }
            if (pressedBack()) {
                sleep_for(200ms);
                return;
            }

            sleep_for(20ms);
        }
    }

    // Create dumping queue
    std::vector<std::shared_ptr<titleEntry>> queuedTitles = {};
    for (auto& title : printTitles) {
        if (title.queued) queuedTitles.emplace_back(title.listedEntry);
    }

    if (queuedTitles.empty()) {
        showDialogPrompt("Select at least one title to dump!", "Go Back");
        showTitleList(message, config);
        return;
    }

    // Show the option screen and give a last chance to stop the update
    if (!showOptionMenu(config, HAS_FLAG(config.dumpTypes, DUMP_TYPE_FLAGS::SAVES))) return;
    auto startTime = std::chrono::system_clock::now();
    if (dumpQueue(queuedTitles, config)) {
        auto endTime = std::chrono::system_clock::now();
        auto elapsedDuration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
        std::string finishedMsg = "Dumping successfully finished in\n"+formatElapsedTime(elapsedDuration)+"!";
        showDialogPrompt(finishedMsg.c_str(), "Continue to Main Menu");
    }
}