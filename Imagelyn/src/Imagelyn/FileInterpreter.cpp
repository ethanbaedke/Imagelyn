#include "imagelynpch.h"

#include "FileInterpreter.h"
#include "Client.h"
#include "ConsoleManager.h"

#include <fstream>

namespace Imagelyn {

	// Declare static variables
	std::shared_ptr<Location> FileInterpreter::m_CurrentLocation;
	std::shared_ptr<Activity> FileInterpreter::m_CurrentActivity;

	bool FileInterpreter::Load(std::string path)
	{
		m_CurrentLocation.reset();
		m_CurrentActivity.reset();

		std::ifstream file(path);

		// Make sure the file was found and opened correctly
		if (!file.is_open())
		{
			ConsoleManager::Error(std::string("Could not open file: ") + path);
			return false;
		}

		// A reference to this string is passed to all helper functions and it will be populated with an error message if anything goes wrong
		// If the first characte of the error message is '~', consider it a warning and continue the flow of execution
		std::string errorMessage;

		// The default output when an error occurs (error message will be appended to the end)
		std::string errorOutput = "Error reading file (" + path + "): ";

		while (!file.eof())
		{
			std::string keyword = ParseKeyword(file, errorMessage);

			// If we hit the end of the file looking for a keyword, exit the loop
			if (errorMessage == "EOF")
			{
				break;
			}
			// If there was a warning parsing for a keyword, print it, clear the variable, and continue execution
			else if (errorMessage[0] == '~')
			{
				std::string warning = errorMessage.substr(1, errorMessage.size());
				ConsoleManager::Warning(errorOutput + warning);
				errorMessage = "";
			}
			// If there was an error parsing for a keyword, print it and return
			else if (errorMessage != "")
			{
				ConsoleManager::Error(errorOutput + errorMessage);
				return false;
			}

			HandleKeyword(keyword, file, errorMessage);

			// If there was a warning parsing for a keyword, print it, clear the variable, and continue execution
			if (errorMessage[0] == '~')
			{
				std::string warning = errorMessage.substr(1, errorMessage.size());
				ConsoleManager::Warning(errorOutput + warning);
				errorMessage = "";
			}
			// If there was an error handling our keyword, print it and return
			else if (errorMessage != "")
			{
				ConsoleManager::Error(errorOutput + errorMessage);
				return false;
			}
		}

		// File successfullly interpreted
		ConsoleManager::Message("File " + path + " loaded successfully");
		if (m_CurrentLocation)
			LocationManager::AddLocation(m_CurrentLocation);
		return true;
	}

	bool FileInterpreter::IsAlphanumeric(char character)
	{
		return (character > 0x40 && character < 0x5B) || (character > 0x60 && character < 0x7B);
	}

	const std::string FileInterpreter::ParseKeyword(std::ifstream& file, std::string& errorMessage)
	{
		char byte = 0x00;

		// Go through the file until we hit an alphanumeric character or the end of the file
		while (!file.eof() && !IsAlphanumeric(byte))
			file.read(&byte, 1);

		// If we are at the end of the file, send an EOF error message and return
		if (file.eof())
		{
			errorMessage = "EOF";
			return "";
		}

		// If we found an alphanumeric character, read the rest of the word and return it
		// Keywords should always be followed by a space
		std::string keyword = "";
		while (!file.eof() && byte != 0x0D/*Carriage Return*/ && byte != 0x0A/*Line Feed*/ && byte != 0x20/*Space*/)
		{
			keyword += byte;
			file.read(&byte, 1);
		}

		// If we hit the end of the file, found a carraige return, or found a line feed, the keyword was incomplete
		if (file.eof() || byte == 0x0D/*Carraige Return*/ || byte == 0x0A/*Carriage Return*/)
		{
			errorMessage = "Keyword (" + keyword + ") is incomplete";
			return "";
		}

		// If we found a space, return the keyword
		return keyword;
	}

	const std::string FileInterpreter::ParseString(std::ifstream& file, std::string& errorMessage)
	{
		char byte = 0x00;

		// Go through the file until we hit a quotation mark or the end of the file
		while (!file.eof() && !IsAlphanumeric(byte) && byte != '"')
			file.read(&byte, 1);

		// If we are at the end of the file, send an EOF error message and return
		if (file.eof())
		{
			errorMessage = "EOF";
			return "";
		}

		// If we hit an alphanumeric character before a quotation mark, set an error message and return
		if (IsAlphanumeric(byte))
		{
			errorMessage = "Expected quotation mark at beginning of string";
			return "";
		}

		// If we found a quotation mark, read the rest of the word and return it
		// Strings should always be on one line and end with a quotation mark
		std::string string = "";
		file.read(&byte, 1);
		while (!file.eof() && byte != 0x0D/*Carriage Return*/ && byte != 0x0A/*Line Feed*/ && byte != 0x22/*Double Quotes*/)
		{
			string += byte;
			file.read(&byte, 1);
		}

		// If we hit the end of the file, found a carraige return, or found a line feed, the string was incomplete
		if (file.eof() || byte == 0x0D/*Carraige Return*/ || byte == 0x0A/*Carriage Return*/)
		{
			errorMessage = "Incomplete string";
			return "";
		}

		// If we found the second quotation mark, return the string in between the two
		return string;
	}

	void FileInterpreter::HandleKeyword(const std::string& keyword, std::ifstream& file, std::string& errorMessage)
	{
		if (keyword == "location")
		{
			// If there is already a location defined in this file, set an error message and return
			if (m_CurrentLocation != nullptr)
			{
				errorMessage = "Only one location should be defined per file";
				return;
			}

			std::string locationName = ParseString(file, errorMessage);

			// The actual error message will be handeled outside of this function, we just make sure there isn't one before we create the location object
			if (errorMessage == "")
			{
				m_CurrentLocation = std::make_shared<Location>(locationName);
			}
		}
		else if (keyword == "activity")
		{
			// If there is no active location for this activity to be added to, set an error message and return
			if (m_CurrentLocation == nullptr)
			{
				errorMessage = "Location must be defined before activity";
				return;
			}

			std::string activityName = ParseString(file, errorMessage);

			// The actual error message will be handeled outside of this function, we just make sure there isn't one before we create the activity object
			if (errorMessage == "")
			{
				m_CurrentActivity = std::make_shared<Activity>(activityName);
				m_CurrentLocation->AddActivity(m_CurrentActivity);
			}
		}
		else if (keyword == "preference+")
		{
			// If there is no active activity for this preference to be added to, set an error message and return
			if (m_CurrentActivity == nullptr)
			{
				errorMessage = "Activity must be defined before a preference";
				return;
			}

			std::string preference = ParseString(file, errorMessage);

			// Make preference all lower case
			for (int i = 0; i < preference.size(); i++)
			{
				preference[i] = std::tolower(preference[i]);
			}

			// The actual error message will be handeled outside of this function, we just make sure there isn't one before we add the preference to our current activity
			if (errorMessage == "")
			{
				if (std::shared_ptr<Preference> p = PreferenceManager::GetPreferenceByName(preference))
					m_CurrentActivity->AddPositivePreference(p);
				else
					errorMessage = "~Preference (" + preference + ") is undefined";
			}
		}
		else if (keyword == "preference-")
		{
			// If there is no active activity for this preference to be added to, set an error message and return
			if (m_CurrentActivity == nullptr)
			{
				errorMessage = "Activity must be defined before a preference";
				return;
			}

			std::string preference = ParseString(file, errorMessage);

			// Make preference all lower case
			for (int i = 0; i < preference.size(); i++)
			{
				preference[i] = std::tolower(preference[i]);
			}

			// The actual error message will be handeled outside of this function, we just make sure there isn't one before we add the preference to our current activity
			if (errorMessage == "")
			{
				if (std::shared_ptr<Preference> p = PreferenceManager::GetPreferenceByName(preference))
					m_CurrentActivity->AddNegativePreference(p);
				else
					errorMessage = "~Preference (" + preference + ") is undefined";
			}
		}
		else if (keyword == "add_preference")
		{
			std::string preference = ParseString(file, errorMessage);

			// Make preference all lower case
			for (int i = 0; i < preference.size(); i++)
			{
				preference[i] = std::tolower(preference[i]);
			}

			// The actual error message will be handeled outside of this function, we just make sure there isn't one before we create the preference object
			if (errorMessage == "")
			{
				std::shared_ptr<Preference> p = std::make_shared<Preference>(preference);

				// Try and add the new preference to the list, if it already exists set an error message and return
				if (!PreferenceManager::AddPreference(p))
					errorMessage = "Preference (" + preference + ") defined more than once";
			}
		}
		else if (keyword == "name")
		{
			std::string name = ParseString(file, errorMessage);

			// The actual error message will be handeled outside of this function, we just make sure there isn't one before we add the name object
			if (errorMessage == "")
				ClientManager::AddName(name);
		}
		else
		{
			// The keyword is not recognized
			errorMessage = "Keyword (" + keyword + ") not recognized";
		}
	}


}