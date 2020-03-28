#include <iostream>
#include "../util/strings.h"


/** wraps around the Dictionary to
  - reach the protected methods from StringsDictionary
  - to fake a communication of two FOs and emulate syncing of their Dictionaries */
class DictionaryBroadcaster: public StringsDictionary
{
public:
	void StartBroadcastFromMeToNetwork()
	{
		for (const auto& i : theseShouldBeBroadcast())
		{
			std::cout << "This is travelling via network: >>" << i.second << "<<\n";
			//NB: no need to send i.first == ID == hash, as receiver can calculate it too,
			//    but it does not hurt if it is sent too
		}
	}

	void StopBroadcastFromMeToNetwork()
	{
		//call this right after the broadcast is over...
		markAllWasBroadcast();

		//...and pass token on some other FO to start broadcasting its recent additions into its Dictionary
	}

	void ProcessItemFromNetwork(const StringsDictionary& remoteSource)
	{
		//normally the data arrives from MPI,
		//here, we fake this communication by reading out new stuff from remoteSource
		//
		//actually, the remoteSource.theseShouldBeBroadcast() should be considered in this fake
		//but we cannot reach it, luckily its showNewDictionary() gives the same data
		for (const auto& i : remoteSource.showNewDictionary())
		{
			enlistTheIncomingItem(i.first,i.second);
		}
	}
};


void reportAndSync(DictionaryBroadcaster& d1, DictionaryBroadcaster& d2)
{
	//reporting:
	std::cout << "========================================\n";
	std::cout << "Dict 1:\n";
	d1.printCompleteDictionary();
	std::cout << "\nDict 2:\n";
	d2.printCompleteDictionary();
	std::cout << "\n";

	//syncing (fake transfers), the MPI point of view
	d1.StartBroadcastFromMeToNetwork();
	d2.ProcessItemFromNetwork(d1);
	d1.StopBroadcastFromMeToNetwork();

	d2.StartBroadcastFromMeToNetwork();
	d1.ProcessItemFromNetwork(d2);
	d2.StopBroadcastFromMeToNetwork();

	//reporting:
	std::cout << "\nDict 1:\n";
	d1.printCompleteDictionary();
	std::cout << "\nDict 2:\n";
	d2.printCompleteDictionary();
	std::cout << "========================================\n";
}


int main(void)
{
try
{
	//FO 1, this is the user/simulation point of view
	DictionaryBroadcaster d1;

	hashedString agent1_1("nucleus 1"); //create new agentType for whatever reason
	d1.registerThisString(agent1_1);

	char buf[StringsImprintSize];
	agent1_1.printIntoBuffer(buf,StringsImprintSize);
	std::cout << "imprinted (0-pad): >>" << buf << "<<\n";
	agent1_1.printIntoBuffer(buf,StringsImprintSize,'B');
	std::cout << "imprinted (B-pad): >>" << buf << "<<\n";
	std::cout << "couted           : >>" << agent1_1 << "<<\n";

	hashedString agent1_2("nucleus 2");
	d1.registerThisString(agent1_2);

	//FO 2, this is the user/simulation point of view
	DictionaryBroadcaster d2;

	std::string agent2_1("nucleus 2");
	d2.registerThisString(agent2_1);

	std::string agent2_2("nucleus 3");
	d2.registerThisString(agent2_2);

	reportAndSync(d1,d2);

	//FO 1, this is the user/simulation point of view
	d1.registerThisString(std::string("nucleus 3"));
	d1.registerThisString(std::string("nucleus 5"));
	d1.registerThisString(agent1_1);
	//FO 2, this is the user/simulation point of view

	reportAndSync(d1,d2);

	//FO 1, some testing:
	std::cout << "My  agent1: " << agent1_1.getString() << " of hash " << agent1_1.getHash() << "\n";
	std::cout << "Dictionary: " << d1.translateIdToString(agent1_1.getHash()) << "\n\n";

	std::cout << "My  agent2: " << agent1_2.getString() << " of hash " << agent1_2.getHash() << "\n";
	std::cout << "Dictionary: " << d1.translateIdToString(agent1_2.getHash()) << "\n\n";

	//must err
	d1.translateIdToString(agent1_2.getHash()+1);
}
catch (std::runtime_error* e)
{
	std::cout << "ERR: " << e->what() << "\n";
}
catch (...)
{
	std::cout << "some other error\n";
}

	return 0;
}
