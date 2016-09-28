#include <stdlib.h>
#include <iostream>
#include <string>
#include <sstream>
#include <time.h>
#include "websocket.h"
#include <map>
#include <windows.h>
#include "Game.h"
#include <random>
#include "Messages.h"
#include <chrono>
#include <typeinfo>
using namespace std::chrono;

using namespace std;

webSocket server;

map<int, string> clientToGameID; //clientID -> Game
map<string, Game> gameIDToGame;

// #clientLatencyStuff
#include <numeric>
typedef vector<long long> Latency;

long long average(Latency *l)
{
	if (l->size() == 0)
	{
		return 0;
	}
	long long sum = std::accumulate(l->begin(), l->end(), 0);
	if (sum == 0)
	{
		return 0;
	}
	long long avg = sum / l->size();
	return avg;
}
map<int, Latency> clientToLatency;
#define SLOW_TIMER_MAX 500;
int slowTimer = SLOW_TIMER_MAX;

long long getTS(string timestamp)
{
	return atoll(timestamp.c_str());
}

long long generateCurrentTime()
{
	milliseconds ms = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch());
	return atoll(to_string(ms.count()).c_str());
}

void updateLatency(int clientID, long long ts)
{
	long long timeDifference = generateCurrentTime() - ts;
	Latency *l = &clientToLatency[clientID];
	l->push_back(timeDifference);
	int size = l->size();
	if (size >= 15)
	{
		l->erase(l->begin());
	}
}
void updateLatency(int clientID, string timestamp)
{
	updateLatency(clientID, getTS(timestamp));
}

Messages incoming = Messages();
Messages outgoing = Messages();



string converter(int integer)
{
	ostringstream c;
	c.flush();
	c << integer;
	return c.str();
}

string currentTimeStamp()
{
	milliseconds ms = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch());
	return "time " + to_string(ms.count());
}


int compareTimestamps(int compareWith)
{
	time_t current = time(NULL);
	return abs((current - compareWith));
}

int compareTimestamps(string compareWith)
{
	return compareTimestamps(atoi(compareWith.substr(4).c_str()));
}


/* called when a client connects */
void openHandler(int clientID)
{
}

/* called when a client disconnects */
void closeHandler(int clientID){

	//go to game, find other player, send game over. 
	int opp = gameIDToGame[clientToGameID[clientID]].getOpponent(clientID);
	server.wsSend(opp, "playerdc"); 
	gameIDToGame.erase(clientToGameID[clientID]);

	clientToGameID.erase(opp);
	clientToGameID.erase(clientID);

	// #clientLatencyStuff
	clientToLatency.erase(opp);
	clientToLatency.erase(clientID);

}


void messageParse(int clientID, string message)
{
	if (message.find("gameid") != string::npos)
	{
		string gameID = message.substr(21);
		

		if (gameIDToGame[gameID].gameFull() == 1) 
		{
			ostringstream os, as;
			os << "Game lobby '" << gameID << "' is already full. Please pick a new name.";
			as << "disconnect (server)";

			outgoing.addMessage(clientID, os.str(), generateCurrentTime());
			outgoing.addMessage(clientID, as.str(), generateCurrentTime());

			os.flush();
			as.flush();
		}
		else if (gameIDToGame[gameID].init() == 0)
		{
			
			ostringstream os;
			clientToGameID[clientID] = gameID;
			gameIDToGame[gameID] = Game{ gameID, server.getClientIP(clientID), clientID }; 
			os << "one ";

			outgoing.addMessage(clientID, os.str(), generateCurrentTime());

			os.flush();
		}
		else if (gameIDToGame[gameID].init() == 1)
		{
			ostringstream os, as;
			clientToGameID[clientID] = gameID;
			gameIDToGame[gameID].addPlayer2(clientID, server.getClientIP(clientID));

			as << "ready ";

			int opponentClientID = gameIDToGame[clientToGameID[clientID]].getOpponent(clientID);

			outgoing.addMessage(opponentClientID, as.str(), generateCurrentTime());
			outgoing.addMessage(clientID, as.str(), generateCurrentTime());

			outgoing.addMessage(opponentClientID, gameIDToGame[gameID].sendBallData(), generateCurrentTime());
			outgoing.addMessage(clientID, gameIDToGame[gameID].sendBallData(), generateCurrentTime());

			os.flush();
			as.flush();
		}
	}
	else if (message.find("paddle") != string::npos)
	{
		string timestamp = message.substr(0, 13);

		string thing = message.substr(22);
		string ypos = thing.substr(0, thing.find(" "));
		string yspeed = thing.substr(thing.find(" ") + 1);

		string gameID = clientToGameID[clientID];
		Game *game = &gameIDToGame[gameID];

		int value = atoi(ypos.c_str());
		int value2 = atoi(yspeed.c_str());

		int opponentID = game->getOpponent(clientID);


		if (message.find("paddle1") != string::npos)
		{
			game->readInput(1, value, value2);
		}
		else if (message.find("paddle2") != string::npos)
		{
			game->readInput(2, value, value2);
		}
		else
		{
			cout << "Invalid paddle message!" << endl;
		}
	}

}


/* called when a client sends a message to the server */
void messageHandler(int clientID, string message)
{
	if (message.empty())
	{
		std::cout << "message is empty" << std::endl;
		return;
	}
	string timestamp = message.substr(0, 13);
	updateLatency(clientID, timestamp);
	incoming.addMessage(clientID, message, getTS(timestamp));
}



double PCFreq = 0.0;
__int64 CounterStart = 0;

void StartCounter()
{
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		cout << "QueryPerformanceFrequency failed!\n";

	PCFreq = double(li.QuadPart) / 1000;

	QueryPerformanceCounter(&li);
	CounterStart = li.QuadPart;
}
double GetCounter()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - CounterStart) / PCFreq;
}



/* called once per select() loop */
void periodicHandler(){
	static time_t next = GetCounter() + (1000 / 60);
	time_t current = GetCounter();

	
	for (std::map<string, Game>::iterator it = gameIDToGame.begin(); it != gameIDToGame.end(); ++it)
	{
		if (it->second.gameFull())
		{
			it->second.update();
			int p1 = it->second.getClientID(1);
			int p2 = it->second.getClientID(2);

			outgoing.addMessage(p1, it->second.sendBallData(), generateCurrentTime());
			outgoing.addMessage(p2, it->second.sendBallData(), generateCurrentTime());

			outgoing.addMessage(p1, it->second.sendOpponentPaddleData(p1), generateCurrentTime());
			outgoing.addMessage(p2, it->second.sendOpponentPaddleData(p2), generateCurrentTime());

			outgoing.addMessage(p1, it->second.sendScore(), generateCurrentTime());
			outgoing.addMessage(p2, it->second.sendScore(), generateCurrentTime());
		}

	}



	incoming.update();
	while (incoming.timerIsZero() == 1)
	{
		if (incoming.messageCollection.size() > 0)
		{

			Messages::messagePair mp = incoming.popCurrentMessage();
			messageParse(mp.clientID, mp.message);
			updateLatency(mp.clientID, mp.timestamp);
		}

	}
	outgoing.update();
	while (outgoing.timerIsZero() == 1)
	{
		while (outgoing.messageCollection.size() > 0)
		{
			Messages::messagePair mp = outgoing.popCurrentMessage();
			server.wsSend(mp.clientID, mp.message);
			server.wsSend(mp.clientID, currentTimeStamp());
			updateLatency(mp.clientID, mp.timestamp);
		}
	}

	// #clientLatencyStuff
	vector<int> clientIDs = server.getClientIDs();
	slowTimer--;
	if ((slowTimer > 0) || (clientIDs.size() < 1))
	{
		return;
	}
	slowTimer = SLOW_TIMER_MAX;
	for (int i = 0; i < clientIDs.size(); i++)
	{
		std::cout << "Average latency for clientID " << clientIDs[i] << " is " << average(&clientToLatency[i]) << std::endl;
	}
	// END
}


int main(int argc, char *argv[]){
	int port = 9001;
	StartCounter();
	
	/* set event handler */
	server.setOpenHandler(openHandler);
	server.setCloseHandler(closeHandler);
	server.setMessageHandler(messageHandler);
	server.setPeriodicHandler(periodicHandler);

	/* start the chatroom server, listen to ip '127.0.0.1' and port '9001' */
	server.startServer(port);

	return 1;
}
