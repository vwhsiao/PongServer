#include "Messages.h"

Messages::Messages()
{
	srand(time(NULL));
	resetTimer();
}
void Messages::addMessage(int _clientID, std::string _message)
{
	Messages::messagePair newPair;
	newPair.clientID = _clientID;
	newPair.message = _message;
	Messages::messageCollection.push_back(newPair);
}


void Messages::addMessage(int _clientID, std::string _message, long long _timestamp)
{
	Messages::messagePair newPair;
	newPair.clientID = _clientID;
	newPair.message = _message;
	newPair.timestamp = _timestamp;
	Messages::messageCollection.push_back(newPair);
}

Messages::messagePair Messages::popCurrentMessage()
{
	Messages::messagePair mP = Messages::messageCollection[0];
	Messages::messageCollection.erase(Messages::messageCollection.begin());
	return mP;
}

void Messages::resetTimer()
{	
	Messages::timer = rand() % 4 + 1;
}


void Messages::update()
{
	Messages::timer--;
}

bool Messages::timerIsZero()
{
	bool resetted = (Messages::timer == 0);
	if (resetted)
	{
		Messages::resetTimer();
	}
	return resetted;
}