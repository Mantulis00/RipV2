#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <vector>
#include <string>
#include <Windows.h>
#include <cstdio>
#include <ctime>

using namespace std;

struct Message
{
	string Sender;
	string Receiver;
	vector<string> path;
	int hops =0;
	bool underProcess = false;
};

struct DirectMessage
{
	string Receiver;
	vector<string> path;
	string message;
};

struct RouterInfo
{
	string ip;
	thread router;
	int status = 0; // status 0 no actions // status 1 got message
	vector<Message> gotMessage;
	vector<string> connectedTo;
	bool isUp = true;
	vector<Message> completePath;
	vector<Message> saves;

	vector <DirectMessage> dirMessages;
	vector <DirectMessage> buildMessages;
};

vector<string> routerConfig;
RouterInfo routers[8];
ofstream fd("log.txt");




Message CopyMessage(Message message) // router copies message which (potentially) ll be sent to other routers
{
	Message newMessage;
	newMessage.Sender = (message).Sender;
	newMessage.Receiver = (message).Receiver;
	newMessage.hops = (message).hops + 1;

	for (int x = 0; x < (message).path.size(); x++)
	{
		newMessage.path.push_back((message).path[x]);
	}


	return newMessage;
}

int FindRouter(string ip) // find router configuration by ip
{
	for (int x = 0; x < routerConfig.size(); x++)
	{
		if (routerConfig[x] == ip)
		{
			return x;
		}
	}
	cout << "ip not found" << endl;
	return -1;
}


bool BackgroundCheck(vector<string> myConnections, string myIp) // prevents looping
{
	for (int x = 0; x < myConnections.size(); x++)
	{
		if (myConnections[x] == myIp) return false;
	}
	return true;
}


// routerinfo routers.message -> send gotmessage to router
// to send change routers message ant status
void AskForTable(vector<string> *myConnections, Message message, string myIp)
{
	cout << myIp << " " << (message).hops << endl;
	if ((message).Receiver != myIp)
	{
		for (int x = 0; x < (*myConnections).size(); x++)
		{
			if ((message).hops < 16 && BackgroundCheck((message).path, myIp) && routers[FindRouter((*myConnections)[x])].isUp) // send message to router
			{
				Message newMessage = CopyMessage(message);
				newMessage.path.push_back(myIp);


				routers[FindRouter((*myConnections)[x])].gotMessage.push_back( newMessage);

			}
		}
	}
	//
	else
	{
		
		Sleep(100);
		Message newMessage = CopyMessage(message);
		newMessage.path.push_back(myIp);
		cout << "PATH FOUND : ";
		for (int x = 0; x < newMessage.path.size(); x++)
		{
			cout << " " << newMessage.path[x];
		}
		cout << endl;


		routers[FindRouter(myIp)].completePath.push_back((newMessage));
		
	}
	
	Sleep(100);
}


void ReturnTableByPath(string myIp, Message *message) // unfold path, return path table to sender
{
	cout << myIp << endl;
	
	int currentRouter = FindRouter(myIp);
	int receiverRouter;

	for (int x = 0; x < (*message).path.size(); x++)
	{
		if ((*message).path[x] == myIp)
		{
			if (x == 0) // found
			{
				cout << "Got message" << endl;
				return;
			}

			receiverRouter = FindRouter((*message).path[x - 1]);
			break;
		}	
	}

		routers[receiverRouter].completePath.push_back( (*message));
		routers[receiverRouter].status = 2;
	
	
}


void SortSaves(vector<Message> *saves)
{
	Message k;
	for (int x = 0; x < (*saves).size(); x++)
	{
		for (int y = x+1; y < (*saves).size(); y++)
		{
			if ((*saves)[x].hops > (*saves)[y].hops)
			{
				k = (*saves)[x];
				(*saves)[x] = (*saves)[y];
				(*saves)[y] = k;
			}
		}
	}
}


void WriteLog(double time, string ip, bool isUp)
{
	string log;
	log += to_string(time);
	log += " ";
	log += ip;
	log += " ";

	if (isUp) log += "working";
	else log += "down";

	log += '\n';
	fd << log;
}


void RouteMessage(DirectMessage message, string myIp)
{
	for (int x = 0; x < message.path.size(); x++)
	{
		if (message.path[x] == myIp)
		{
			if (x + 1 == message.path.size())
				cout << "you got mail : " << message.message << endl;
			else // send message to second in the path router
				routers[FindRouter(message.path[x + 1])].buildMessages.push_back(message);
		}
	}
}

int CheckPosibility(vector<Message> completeMessages, string Receiver)
{

	for (int x = 0; x < completeMessages.size(); x++)
	{
		if (completeMessages[x].path[completeMessages[x].path.size() - 1] == Receiver)
		{
			return x;
		}
		Sleep(10);
	}
	return -1;
}


void Router(RouterInfo *router)
{
	int sizer = 0;

	clock_t lastTime;
	int duration = 3000;
	int radius = 500;

	lastTime = clock();

	while (true)
	{
		
		if (clock()  > lastTime + duration)
		{
			lastTime += duration;
			WriteLog(clock(), (*router).ip, (*router).isUp);
		}



		if ((*router).gotMessage.size() > 0 )
		{
				AskForTable(&(*router).connectedTo, (*router).gotMessage[0], (*router).ip);
				Sleep(1000);
				(*router).gotMessage.erase((*router).gotMessage.begin());	
		}
		if ((*router).completePath.size() > 0)
		{
			if ((*router).completePath[0].Sender == (*router).ip)
				(*router).saves.push_back((*router).completePath[0]);

				ReturnTableByPath((*router).ip, &(*router).completePath[0]);
				(*router).completePath.erase((*router).completePath.begin());

		}


		if ((*router).saves.size() > sizer)
		{
			sizer = (*router).saves.size();
			SortSaves(&(*router).saves);
			for (int x = 0 ;x < (*router).saves.size(); x++)
			{
				for (int y = 0; y < (*router).saves[x].path.size(); y++)
				{
					cout << (*router).saves[x].path[y] << " ";
				}
				cout << (*router).saves[x].hops << endl;
				Sleep(100);
			}
		}

		if ((*router).dirMessages.size() > 0 && (*router).saves.size() > 0 && CheckPosibility((*router).saves, (*router).dirMessages[0].Receiver) !=-1 )
		{
			SortSaves(&(*router).saves);
			(*router).dirMessages[0].path = (*router).saves[CheckPosibility((*router).saves, (*router).dirMessages[0].Receiver)].path;
			(*router).buildMessages.push_back((*router).dirMessages[0]);
			(*router).dirMessages.erase((*router).dirMessages.begin());
		}

		if ((*router).buildMessages.size() > 0)
		{

			RouteMessage((*router).buildMessages[0], (*router).ip);
			(*router).buildMessages.erase((*router).buildMessages.begin());
		}
		
		Sleep(100);
	}
}








///////////////////////////////////////////////////////////////// Routers congiruation
void ConfigRouters()
{
	routerConfig.push_back("0.0.0.0");
	routerConfig.push_back("1.0.0.0");
	routerConfig.push_back("2.0.0.0");
	routerConfig.push_back("3.0.0.0");
	routerConfig.push_back("4.0.0.0");
	routerConfig.push_back("5.0.0.0");
	routerConfig.push_back("6.0.0.0");
	routerConfig.push_back("7.0.0.0");
}

void SetRouterConfig()
{
	ifstream fs("duom.txt");

	while (!fs.eof())
	{
		string from, to;
		fs >> from >> to;
		routers[FindRouter(from)].connectedTo.push_back(to);
		routers[FindRouter(to)].connectedTo.push_back(from);
	}
}



void StartRouters()
{
	for (int x = 0; x < 8; x++)
	{
		routers[x].router = thread(Router, &routers[x]);
	}
}

void AsignIps()
{
	for (int x = 0; x < 8; x++)
	{
		routers[x].ip = routerConfig[x];
	}
}
//////////////////////////////////////////////////////////


int main()
{
	ConfigRouters();
	AsignIps();

	SetRouterConfig();
	routers[3].isUp = false;
	StartRouters();
	

	Sleep(100);

	Message message;
	message.hops = 0;
	message.Sender = routerConfig[1];
	message.Receiver = "7.0.0.0";
	message.underProcess = false;
	routers[1].gotMessage.push_back ( message);

	DirectMessage dirMessage; 
	dirMessage.Receiver  = message.Receiver;
	dirMessage.message = "ofc";
	routers[1].dirMessages.push_back(dirMessage);


	routers[1].status = 0;


	while (true)
	{
		Sleep(1);
	}


	return 1;
}


