#include "../headers/editor.h"

namespace
{
    std::vector<std::string>* split(const std::string& s, char delim) {
        std::vector<std::string>* elems = new std::vector<std::string>;
        std::string item;
        std::string::size_type index = s.find(delim); // Finds the index of the first instance of the delimiter
        int oldIndex = 0;
        while (index != std::string::npos) { // Loop until no more instances of the delimiter exist
            elems->push_back(s.substr(oldIndex, index - oldIndex)); // Add the substring between this index and the previous to the vector
            oldIndex = index + 1;
            index = s.find(delim, oldIndex); // Find the index of the next instance of the delimiter
        }
        if ((unsigned)oldIndex < s.length() - 1) elems->push_back(s.substr(oldIndex, s.length() - oldIndex)); // Add any remaining substring to the vector
        return elems;
    }
}

Editor::Editor()
{
    // Allocate memory
    this->characterValues = new std::unordered_map<std::string, QString>;
    this->inventory = new int[Items::NUM_INVENTORY_SLOTS];
    this->itemSettings = new ItemSettings[Items::NUM_INVENTORY_SLOTS];
    this->combatChips = new int[Items::NUM_COMBAT_CHIP_SLOTS];

    // Determine the username
    QString username = qgetenv("USER");
    if (username.isEmpty())
        username = qgetenv("USERNAME");

    // The location of the save file is different on Windows and Mac
    this->_playerDataLocation = Strings::playerDataPrefix + username.toStdString() + Strings::playerDataSuffix;
    this->_tmpDataLocation = Strings::playerDataPrefix + username.toStdString() + Strings::tmpDataSuffix;

    // Open a stream and load contents into memory
    std::ifstream playerDataStream(this->_playerDataLocation);
    if (playerDataStream.fail()) std::abort(); // TODO: Implement more graceful exception handling.
    this->_playerData = new std::string((std::istreambuf_iterator<char>(playerDataStream)),
                                       std::istreambuf_iterator<char>());

    // The space is semi-important, treasure it always
    *this->_playerData = " " + *this->_playerData;

    std::string reservedItemIDs = loadValue("ReservedItemIDs");
    if (reservedItemIDs.compare("") == 0) this->_reservedItemIDs = nullptr;
    else
    {
        this->_reservedItemIDs = split(reservedItemIDs, ',');
        QStringList reservedItemNames;
        for (std::size_t i = 0; i < this->_reservedItemIDs->size(); i++)
        {
            std::string::size_type colonIndex = (*this->_reservedItemIDs)[i].find(':');
            std::string::size_type equalsIndex = (*this->_reservedItemIDs)[i].find('=');
            if (colonIndex != std::string::npos && equalsIndex != std::string::npos)
            {
                reservedItemNames.append(QString::fromStdString((*this->_reservedItemIDs)[i].substr(0, colonIndex - 1) + (*this->_reservedItemIDs)[i].substr(colonIndex, equalsIndex - colonIndex)));
            }
        }
        Items::itemList = Items::itemList + reservedItemNames;
    }
    std::string reservedChipIDs = loadValue("ReservedChipIDs");
    if (reservedChipIDs.compare("") == 0) this->_reservedChipIDs = nullptr;
    else
    {
        this->_reservedChipIDs = split(reservedChipIDs, ',');
        QStringList reservedChipNames;
        for (std::size_t i = 0; i < this->_reservedChipIDs->size(); i++)
        {
            std::string::size_type colonIndex = (*this->_reservedChipIDs)[i].find(':');
            std::string::size_type equalsIndex = (*this->_reservedChipIDs)[i].find('=');
            if (colonIndex != std::string::npos && equalsIndex != std::string::npos)
            {
                reservedChipNames.append(QString::fromStdString((*this->_reservedChipIDs)[i].substr(0, colonIndex - 1) + (*this->_reservedChipIDs)[i].substr(colonIndex, equalsIndex - colonIndex)));
            }
        }
        Items::combatChipList = Items::combatChipList + reservedChipNames;
    }

    playerDataStream.close();
}

Editor::~Editor()
{
    delete this->characterValues;
    delete[] this->inventory;
    delete[] this->combatChips;
    delete[] this->itemSettings;
    delete this->_playerData;
    if (this->_reservedItemIDs != nullptr) delete this->_reservedItemIDs;
    if (this->_reservedChipIDs != nullptr) delete this->_reservedChipIDs;
}

std::string Editor::loadValue(std::string specifier)
{
    /* Returns the value assigned to specifier. */
    // Find the value by determining the string between start and end (Ex:  0hp : <value> : System.Int32;
    std::string startDelimiter = " " + specifier + Strings::paddedSeperator;
    std::string endDelimiter = Strings::paddedSeperator;

    // Find the location of key in playerData
    std::size_t first = this->_playerData->find(startDelimiter);
    if (first == this->_playerData->npos) return ""; // Break if not found
    first += startDelimiter.length(); // Move to the position just before value
    std::size_t last = this->_playerData->find(endDelimiter, first);

    return this->_playerData->substr(first, last-first);
}

std::string Editor::loadReservedName(int id, bool isChip)
{
    std::vector<std::string>* reservedIDs = isChip ? this->_reservedChipIDs : this->_reservedItemIDs;
    if (reservedIDs == nullptr) return "";
    std::string idString = std::to_string(id);
    std::string reservedID;
    int idStringSize = idString.size();
    int reservedIDSize;
    for (std::vector<std::string>::iterator it = reservedIDs->begin(); it != reservedIDs->end(); it++)
    {
        reservedID = *it;
        reservedIDSize = reservedID.size();
        if (reservedIDSize > idStringSize && reservedID.substr(reservedIDSize - idStringSize, idStringSize).compare(idString) == 0)
        {
            std::string::size_type colonIndex = reservedID.find(':');
            if (colonIndex != std::string::npos)
            {
                return reservedID.substr(0, colonIndex - 1) + reservedID.substr(colonIndex, reservedIDSize - colonIndex - idStringSize - 1);
            }
        }
    }
    return "";
}

int Editor::loadReservedID(const std::string& name, bool isChip)
{
    std::vector<std::string>* reservedIDs = isChip ? this->_reservedChipIDs : this->_reservedItemIDs;
    if (reservedIDs == nullptr) return -1;
    std::string idString;
    std::string::size_type colonIndex = name.find(':');
    if (colonIndex != std::string::npos)
    {
        idString = name.substr(0, colonIndex) + '\\' + name.substr(colonIndex, name.size() - colonIndex);
    }
    else idString = name;
    std::string reservedID;
    int idStringSize = idString.size();
    int reservedIDSize;
    for (std::vector<std::string>::iterator it = reservedIDs->begin(); it != reservedIDs->end(); it++)
    {
        reservedID = *it;
        reservedIDSize = reservedID.size();
        std::string::size_type equalsIndex = reservedID.find('=');
        if (equalsIndex != std::string::npos && reservedIDSize > idStringSize && reservedID.substr(0, equalsIndex).compare(idString) == 0)
        {
            int id = 0;
            try
            {
                id = std::stoi(reservedID.substr(equalsIndex + 1, reservedIDSize - equalsIndex - 1));
            }
            catch (const std::invalid_argument& e) {}
            return id;
        }
    }
    return -1;
}

void Editor::replaceValue(std::string specifier, std::string oldValue, std::string newValue)
{
    /* Replaces oldValue with newValue in this->playerData. */
    std::string oldString = " " + specifier + Strings::paddedSeperator + oldValue; // String to be replaced
    std::string newString = " " + specifier + Strings::paddedSeperator + newValue; // String to be inserted

    // Find the location at which to replace
    std::size_t position = this->_playerData->find(oldString);

    // Erase oldString at position from playerData
    this->_playerData->erase(position, oldString.length());

    // Insert newString into playerData at position
    this->_playerData->insert(position, newString);
}

void Editor::save()
{
    /* This function is called when the user clicks on Save Character. First it outputs
     * its Editor's playerData member to a temporary file.  If this was successful, it overwrites
     * the original PlayerPrefs.txt with the temporary file. */

    // First, output the edited playerData into tmpDataLocation
    std::ofstream saveStream(this->_tmpDataLocation);
    saveStream << this->_playerData->substr(1, this->_playerData->size() - 1);  // Ignore the extra space at the beginning
    if (saveStream.fail()) std::abort();
    saveStream.close();

    // Next, overwrite the original playerData and replace it with the new playerData
    std::remove(this->_playerDataLocation.c_str());
    std::rename(this->_tmpDataLocation.c_str(), (const char*) this->_playerDataLocation.c_str());
}

QString* Editor::loadCharacterNames()
{
    /* This function searches playerData.txt for all character names and adds seperators to
     * the "Load Player" section in the menu bar of w. Type can either be System.String or
     * System.Int32. */
    std::string characterName;
    std::string specifier;
    QString* characterNames = new QString[this->MAX_CHARACTERS];

    /* In PlayerData.txt, character names are specified by their number, ranging from
     * 0 to MAX_CHARACTERS, followed by "name" (e.g. "0name : Smurfalicious"). */
    for (int i = 0; i < this->MAX_CHARACTERS; i++)
    {
        // Load the name and store it in characterNames
        specifier = std::to_string(i) + Strings::nameSpecifier;
        characterName = this->loadValue(specifier);
        characterNames[i] = QString::fromStdString(characterName);
    }

    return characterNames;
}

void Editor::loadCharacterValues()
{
    /* Loads the settings and stats of the character specified by ID. */
    std::string val;

    // Load and store the name and experience
    (*this->characterValues)[Strings::nameSpecifier] = QString::fromStdString(this->loadValue(this->currentID + Strings::nameSpecifier));
    (*this->characterValues)[Strings::characterExperienceSpecifier] = QString::fromStdString(this->loadValue(this->currentID + Strings::characterExperienceSpecifier));

    // Load comboBoxes
    for (int i = 0; i < Strings::CHARACTER_TAB_NUM_COMBOBOXES; i++)
    {
        val = this->loadValue(this->currentID + Strings::cComboBoxSpecifiers[i]);
        (*this->characterValues)[Strings::cComboBoxSpecifiers[i]] = QString::fromStdString(val);
    }

    // Load spinBoxes
    for (int i = 0; i < Strings::CHARACTER_TAB_NUM_SPINBOXES; i++)
    {
        if (Strings::cSpinBoxSpecifiers[i] == Strings::characterLevelSpecifier) continue;
        (*this->characterValues)[Strings::cSpinBoxSpecifiers[i]] = QString::fromStdString(this->loadValue(this->currentID + Strings::cSpinBoxSpecifiers[i]));
    }
}

void Editor::loadCharacterItemBrowser()
{
    /* Loads the combat chips and inventory of the character specified by ID*/
    ItemSettings setting;
    std::string specifierPrefix, specifierPostfix;

    // Load the inventory of the character specified by ID into this->inventory and this->itemSettings
    for (int i = 0; i < Items::NUM_INVENTORY_SLOTS; i++)
    {
        specifierPrefix = this->currentID + std::to_string(i);
        this->inventory[i] = std::stoi(this->loadValue(specifierPrefix + Strings::idSpecifier));
        setting.quantity = this->loadValue(specifierPrefix + Strings::itemQuantitySpecifier);
        setting.exp = this->loadValue(specifierPrefix + Strings::itemExperienceSpecifier);
        setting.rarity = this->loadValue(specifierPrefix + Strings::itemRaritySpecifier);
        setting.corrupted = this->loadValue(specifierPrefix + Strings::itemCorruptedSpecifier);
        for (int j = 0; j < Items::NUM_MOD_SLOTS; j++)
        {
            specifierPostfix = std::to_string(j);
            setting.mods[j] = this->loadValue(specifierPrefix + Strings::itemModSpecifier + specifierPostfix);
            setting.modQuantities[j] = this->loadValue(specifierPrefix + Strings::itemModQuantitySpecifier + specifierPostfix);
        }
        this->itemSettings[i] = setting;
    }

    // Load the combat chips of the character specified by ID into this->combatChips
    for (int i = 0; i < Items::NUM_COMBAT_CHIP_SLOTS; i++)
        this->combatChips[i] = std::stoi(this->loadValue(this->currentID + Strings::combatChipSpecifier + std::to_string(i)));
}

int Editor::calculateItemLevelFromExperience(int exp)
{
    // Takes exp and returns the associated item level
    if      (exp >= Items::itemLevel10exp) return 10;
    else if (exp >= Items::itemLevel9exp) return 9;
    else if (exp >= Items::itemLevel8exp) return 8;
    else if (exp >= Items::itemLevel7exp) return 7;
    else if (exp >= Items::itemLevel6exp) return 6;
    else if (exp >= Items::itemLevel5exp) return 5;
    else if (exp >= Items::itemLevel4exp) return 4;
    else if (exp >= Items::itemLevel3exp) return 3;
    else if (exp >= Items::itemLevel2exp) return 2;

    // Default to returning one
    return 1;
}

int Editor::calculateItemExperienceFromLevel(int level)
{
    // Use Roguelands formula to convert newLevel to exp
    switch (level)
    {
    case 2:  return Items::itemLevel2exp;
    case 3:  return Items::itemLevel3exp;
    case 4:  return Items::itemLevel4exp;
    case 5:  return Items::itemLevel5exp;
    case 6:  return Items::itemLevel6exp;
    case 7:  return Items::itemLevel7exp;
    case 8:  return Items::itemLevel8exp;
    case 9:  return Items::itemLevel9exp;
    case 10: return Items::itemLevel10exp;
    default: return Items::itemLevel1exp;
    }
}
