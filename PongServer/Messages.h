#ifndef MESSAGES_H
#define MESSAGES_H

#include <vector>
#include <random>
#include <stdlib.h>
#include <time.h>
class Messages
{
public:
	Messages();
	struct messagePair
	{
		int clientID = -9001;
		std::string message = "empty message";
		long long timestamp = -1;

	};
	void addMessage(int _clientID, std::string _message);
	void addMessage(int _clientID, std::string _message , long long _timestamp);
	messagePair popCurrentMessage();
	void resetTimer();
	void update();
	bool timerIsZero();
	std::vector<messagePair> messageCollection;
	int timer = -9001;


private:

};


#endif