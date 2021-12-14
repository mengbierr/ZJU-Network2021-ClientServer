#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<cstdio>
#include<winsock2.h>
#include<thread>
#include<mutex>
#include<vector>
#include<string>
#define DEFAULT_PORT 4579
#define SERVER_NAME "ARC-POTATO"
#define MAXN 512
std::mutex lock;
struct Client
{
	char* ip;
	int port;
	int id;
	std::thread::id thread_id;
	SOCKET socket;
};
std::vector<struct Client*> client_list;
int count = 0;
void Send(SOCKET sClient, std::string ans)
{
	char cans[MAXN];
	memset(cans, 0, sizeof(cans));
	strcpy(cans, ans.c_str());
	int len = ans.length();
	send(sClient, cans, len, 0);
}
void OutputTime(SOCKET sClient)
{
	//printf("start send");
	using std::chrono::system_clock;
	std::string ans = "#1*";
	std::time_t t;
	t = system_clock::to_time_t(system_clock::now());
	ans += ctime(&t);
	ans += "$\0";
	Send(sClient, ans);
}
void OutputName(SOCKET sClient)
{
	std::string ans = "#2*";
	ans += SERVER_NAME;
	ans += "$\0";
	Send(sClient, ans);
}
void OutputUser(SOCKET sClient)
{
	std::string ans = "#3*";
	std::vector<Client*>::iterator it;
	for (it = client_list.begin();it != client_list.end();it++)
	{
		ans += std::to_string((*it)->id)+" ";
		ans += (*it)->ip;
		ans += ":" + std::to_string((*it)->port);
		ans += "^";
	}
	ans += "$\0";
	Send(sClient, ans);
}
bool exist_id(int id)
{
	bool flag = 0;
	lock.lock();
	std::vector<Client*>::iterator it;
	for (it = client_list.begin();it != client_list.end();it++)
	{
		if ((*it)->id == id)
		{
			flag = 1;
			break;
		}
	}
	lock.unlock();
	return flag;
}
SOCKET FindSocket(int id)
{
	SOCKET now;
	lock.lock();
	std::vector<Client*>::iterator it;
	for (it = client_list.begin();it != client_list.end();it++)
	{
		if ((*it)->id == id)
		{
			now = (*it)->socket;
			break;
		}
	}
	lock.unlock();
	return now;
}
int ProcessRequest(SOCKET sClient, char* buffer)
{
	printf("start process");
	if (buffer[0] != '#')
	{
		printf("expected #, but %c found.\n", buffer[0]);
		return -1;
	}
	char op = buffer[1];
	if (op == '1')
	{
		OutputTime(sClient);
	}
	else if (op == '2')
	{
		OutputName(sClient);
	}
	else if (op == '3')
	{
		OutputUser(sClient);
	}
	else if (op == '4')
	{
		SOCKET dest;
		std::string message;
		std::string packet = buffer;
		int x = packet.find("*");
		int y = packet.find("*", x+1);
		int id = 0;
		for (int i = x + 1;i < y;i++)
		{
			id = id * 10 + packet[i] - '0';
		}
		if (exist_id(id))
		{
			dest = FindSocket(id);
			std::string info = "#4*";
			info += std::to_string(id);
			info += "*";
			for (int i = y + 1;;i++)
			{
				if (packet[i] == '$') break;
				info += packet[i];
			}
			info += "$";
			Send(dest, info);
			Send(sClient, "#A$");
		}
		else
		{
			Send(sClient, "#E$");
		}

	}
	return 1;
}
void solve(SOCKET sClient)
{
	SOCKADDR_IN client_info = { 0 };
	int addrsize = sizeof(client_info);
	std::thread::id thread_id = std::this_thread::get_id();
	getpeername(sClient, (struct sockaddr*)&client_info, &addrsize);
	char* ip = inet_ntoa(client_info.sin_addr);
	int port = client_info.sin_port;
	Client now;
	now.socket = sClient;now.ip = ip;now.id = count++;
	now.port = port;now.thread_id = thread_id;
	Client* pnow = &now;
	lock.lock();
	client_list.push_back(pnow);
	lock.unlock();
	bool finish = 0;
	char buffer[MAXN] = "\0";
	while (!finish)
	{
		ZeroMemory(buffer, MAXN);
		int state = recv(sClient, buffer, MAXN, 0);
		if (state > 0)
		{
			ProcessRequest(sClient, buffer);
		}
		else if (state == 0)
		{
			printf("Connection closed.\n");
			finish = 1;
		}
		else
		{
			printf("rev() error!\n");
			closesocket(sClient);
			return;
		}
	}
	closesocket(sClient);
	std::vector<Client*>::iterator it;
	for (it = client_list.begin();it != client_list.end();it++)
	{
		if ((*it)->id == pnow->id && (*it)->ip == pnow->ip) break;
	}
	lock.lock();
	client_list.erase(it);
	lock.unlock();
	return;
}
int main()
{
	WSADATA wsaData;
	SOCKET sListen, sClient;
	SOCKADDR_IN servAddr, clntAddr;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup() error!\n");
		return 0;
	}
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == INVALID_SOCKET)
	{
		printf("socket() error");
		return 0;
	}
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(DEFAULT_PORT);
	if (bind(sListen, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
	{
		printf("bind() error! code:%d\n", WSAGetLastError());
		closesocket(sListen);
		return 0;
	}

	if (listen(sListen, 5) == SOCKET_ERROR)
	{
		printf("listen() error! code:%d\n", WSAGetLastError());
		closesocket(sListen);
		return 0;
	}


	while (1)
	{
		printf("start solve");
		int szClntAddr = sizeof(clntAddr);
		sClient = accept(sListen, (SOCKADDR*)&clntAddr, &szClntAddr);
		if (sClient == INVALID_SOCKET)
		{
			printf("accept() error! code:%d\n", WSAGetLastError());
			closesocket(sClient);
			return 0;
		}

		printf("%s\n", inet_ntoa(clntAddr.sin_addr));
		std::thread(solve, std::ref(sClient)).detach();
	}

	closesocket(sListen);
	WSACleanup();
	return 0;
}