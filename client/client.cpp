#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <ws2tcpip.h>
#include <conio.h>
#include <string>
#include <thread>
#include <iomanip>
#include <vector>

using namespace std;

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

#define PAUSE 1

SOCKET ConnectSocket = INVALID_SOCKET;

COORD server_smile;
COORD client_smile;

enum class KeyCodes { LEFT = 75, RIGHT = 77, UP = 72, DOWN = 80, ENTER = 13, ESCAPE = 27, SPACE = 32, ARROWS = 224 };
enum class Colors { BLUE = 9, RED = 12, BLACK = 0, YELLOW = 14, DARKGREEN = 2 };

bool gameRunning = true;
int elapsedSeconds = 0;

struct Coin {
    int x;
    int y;
    bool taken;
};

vector<Coin> server_coins;
vector<Coin> client_coins;
int server_coin_count = 0;
int client_coin_count = 0;

DWORD WINAPI TimerThread(LPVOID lpParam) {
    while (gameRunning) {
        this_thread::sleep_for(chrono::seconds(1));
        elapsedSeconds++;

        int hours = elapsedSeconds / 3600;
        int minutes = (elapsedSeconds % 3600) / 60;
        int seconds = elapsedSeconds % 60;

        string title = "Elapsed Time: ";
        title += to_string(hours) + ":" + to_string(minutes) + ":" + to_string(seconds);
        wstring wideTitle(title.begin(), title.end());

        SetConsoleTitle(wideTitle.c_str());

        if (minutes >= 1) {
            system("cls");
            cout << "SERVER: " << server_coin_count << endl;
            cout << "CLIENT: " << client_coin_count << endl;
            gameRunning = false;
        }
    }

    return 0;
}

DWORD WINAPI Sender(void* param)
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_CURSOR_INFO cursor;
    cursor.bVisible = false;
    cursor.dwSize = 100;
    SetConsoleCursorInfo(h, &cursor);

    while (true)
    {
        KeyCodes code = (KeyCodes)_getch();
        if (code == KeyCodes::ARROWS) code = (KeyCodes)_getch();

        SetConsoleCursorPosition(h, client_smile);
        cout << " ";

        int direction = 0;

        if (code == KeyCodes::LEFT) {
            client_smile.X--;
            direction = 1;
        }
        else if (code == KeyCodes::RIGHT) {
            client_smile.X++;
            direction = 2;
        }
        else if (code == KeyCodes::UP) {
            client_smile.Y--;
            direction = 3;
        }
        else if (code == KeyCodes::DOWN) {
            client_smile.Y++;
            direction = 4;
        }

        SetConsoleCursorPosition(h, client_smile);
        SetConsoleTextAttribute(h, WORD(Colors::BLUE));
        cout << (char)1;

        string coinPositions = "";
        for (const Coin& coin : client_coins) {
            coinPositions += to_string(coin.x) + "," + to_string(coin.y) + ";";
        }
        char message[200];
        strcpy_s(message, 199, (to_string(direction) + ";" + coinPositions).c_str());

        int iResult = send(ConnectSocket, message, (int)strlen(message), 0);
        if (iResult == SOCKET_ERROR) {
            cout << "send failed with error: " << WSAGetLastError() << "\n";
            closesocket(ConnectSocket);
            WSACleanup();
            return 15;
        }
    }

    return 0;
}

void ParseData(char data[], char**& map, unsigned int& rows, unsigned int& cols)
{
    string info = data;
    int h_index = info.find("h");
    int w_index = info.find("w");
    int d_index = info.find("d");
    string r = "";
    if (w_index == 2) r = info.substr(1, 1);
    else r = info.substr(1, 2);
    int rows_count = stoi(r);
    int distance = d_index - w_index - 1;
    string columns = info.substr(w_index + 1, distance);
    int columns_count = stoi(columns);
    string map_data = info.substr(d_index + 1, rows_count * columns_count);

    rows = rows_count;
    cols = columns_count;
    int map_data_current_element = 0;
    map = new char* [rows_count];
    for (int y = 0; y < rows_count; y++)
    {
        map[y] = new char[columns_count];
        for (int x = 0; x < columns_count; x++)
        {
            map[y][x] = map_data[map_data_current_element];
            map_data_current_element++;
        }
    }
}

void ShowMap(char** map, const unsigned int height, const unsigned int width) {
    setlocale(0, "C");
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            bool coin_displayed = false;
            for (const Coin& coin : server_coins) {
                if (coin.x == x && coin.y == y && !coin.taken) {
                    SetConsoleTextAttribute(h, WORD(Colors::YELLOW));
                    cout << "o";
                    coin_displayed = true;
                    break;
                }
            }
            if (coin_displayed) continue;

            for (const Coin& coin : client_coins) {
                if (coin.x == x && coin.y == y && !coin.taken) {
                    SetConsoleTextAttribute(h, WORD(Colors::YELLOW));
                    cout << "o";
                    coin_displayed = true;
                    break;
                }
            }
            if (coin_displayed) continue;

            if (map[y][x] == ' ')
            {
                SetConsoleTextAttribute(h, WORD(Colors::BLACK));
                cout << " ";
            }
            else if (map[y][x] == '#')
            {
                SetConsoleTextAttribute(h, WORD(Colors::DARKGREEN));
                cout << (char)219;
            }
            else if (map[y][x] == '1')
            {
                SetConsoleTextAttribute(h, WORD(Colors::RED));
                cout << (char)1;
                server_smile.X = x;
                server_smile.Y = y;
            }
            else if (map[y][x] == '2')
            {
                SetConsoleTextAttribute(h, WORD(Colors::BLUE));
                cout << (char)1;
                client_smile.X = x;
                client_smile.Y = y;
            }
        }
        cout << "\n";
    }
}

void CheckCoinCollision() {
    for (Coin& client_coin : client_coins) {
        if (client_coin.x == server_smile.X && client_coin.y == server_smile.Y && !client_coin.taken) {
            client_coin.taken = true;
            client_coin_count++;
        }
    }

    for (Coin& server_coin : server_coins) {
        if (server_coin.x == client_smile.X && server_coin.y == client_smile.Y && !server_coin.taken) {
            server_coin.taken = true;
            server_coin_count++;
        }
    }
}

DWORD WINAPI Receiver(void* param)
{
    char answer[DEFAULT_BUFLEN];
    int iResult = recv(ConnectSocket, answer, DEFAULT_BUFLEN, 0);
    answer[iResult] = '\0';

    char** map = nullptr;
    unsigned int rows;
    unsigned int cols;
    ParseData(answer, map, rows, cols);
    ShowMap(map, rows, cols);

    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

    while (true)
    {
        int iResult = recv(ConnectSocket, answer, DEFAULT_BUFLEN, 0);
        answer[iResult] = '\0';

        string message = answer;

        SetConsoleCursorPosition(h, server_smile);
        SetConsoleTextAttribute(h, WORD(Colors::BLACK));
        cout << " ";

        if (message == "1") {
            server_smile.X--;
        }
        else if (message == "2") {
            server_smile.X++;
        }
        else if (message == "3") {
            server_smile.Y--;
        }
        else if (message == "4") {
            server_smile.Y++;
        }

        SetConsoleCursorPosition(h, server_smile);
        SetConsoleTextAttribute(h, WORD(Colors::RED));
        cout << (char)1;

        if (iResult > 0) {
            CheckCoinCollision();
        }
        else if (iResult == 0)
            cout << "соединение с сервером закрыто.\n";
        else
            cout << "recv failed with error: " << WSAGetLastError() << "\n";
    }
    return 0;
}

int main()
{
    setlocale(0, "");
    system("title CLIENT SIDE");
    MoveWindow(GetConsoleWindow(), 50, 50, 500, 500, true);

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        cout << "WSAStartup failed with error: " << iResult << "\n";
        return 11;
    }

    addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    const char* ip = "localhost";
    addrinfo* result = NULL;
    iResult = getaddrinfo(ip, DEFAULT_PORT, &hints, &result);

    if (iResult != 0) {
        cout << "getaddrinfo failed with error: " << iResult << "\n";
        WSACleanup();
        return 12;
    }
    else {
        Sleep(PAUSE);
    }

    for (addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (ConnectSocket == INVALID_SOCKET) {
            cout << "socket failed with error: " << WSAGetLastError() << "\n";
            WSACleanup();
            return 13;
        }

        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        cout << "невозможно подключиться к серверу!\n";
        WSACleanup();
        return 14;
    }

    server_coins.push_back({ 3, 4, false });
    server_coins.push_back({ 7, 2, false });
    server_coins.push_back({ 8, 5, false });
    server_coins.push_back({ 6, 8, false });
    server_coins.push_back({ 1, 9, false });

    CreateThread(0, 0, Sender, 0, 0, 0);
    CreateThread(0, 0, Receiver, 0, 0, 0);
    CreateThread(0, 0, TimerThread, 0, 0, 0);
    Sleep(INFINITE);
}
