#ifndef EDITOR_H
#define EDITOR_H

#pragma once
#include <QMenu>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include "items.h"
#include "strings.h"

// Define an item as an object with a various properties
struct ItemSettings
{
    std::string quantity;
    std::string exp;
    std::string rarity;
    std::string corrupted;
    std::string mods[Items::NUM_MOD_SLOTS];
    std::string modQuantities[Items::NUM_MOD_SLOTS];
};

class Editor
{
public:
    Editor();
    ~Editor();

    std::string loadValue(std::string specifier); // Loads a value from file
    std::string loadReservedName(int id, bool isChip = false); // Loads a GadgetCore reserved registry name
    int loadReservedID(const std::string& name, bool isChip = false); // Loads a GadgetCore reserved ID
    void replaceValue(std::string specifier,                 // Replaces a value in file
                              std::string oldValue,
                              std::string newValue);
    void save();                                   // Save player data to file
    QString* loadCharacterNames();                 // Load in characters from file
    void loadCharacterValues();                    // Load character settings and stats
    void loadCharacterItemBrowser();               // Load inventory of character specified by ID
    void loadPlayerData();                         // Loads data from file
    int calculateItemLevelFromExperience(int exp);
    int calculateItemExperienceFromLevel(int level);


    static const int MAX_CHARACTERS = 6;       // Maximum number of characters allowed by Roguelands

    std::string currentID;                                      // ID of the current character

    std::unordered_map<std::string, QString>* characterValues;  // Maps a specifier string to the character's associated value in QString form
    int* inventory;                                             // Maps an index to an item
    int* combatChips;                                           // Maps an index to a combat chip ID
    ItemSettings* itemSettings;                                 // Maps an index to ItemSettings
private:
    std::string _playerDataLocation;    // Location of file
    std::string _tmpDataLocation;       // Location to store temporary data when saving
    std::string* _playerData;           // Pointer to contents of file in memory
    std::vector<std::string>* _reservedItemIDs;
    std::vector<std::string>* _reservedChipIDs;
};

#endif
