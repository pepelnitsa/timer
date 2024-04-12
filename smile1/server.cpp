#define WIN32_LEAN_AND_MEAN // To speed the build process: https://stackoverflow.com/questions/11040133/what-does-defining-win32-lean-and-mean-exclude-exactly

#include <iostream>
#include <string>
#include <windows.h>
#include <ws2tcpip.h> // WSADATA type; WSAStartup, WSACleanup functions and many more
using namespace std;

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015" // a port is a logical construct that identifies a specific process or a type of network service - https://en.wikipedia.org/wiki/Port_(computer_networking)

#define PAUSE 1

// Accept a client socket
SOCKET ClientSocket = INVALID_SOCKET;

void GenerateMap(char map[][9], const unsigned int height, const unsigned int width) {
	for (int y = 0; y < height; y++) // перебiр рядкiв
	{
		for (int x = 0; x < width; x++) // перебiр кожноi комiрки в межах рядка
		{
			map[y][x] = ' '; // за замовчуванням всi комiрки - це коридори 
			if (x == 0 || x == width - 1 || y == 0 || y == height - 1)
				map[y][x] = '#'; // стiнка
			if (x == width - 1 && y == height / 2)
				map[y][x] = ' '; // вихiд 
			if (x == 1 && y == 1)
				map[y][x] = '1'; // гравець сервера
			if (x == 1 && y == 3)
				map[y][x] = '2'; // гравець клiента
		}
	}
}

void ShowMap(char map[][9], const unsigned int height, const unsigned int width) {
	setlocale(0, "C");
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	for (int y = 0; y < height; y++) // перебiр рядкiв
	{
		for (int x = 0; x < width; x++) // перебiр кожноi комiрки в межах рядка
		{
			if (map[y][x] == ' ')
			{
				SetConsoleTextAttribute(h, 0);
				cout << " ";
			}
			else if (map[y][x] == '#')
			{
				SetConsoleTextAttribute(h, 2);
				cout << (char)219; // cout << "#"; 
			}
			else if (map[y][x] == '1')
			{
				SetConsoleTextAttribute(h, 12);
				cout << (char)1; // cout << "@";
			}
			else if (map[y][x] == '2')
			{
				SetConsoleTextAttribute(h, 9);
				cout << (char)1; // cout << "@";
			}
		}
		cout << "\n"; // перехiд на наступний рядок
	}
}

string MakeMessage(char map[][9], const unsigned int height, const unsigned int width)
{
	string message = "h";
	message += to_string(height); // h5
	message += "w"; // h5w
	message += to_string(width); // h5w9
	message += "d"; // d = data
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			message += map[y][x];
	return message;

	/*string message; // "h5w9data#########         @@@"
	message = "h"; // h
	message = message + to_string(height); // h5
	message = message + "w"; // h5w
	message = message + to_string(width); // h5w9
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			message = message + map[y][x];
		}
	}*/
}

DWORD WINAPI Sender(void* param)
{
	// формування локацii
	const unsigned int height = 5; // <<< висота рiвня в рядках
	const unsigned int width = 9; // <<< ширина рiвня в стовпчиках
	char map[height][width] = {}; // статичний масив для зберiгання всiх об'ектiв локацii
	GenerateMap(map, height, width);
	system("cls");
	ShowMap(map, height, width);

	string message = MakeMessage(map, height, width);
	// cout << message << "\n";

	int iSendResult = send(ClientSocket, message.c_str(), message.size(), 0); // The send function sends data on a connected socket: https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-send
	if (iSendResult == SOCKET_ERROR) {
		cout << "send failed with error: " << WSAGetLastError() << "\n";
		cout << "упс, отправка (send) ответного сообщения не состоялась ((\n";
		closesocket(ClientSocket);
		WSACleanup();
		return 7;
	}

	while (true) {
		char answer[200];
		// _getch()
		cin.getline(answer, 199);
		int iSendResult = send(ClientSocket, answer, strlen(answer), 0); // The send function sends data on a connected socket: https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-send

		if (iSendResult == SOCKET_ERROR) {
			cout << "send failed with error: " << WSAGetLastError() << "\n";
			cout << "упс, отправка (send) ответного сообщения не состоялась ((\n";
			closesocket(ClientSocket);
			WSACleanup();
			return 7;
		}
	}

	return 0;
}

DWORD WINAPI Receiver(void* param)
{
	while (true) {
		char message[DEFAULT_BUFLEN];

		int iResult = recv(ClientSocket, message, DEFAULT_BUFLEN, 0); // The recv function is used to read incoming data: https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-recv
		message[iResult] = '\0';

		if (iResult > 0)
			cout << message << "\n"; // 1 або 2
	}
	return 0;
}

int main()
{
	setlocale(0, "");
	system("title SERVER SIDE");

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	// Initialize Winsock
	WSADATA wsaData; // The WSADATA structure contains information about the Windows Sockets implementation: https://docs.microsoft.com/en-us/windows/win32/api/winsock/ns-winsock-wsadata
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // The WSAStartup function initiates use of the Winsock DLL by a process: https://firststeps.ru/mfc/net/socket/r.php?2
	if (iResult != 0) {
		cout << "WSAStartup failed with error: " << iResult << "\n";
		cout << "подключение Winsock.dll прошло с ошибкой!\n";
		return 1;
	}
	else {
		// cout << "подключение Winsock.dll прошло успешно!\n";
		Sleep(PAUSE);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct addrinfo hints; // The addrinfo structure is used by the getaddrinfo function to hold host address information: https://docs.microsoft.com/en-us/windows/win32/api/ws2def/ns-ws2def-addrinfoa
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET; // The Internet Protocol version 4 (IPv4) address family
	hints.ai_socktype = SOCK_STREAM; // Provides sequenced, reliable, two-way, connection-based byte streams with a data transmission mechanism
	hints.ai_protocol = IPPROTO_TCP; // The Transmission Control Protocol (TCP). This is a possible value when the ai_family member is AF_INET or AF_INET6 and the ai_socktype member is SOCK_STREAM
	hints.ai_flags = AI_PASSIVE; // The socket address will be used in a call to the "bind" function

	// Resolve the server address and port
	struct addrinfo* result = NULL;
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed with error: " << iResult << "\n";
		cout << "получение адреса и порта сервера прошло c ошибкой!\n";
		WSACleanup(); // The WSACleanup function terminates use of the Winsock 2 DLL (Ws2_32.dll): https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsacleanup
		return 2;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Create a SOCKET for connecting to server
	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "socket failed with error: " << WSAGetLastError() << "\n";
		cout << "создание сокета прошло c ошибкой!\n";
		freeaddrinfo(result);
		WSACleanup();

		return 3;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen); // The bind function associates a local address with a socket: https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-bind
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed with error: " << WSAGetLastError() << "\n";
		cout << "внедрение сокета по IP-адресу прошло с ошибкой!\n";
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 4;
	}

	freeaddrinfo(result);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	iResult = listen(ListenSocket, SOMAXCONN); // The listen function places a socket in a state in which it is listening for an incoming connection: https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen
	if (iResult == SOCKET_ERROR) {
		cout << "listen failed with error: " << WSAGetLastError() << "\n";
		cout << "прослушка информации от клиента не началась. что-то пошло не так!\n";
		closesocket(ListenSocket);
		WSACleanup();
		return 5;
	}
	else {
		cout << "пожалуйста, запустите client.exe\n";
		// здесь можно было бы запустить некий прелоадер в отдельном потоке
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	ClientSocket = accept(ListenSocket, NULL, NULL); // The accept function permits an incoming connection attempt on a socket: https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-accept
	if (ClientSocket == INVALID_SOCKET) {
		cout << "accept failed with error: " << WSAGetLastError() << "\n";
		cout << "соединение с клиентским приложением не установлено! печаль!\n";
		closesocket(ListenSocket);
		WSACleanup();
		return 6;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	CreateThread(0, 0, Receiver, 0, 0, 0);

	CreateThread(0, 0, Sender, 0, 0, 0);

	Sleep(INFINITE);
}