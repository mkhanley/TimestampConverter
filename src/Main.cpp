// This file is part of TimestampConverter.
// 
// Copyright (C)2021 mkhanley
// 
// TimestampConverter is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "AboutDialog.h"
#include "resource.h"
#include "PluginInterface.h"
#include "ScintillaEditor.h"
#include "menuCmdID.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

// The handle to the current DLL Module
static HANDLE dllModule;
// Data provided by Notepad++
static NppData nppData;
// The main (left) editor
static ScintillaEditor editor1;
// The secondary (right) editor
static ScintillaEditor editor2;
// References the current editor
static ScintillaEditor editor;

// Forward declaration of menu callbacks
static void executeTimestampConverter();
static void showAbout();


// The menu entries for the plugin
static FuncItem menuItems[] = {
	// name, function, 0, is_checked, shortcut
	{ L"Execute", executeTimestampConverter, 0, false, nullptr },
	{ L"", nullptr, 0, false, nullptr }, // Separator
	{ L"About...", showAbout, 0, false, nullptr }
};

static void executeTimestampConverter() {
	TCHAR buff[1024];
	size_t currentWordLen = 1024;
	bool suc = SendMessage(nppData._nppHandle, NPPM_GETCURRENTWORD, currentWordLen, (LPARAM)&buff);

	uint64_t num = _ttoll(buff);
	std::cout << num << "\n";


	auto epoch = std::chrono::time_point<std::chrono::system_clock>();
	std::chrono::microseconds since_epoch;
	int len = wcslen(buff);
	if (len <= 10) {
		// assume seconds
		since_epoch = std::chrono::seconds(num);
	}
	else if (len == 13) {
		//assume milliseconds
		since_epoch = std::chrono::milliseconds(num);
	}
	else if (len == 16) {
		// assume microseconds
		since_epoch = std::chrono::microseconds(num);
	}
	else if (len == 19) {
		// assume nanoseconds
		//since_epoch = std::chrono::nanoseconds(num);
	}

	auto timestamp = epoch + since_epoch;
	uint64_t nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch).count() % 1000000000;

	
	std::stringstream ss;
	std::string format = "%Y-%m-%d %H:%M:%S.";

	std::time_t timestamp_c = std::chrono::system_clock::to_time_t(timestamp);
	std::tm timstamp_tm = *std::localtime(&timestamp_c);

	ss << "\n" << std::put_time(&timstamp_tm, format.c_str()) << std::setfill('0') << std::setw(9) << nanos;
	editor.AddText(ss.str());
}

static void showAbout() {
	ShowAboutDialog((HINSTANCE)dllModule, MAKEINTRESOURCE(IDD_ABOUTDLG), nppData._nppHandle);
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode) {
	// If another buffer was activated update the editor reference to the correct one.
	if (notifyCode->nmhdr.code == NPPN_BUFFERACTIVATED) {
		int which = -1;
		SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, SCI_UNUSED, (LPARAM)&which);

		editor = (which == 0) ? editor1 : editor2;
	}

	// Handle notifications here
	switch (notifyCode->nmhdr.code) {
		case SCN_CHARADDED:
			break;
	}

	return;
}

extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData) {
	nppData = notepadPlusData;

	// Set these as early as possible so they are in a valid state
	editor1.SetScintillaInstance(nppData._scintillaMainHandle);
	editor2.SetScintillaInstance(nppData._scintillaSecondHandle);

	editor = editor1;
}

extern "C" __declspec(dllexport) const wchar_t *getName() {
	return L"TimestampConverter";
}

extern "C" __declspec(dllexport) FuncItem *getFuncsArray(int *nbF) {
	*nbF = sizeof(menuItems) / sizeof(menuItems[0]);
	return menuItems;
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam) {
	return TRUE;
}

extern "C" __declspec(dllexport) BOOL isUnicode() {
	return TRUE;
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  reasonForCall, LPVOID lpReserved) {
	switch (reasonForCall) {
		case DLL_PROCESS_ATTACH:
			dllModule = hModule;
			break;
		case DLL_PROCESS_DETACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}
