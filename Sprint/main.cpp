#include <iostream>
#include <windows.h>
#include <fstream>
#include <string>
#include <thread>

int forward = 0, sprint = 0;
bool is_sprinting = false;

HHOOK keybd_hook = { NULL };
LRESULT CALLBACK keybd_proc(int ncode, WPARAM w_param, LPARAM l_param)
{
	if (ncode == HC_ACTION)
	{
		PKBDLLHOOKSTRUCT ptr = (PKBDLLHOOKSTRUCT)l_param;
		switch (w_param)
		{
		case WM_KEYDOWN:
			if (ptr->vkCode == forward && !is_sprinting)
				is_sprinting = true;
			break;

		case WM_KEYUP:
			if (ptr->vkCode == forward && is_sprinting)
				is_sprinting = false;
			break;
		}
	}
	return CallNextHookEx(NULL, ncode, w_param, l_param);
}

void keybd_io()
{
	keybd_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keybd_proc, 0, 0);
	MSG lp_msg;

	while (!GetMessage(&lp_msg, NULL, 0, 0))
	{
		TranslateMessage(&lp_msg);
		DispatchMessage(&lp_msg);
	}
	UnhookWindowsHookEx(keybd_hook);
}

void extract_keys(int& forward, int& sprint)
{
	char* buf = nullptr;
	size_t sz = 0, pos = 0;
	std::string path;

	if (_dupenv_s(&buf, &sz, "LOCALAPPDATA") == 0 && buf != nullptr)
	{
		path = buf;
		free(buf);
	}

	path += "\\Packages\\Microsoft.MinecraftUWP_8wekyb3d8bbwe";
	path += "\\LocalState\\games\\com.mojang\\minecraftpe";

	std::string del = ":", key;
	std::ifstream options(path + "\\options.txt");
	
	if (options.is_open()) {
		while (getline(options, key))
		{
			if (key.find("keyboard_type_0_key.forward") != std::string::npos)
			{
				pos = key.find(del) + 1;
				forward = stoi(key.substr(pos));
			}

			if (key.find("keyboard_type_0_key.sprint") != std::string::npos) 
			{
				pos = key.find(del) + 1;
				sprint = stoi(key.substr(pos));
			}
		}
		options.close();
	}
	else exit(0);
}

void set_sprint(INPUT ip[2], int delay)
{
	SendInput(1, &ip[0], sizeof(INPUT));
	std::this_thread::sleep_for(std::chrono::milliseconds(delay / 2));
	SendInput(1, &ip[1], sizeof(INPUT));
	std::this_thread::sleep_for(std::chrono::milliseconds(delay / 2));
}

int main()
{
	CreateMutexA(0, FALSE, "SPRINTMOD");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		return 0;
	extract_keys(forward, sprint);
	std::thread setup_keybd(keybd_io);

	INPUT ip[2] = { 0, 0 };
	ip[0].type = INPUT_KEYBOARD; ip[0].ki.wVk = sprint;
	ip[1].type = INPUT_KEYBOARD; ip[1].ki.wVk = sprint;
	ip[1].ki.dwFlags = KEYEVENTF_KEYUP;

	while (true)
	{
		static int delay = 50;
		if (GetForegroundWindow() == FindWindow(NULL, "Minecraft") && is_sprinting)
		{
			CURSORINFO cursor = { sizeof(cursor) };
			if (GetCursorInfo(&cursor))
			{
				if (cursor.flags == 0)
					set_sprint(ip, delay);
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
		std::cout << "\rsprint: " << is_sprinting;
	}
	return 0;
}