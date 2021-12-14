#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<winsock2.h>
#include<ws2tcpip.h>
#include<cstdio>
#include<iostream>
#include<thread>
#include<mutex>
#define MAXN 512
#define DEFAULT_PORT "4579"
std::mutex mutex_time;
std::mutex mutex_name;
std::mutex mutex_user;
std::mutex mutex_message;
std::condition_variable Time;
std::condition_variable Name;
std::condition_variable User;
std::condition_variable Message;
void info()
{
	printf("input the operation number:1. connect 2. exit 3. get server time 4. get server name 5. get user list 6. send a message\n");
}
int connection_count = 0;
int receive(SOCKET connect_socket)
{
	//printf("receive");
	char buffer[MAXN];
	int flag;
	do
	{
		memset(buffer, 0, sizeof(buffer));
		flag = recv(connect_socket, buffer, MAXN, 0);
		if (flag > 0)
		{
			std::string message = buffer;
			printf("%s\n", buffer);
			int x = message.find("#") + 1;

			char op = message[x];
			if (message[x + 1] != '*'&&(message[x + 1] != '$'||(op!='A'&&op!='E')))
			{
				printf("expected *, but %c found", message[x + 1]);
			}
			if (op == '1')
			{
				printf("server time:");
				for (int i = x+2;;i++)
				{
					if (message[i] == '$') break;
					printf("%c", message[i]);
				}
				puts("");
				Time.notify_one();
			}
			else if (op == '2')
			{
				printf("server name:");
				for (int i = x + 2;;i++)
				{
					if (message[i] == '$') break;
					printf("%c", message[i]);
				}
				puts("");
				Name.notify_one();
			}
			else if (op == '3')
			{

				//printf("%s\n", buffer);
				printf("user list:\n");
				for (int i = 1;i <= 30;i++) printf("-"); puts("");
				printf("|  ID  |       IP:port       |\n");
				for (int i = 1;i <= 30;i++) printf("-"); puts("");
				for (int i = x + 2;;i++)
				{

					if (message[i] == '$') break;
					printf("|");
					int id = 0;
					for (;;i++)
					{
						if (message[i] == ' ') break;
						id = id * 10 + message[i] - '0';
					}
					printf("%6d|", id);
					i++;
					
					int cou = 0;
					for (;;i++)
					{
					
						if (message[i] == '^') break;
						std::cout << message[i];
						cou++;
					}
					for (int j = 1;j <= 21 - cou;j++) printf(" ");
					puts("|");
				}
				for (int i = 1;i <= 30;i++) printf("-"); puts("");
				User.notify_one();
			}
			else if (op == '4')
			{
				int y = message.find("*");
				printf("get message from ");
				int idx = message.find("*", y + 1);
				for (int i = y + 1;i < idx;i++) std::cout << message[i];
				printf(":\n");
				int len = 0;
				y = message.find("*", idx + 1);
				for (int i = idx + 1;i < y; i++)
				{
					len = len * 10 + message[i] - '0';
				}
				for (int i = y + 1;i < y+1+len;i++)
				{
					std::cout << message[i];
				}
				puts("");
				
			}
			else if (op == 'A')
			{
				printf("Message has been sent.\n");
				Message.notify_one();
			}
			else if (op == 'E')
			{
				printf("No such destination!\n");
				Message.notify_one();
			}
		}
		else if (flag == 0) Time.notify_one(), printf("Connection closed.\n");
		else printf("recv() failed!\n");
	} while (flag > 0);
	return 0;
}
void AskTime(SOCKET connect_socket)
{
	char data[] = "#1$";
	send(connect_socket, data, (int)strlen(data), 0);

	std::unique_lock<std::mutex> lck(mutex_time);
	printf("start wait\n");
	Time.wait(lck);
	printf("wait ok\n");
}
void AskName(SOCKET connect_socket)
{
	char data[] = "#2$";
	send(connect_socket, data, (int)strlen(data), 0);

	std::unique_lock<std::mutex> lck(mutex_name);
	//printf("start wait\n");
	Name.wait(lck);
	//printf("wait ok\n");
}
void AskUser(SOCKET connect_socket)
{
	char data[] = "#3$";
	send(connect_socket, data, (int)strlen(data), 0);

	std::unique_lock<std::mutex> lck(mutex_user);
	User.wait(lck);
}
void TrySendMessage(SOCKET connect_socket)
{
	char data[MAXN<<2];
	printf("please input thread id\n");
	int id;
	char message[MAXN << 1];
	std::cin >> id;
	getchar();
	std::cout << "Please input your message, press enter to finish" << std::endl;
	std::cin.getline(message, MAXN << 1);
	int n = strlen(message);
	sprintf(data, "#4*%d*%d*%s$", id, n, message);
	//printf("%s", data);
	send(connect_socket, data, (int)strlen(data), 0);
	std::unique_lock<std::mutex> lck(mutex_message);
	Message.wait(lck);
}
int main()
{
	WSADATA wsaData;
	addrinfo* result = NULL, * p;
	addrinfo hints;
	while (1)
	{
		info();
		int op;
		char ip[MAXN];
		SOCKET connect_socket = INVALID_SOCKET;
		std::cin >> op;
		if (op == 1)
		{
			printf("please input ip:\n");
			std::cin >> ip;
			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			{
				printf("WSAStartup() error!\n");
				return 0;
			}
			ZeroMemory(&hints, sizeof(hints));
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;
			if (getaddrinfo(ip, DEFAULT_PORT, &hints, &result) != 0)
			{
				std::cout << "getaddinfo error!" << std::endl;
				WSACleanup();
				continue;
			}
			int success = 1;
			for (p = result;p != NULL;p = p->ai_next)
			{
				if ((connect_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == INVALID_SOCKET)

				{
					success = 0;
					printf("socket() error\n");
					WSACleanup();
				}
				if (connect(connect_socket, p->ai_addr, (int)p->ai_addrlen) == SOCKET_ERROR)
				{
					closesocket(connect_socket);
					connect_socket = INVALID_SOCKET;
					printf("connect() error!\n");
				}
				break;
			}
			if (success)
			{
				freeaddrinfo(result);
				if (connect_socket == INVALID_SOCKET)
				{
					std::cout << "connect to server error!" << std::endl;
					WSACleanup();
					continue;
				}
				else
				{
					std::cout << "connect successfully!" << std::endl;
					SOCKADDR_IN clientInfo = { 0 };
					int addr_size = sizeof(clientInfo);
					std::thread::id id = std::this_thread::get_id();

					getpeername(connect_socket, (sockaddr*)&clientInfo, &addr_size);
					char* ip = inet_ntoa(clientInfo.sin_addr);
					int port = clientInfo.sin_port;
					std::thread(receive, std::move(connect_socket)).detach();

					while (1)
					{

						//printf("ok");
						info();
						int op;
						std::cin >> op;
						if (op == 1)
						{
							printf("You have already connected.");
						}
						else if (op == 2)
						{
							closesocket(connect_socket);
							WSACleanup();
							printf("disconnect successfully!");
							break;
						}
						else if (op == 3)
						{
							AskTime(connect_socket);
						}
						else if (op == 4)
						{
							AskName(connect_socket);
						}
						else if (op == 5)
						{
							AskUser(connect_socket);
						}
						else if (op == 6)
						{
							TrySendMessage(connect_socket);
						}
					}
				}
			}

		}
		else if (op == 2)
		{
			printf("exit.");
			return 0;
		}
		else if(op>=3&&op<=6)
		{
			printf("please connect to a server first");
		}
		else
		{
		printf("invalid operation");
		}
	}
}