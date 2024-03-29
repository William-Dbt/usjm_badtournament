#include <iostream>
#include <cstdlib>
#include <exception>
#include <unistd.h>
#include "utils.hpp"
#include "Tournament.hpp"

void	getHistory(Tournament* tournament);
void	startMatch(Tournament* tournament);

bool	g_bFinishTournament = false;

Tournament::Tournament() {
	if (this->_mode == ALL_SIMPLE) {
		this->_commands["STATS"] = STATS;
	}
	this->_commands["JOUEUR"] = PLAYER;
	this->_commands["MATCH"] = MATCH;
	this->_commands["INFOS"] = INFOS;
	this->_commands["FIN"] = FINISH;
	// this->_commands["HISTORIQUE"] = HISTORY;

	this->_infos.nbCourts = 0;
}

Tournament::~Tournament() {
	std::map<const std::string, Player*>::iterator	it;

	for (it = this->_playersList.begin(); it != this->_playersList.end(); it++)
		delete (*it).second;
}

bool	Tournament::isCourtsFull() {
	if (this->_mode == ALL_SIMPLE) {
		if (this->_matchsInProgress.size() >= this->_infos.nbCourts)
			return true;
	}
	else
		if (this->_doubleMatchsInProgress.size() >= this->_infos.nbCourts)
			return true;

	return false;
}

bool	Tournament::startWithHistory() {
	std::string	buffer;

	printMessage("Voulez-vous commencer le tournoi à partir d'un historique existant? (O:oui/N:non) ", -1, false);
	std::getline(std::cin, buffer);
	if (isOui(buffer))
		return true;
	else if (isNon(buffer))
		return false;

	printMessage("Impossible de comprendre votre choix, commencement d'un nouveau tournoi!\n", WARNING);
	return false;	
}

bool	isValidName(std::string name, bool ignoreMinus) {
	std::string::iterator	it;

	if (name.empty())
		return false;

	if (name.size() == 0)
		return false;

	if (isStringEmpty(name))
		return false;

	for (it = name.begin(); it != name.end(); it++) {
		if (!ignoreMinus && it == name.begin() && (*it) == '-')
			continue ;

		if (!isalpha((*it)) && (*it) != '.') {
			printMessage("Le nom du joueur ne peut contenir que des lettres et le caractère \'.\'.", WARNING);
			return false;
		}
	}
	return true;
}

void	Tournament::savePlayers() {
	std::string	buffer;

	if (this->getNumberOfPlayers() == 0) {
		printMessage("\nEntrez le nom des joueurs participant au tournoi");
		printMessage("Si vous souhaitez enlever un participant retapez son nom avec le signe \'-\' devant celui-ci.");
		printMessage("Une fois terminé, tapez \"FIN\".");
	}
	while (std::getline(std::cin, buffer)) {
		if (!isValidName(buffer, false))
			continue ;

		if (buffer.compare("FIN") == 0)
			break ;

		if (buffer[0] == '-')
			this->removePlayer(buffer.c_str() + 1, false);
		else
			this->addPlayer(buffer);
	}
	if (this->getNumberOfPlayers() < 2) {
		printMessage("Pas assez de joueurs inscrits pour lancer le tournoi (2 minimum requis).", ERROR);
		exit(EXIT_FAILURE);
	}
	else if (this->_mode == ALL_DOUBLE &&  this->getNumberOfPlayers() < 4) {
		printMessage("Pas assez de joueurs inscrits pour lancer le tournoi (4 minimum requis).", ERROR);
		exit(EXIT_FAILURE);
	}
	this->showPlayers();

	printMessage("Y-a t'il une erreur dans la liste? (O:oui/N:non) ", -1, false);
	while (std::getline(std::cin, buffer)) {
		if (isOui(buffer)) {
			printMessage("Si vous souhaitez enlever un participant retapez son nom avec le signe \'-\' devant celui-ci.");
			printMessage("Pour en rajouter, tapez juste son nom. Entrez \'FIN\' une fois les modifications terminées.");
			return savePlayers();
		}
		else if (isNon(buffer))
			break ;
		else
			printMessage("Vous pouvez répondre uniquement avec oui (O/OUI/oui) ou non (N/NON/non).", WARNING);
	}
}

void	Tournament::askCourtsNumber() {
	std::string	buffer;

	std::cout << std::endl;
	printMessage("Combien de terrains sont disponibles? ", -1, false);
	while (std::getline(std::cin, buffer)) {
		if (!isStringNumeric(buffer)) {
			printMessage("Vous ne pouvez entrer que des nombres!", WARNING);
			continue ;
		}
		else {
			this->_infos.nbCourts = atoi(buffer.c_str());
			break ;
		}
	}
	std::cout << std::endl << "Nombre de terrains: " << this->_infos.nbCourts << '.' << std::endl;
	printMessage("Souhaitez vous faire une modification sur le nombre de terrains? (O:oui/N:non) ", -1, false);
	while (std::getline(std::cin, buffer)) {
		if (isOui(buffer))
			return this->askCourtsNumber();
		else if (isNon(buffer))
			break ;
		else
			printMessage("Vous pouvez répondre uniquement avec oui (O/OUI/oui) ou non (N/NON/non).", WARNING);
	}
	if (this->_infos.nbCourts < 1) {
		printMessage("Pas assez de terrains disponible pour lancer le tournoi.", ERROR);
		exit(EXIT_FAILURE);
	}
}

void	Tournament::initFirstMatchs() {
	std::map<const std::string, Player*>::iterator	it;

	for (it = this->_playersList.begin(); it != this->_playersList.end(); it++) {
		if ((*it).second->getStatus() != WAITING && (*it).second->getStatus() != -1)
			continue ;

		(*it).second->findMatch(this);
	}
}

void	Tournament::managment() {
	std::string												buffer;
	std::map<std::string, void (*)(Tournament*)>::iterator	it;

	printMessage("\nPour gérer la suite du tournoi, plusieurs commandes sont à votre disposition:");
	for (it = this->_commands.begin(); it != this->_commands.end(); it++)
		printMessage("\t" + (*it).first);

	while (!g_bFinishTournament && std::getline(std::cin, buffer)) {
		if (isStringEmpty(buffer))
			continue ;

		try {
			this->_commands.at(buffer)(this);
		}
		catch (std::exception &e) {
			printMessage("La commande \'" + buffer + "\' n'existe pas!", WARNING);
		}
		if (g_bFinishTournament)
			break ;

		printMessage("\nPour gérer le tournoi, plusieurs commandes sont à votre disposition: ");
		for (it = this->_commands.begin(); it !=  this->_commands.end(); it++)
			printMessage("\t" + (*it).first);
	}
}

void	Tournament::addPlayer(const std::string name) {
	Player*	player;

	if (this->_playersList.find(name) != this->_playersList.end())
		return (printMessage("Le joueur " + name + " existe déjà.", ERROR));

	player = new Player(name);
	this->_playersList.insert(std::make_pair(name, player));
	this->addPlayerToWaitingQueue(player);
}

void	Tournament::removePlayerFromDouble(Player* player, const bool isTournamentStarted) {
	bool				isMatchStopped = false;
	playersMatchDouble	match = this->findDoubleMatchByPlayer(player);

	if (match.first.first != NULL) {
		std::cout << CBOLD << "Suppression du match entre " << CYELLOW << match.first.first->getName() << CRESET << CBOLD << "et" << CYELLOW << match.first.second->getName();
		std::cout << CRESET << CBOLD << " contre ";
		std::cout << CYELLOW << CYELLOW << match.second.first->getName() << CRESET << CBOLD << "et" << CYELLOW << match.second.second->getName();
		std::cout << CRESET << CBOLD << '.';
		this->removeDoubleMatch(match);
		if (player != match.first.first)
			this->addPlayerToWaitingQueue(match.first.first);

		if (player != match.first.second)
			this->addPlayerToWaitingQueue(match.first.second);

		if (player != match.second.first)
			this->addPlayerToWaitingQueue(match.second.first);

		if (player != match.second.second)
			this->addPlayerToWaitingQueue(match.second.second);

		isMatchStopped = true;
	}
	if (this->isPlayerInWaitingQueue(player))
		this->removePlayerFromWaitingQueue(player);

	const std::string	playerName = player->getName();

	player->setStatus(STOPPED);
	if (!isTournamentStarted || player->getDoubleScoreHistory().empty()) {
		delete this->_playersList[playerName];
		this->_playersList.erase(playerName);
	}
	printMessage("Le joueur " + playerName + " a été enlevé du tournoi!");
	if (this->getNumberOfPlayers() <= 1) {
		g_bFinishTournament = true;
		return printMessage("\nIl n'y a plus assez de joueur en liste pour continuer le tournoi!");
	}
	if (isMatchStopped) {
		printMessage("\nUn match en cours a été stoppé, esseyons d'en lancer un autre!");
		startMatch(this);
	}
}

void	Tournament::removePlayer(const std::string name, const bool isTournamentStarted) {
	bool		isMatchStopped = false;
	std::string	buffer;
	Player*		player;
	std::pair<Player*, Player*>	match;

	std::map<const std::string, Player*>::iterator	itPlayer;

	itPlayer = this->_playersList.find(name);
	if (itPlayer == this->_playersList.end())
		return (printMessage("Le joueur \'" + name + "\' n'existe pas.", WARNING));

	player = (*itPlayer).second;
	if (player->getStatus() == FINISHED || player->getStatus() == STOPPED)
		return printMessage("Le joueur ne participe plus au tournoi, impossible de le supprimer.", ERROR);

	if (isTournamentStarted) {
		printMessage("Êtes-vous sûr de vouloir supprimer le joueur " + name + "? (O:oui/N:non)");
		printMessage("Si le joueur est dans un match, ce dernier ne sera pas pris en compte pour les deux joueurs.", WARNING);
		std::getline(std::cin, buffer);
		if (!isOui(buffer))
			return ;
	}
	if (this->_mode == ALL_DOUBLE)
		return this->removePlayerFromDouble(player, isTournamentStarted);

	match = this->findMatchByPlayer(player);
	if (match.first != NULL) {
		std::cout << CBOLD << "\nSuppression du match entre " << CYELLOW << match.first->getName() << CRESETB << " et ";
		std::cout << CYELLOW << match.second->getName() << CRESETB << '.' << std::endl;
		this->removeMatch(match);
		if (player != match.first)
			this->addPlayerToWaitingQueue(match.first);
		else
			this->addPlayerToWaitingQueue(match.second);

		isMatchStopped = true;
	}
	if (this->isPlayerInWaitingQueue(player))
		this->removePlayerFromWaitingQueue(player);

	player->setStatus(STOPPED);
	if (!isTournamentStarted || player->getScoreHistory().empty()) {
		delete this->_playersList[name];
		this->_playersList.erase(name);
	}
	std::cout << CBOLD << "Le joueur " << CYELLOW << name << CRESETB << " a été enlevé du tournoi!" << std::endl;
	if (this->getNumberOfPlayers() <= 1) {
		g_bFinishTournament = true;
		return printMessage("\nIl n'y a plus assez de joueur en liste pour continuer le tournoi!", WARNING);
	}
	if (isMatchStopped) {
		printMessage("\nUn match en cours a été stoppé, esseyons d'en lancer un autre!");
		startMatch(this);
	}
}

Player*	Tournament::findPlayer(const std::string name, const bool evenIfNotParticipate) {
	std::map<const std::string, Player*>::iterator	it;

	it = this->_playersList.find(name);
	if (it == this->_playersList.end())
		return NULL;

	if (!evenIfNotParticipate)
		if ((*it).second->getStatus() == STOPPED || (*it).second->getStatus() == FINISHED)
			return NULL;

	return (*it).second;
}

void	Tournament::showPlayers(const bool printNumberOfPlayers, const bool printNotParticipatingPlayers) {
	std::map<const std::string, Player*>::iterator	it;

	std::cout << "\nListe des joueurs: \n";
	for (it = this->_playersList.begin(); it != this->_playersList.end(); it++) {
		if (!printNotParticipatingPlayers && ((*it).second->getStatus() == STOPPED || (*it).second->getStatus() == FINISHED))
			continue ;

		std::cout << "- " << (*it).first;
		if ((*it).second->getStatus() == FINISHED)
			std::cout << " (a fini tous ses matchs)";
		else if ((*it).second->getStatus() == STOPPED)
			std::cout << " (ne participe plus au tournoi)";

		std::cout << '\n';
	}
	if (printNumberOfPlayers)
		std::cout << "Nombre total de joueurs: " << this->getNumberOfPlayers() << '\n';

	std::cout << std::endl;
}

void	Tournament::addMatch(Player* player1, Player* player2) {
	if (player1 == NULL || player2 == NULL)
		return ;

	this->_matchsInProgress.push_back(std::make_pair(player1, player2));
}

void	Tournament::removeMatch(std::pair<Player*, Player*> match) {
	std::vector< std::pair<Player*, Player*> >::iterator	itSimple;

	for (itSimple = this->_matchsInProgress.begin(); itSimple != this->_matchsInProgress.end(); itSimple++)
		if ((*itSimple) == match)
			break ;

	this->_matchsInProgress.erase(itSimple);
}

void	Tournament::removeDoubleMatch(playersMatchDouble match) {
	vectorMatchsDouble::iterator	itDouble;

	for (itDouble = this->_doubleMatchsInProgress.begin(); itDouble != this->_doubleMatchsInProgress.end(); itDouble++)
		if ((*itDouble) == match)
			break ;

	this->_doubleMatchsInProgress.erase(itDouble);
}

std::pair<Player*, Player*>	Tournament::findMatchByPlayer(Player* player) {
	std::vector< std::pair<Player*, Player*> >::iterator	it;

	if (player == NULL)
		return std::pair<Player*, Player*>(NULL, NULL);

	for (it = this->_matchsInProgress.begin(); it != this->_matchsInProgress.end(); it++)
		if ((*it).first->getName() == player->getName() || (*it).second->getName() == player->getName())
			return (*it);

	return std::pair<Player*, Player*>(NULL, NULL);
}

playersMatchDouble	Tournament::findDoubleMatchByPlayer(Player* player) {
	vectorMatchsDouble::iterator	it;

	if (player == NULL)
		return playersMatchDouble(std::make_pair<Player*, Player*>(NULL, NULL), std::make_pair<Player*, Player*>(NULL,NULL));

	for (it = this->_doubleMatchsInProgress.begin(); it != this->_doubleMatchsInProgress.end(); it++) {
		if ((*it).first.first->getName() == player->getName() || (*it).first.second->getName() == player->getName())
			return (*it);
		else if ((*it).second.first->getName() == player->getName() || (*it).second.second->getName() == player->getName())
			return (*it);
	}
	return playersMatchDouble(std::make_pair<Player*, Player*>(NULL, NULL), std::make_pair<Player*, Player*>(NULL,NULL));
}

void	Tournament::showMatchs(bool showMatchs, bool showWaitingList) {
	std::vector< std::pair<Player*, Player*> >::iterator	it;
	vectorMatchsDouble::iterator							itDouble;
	std::vector<Player*>::iterator							itQueue;

	std::cout << '\n' << CBOLD << "----------------------------------------" << CRESET << '\n';
	if (showMatchs) {
		if (this->_matchsInProgress.size() == 0 && this->_doubleMatchsInProgress.size() == 0)
			std::cout << "Aucun match en cours.\n";
		else {
			std::cout << "Liste des matchs en cours:\n";
			if (this->_mode == ALL_SIMPLE) {
				for (it = this->_matchsInProgress.begin(); it != this->_matchsInProgress.end(); it++)
					std::cout << '\t' << CBOLD << CYELLOW << (*it).first->getName() << CRESET << " contre " << CBOLD << CYELLOW << (*it).second->getName() << CRESET << '\n';
			}
			else {
				for (itDouble = this->_doubleMatchsInProgress.begin(); itDouble != this->_doubleMatchsInProgress.end(); itDouble++) {
					std::cout << '\t' << CBOLD << CYELLOW << (*itDouble).first.first->getName() << " & " << (*itDouble).first.second->getName();
					std::cout << CRESET << " contre ";
					std::cout << CBOLD << CYELLOW << (*itDouble).second.first->getName() << " & " << (*itDouble).second.second->getName() << CRESET << '\n';
				}
			}
		}
	}
	if (showWaitingList) {
		if (showMatchs)
			std::cout << std::endl;

		if (this->_waitingQueue.size() == 0)
			std::cout << "Aucun joueur en attente de match.\n";
		else {
			std::cout << "Liste des joueurs en attente:\n";
			for (itQueue = this->_waitingQueue.begin(); itQueue != this->_waitingQueue.end(); itQueue++)
				std::cout << "\t- " << CBOLD << CYELLOW << (*itQueue)->getName() << CRESET << '\n';
		}
	}
	std::cout << CBOLD << "----------------------------------------" << CRESET << std::endl;
}

void	Tournament::addDoubleMatch(Player* player1, Player* player2, Player* player3, Player* player4) {
	if (player1 == NULL || player2 == NULL)
		return ;

	if (player3 == NULL || player4 == NULL)
		return ;

	this->_doubleMatchsInProgress.push_back(std::make_pair(std::make_pair(player1, player2), std::make_pair(player3, player4)));
}

bool	Tournament::isPlayerInWaitingQueue(Player* player) {
	std::vector<Player*>::iterator	it;

	if (player == NULL)
		return false;

	for (it = this->_waitingQueue.begin(); it != this->_waitingQueue.end(); it++)
		if ((*it) == player)
			return true;

	return false;
}

void	Tournament::addPlayerToWaitingQueue(Player* player) {
	if (player == NULL)
		return ;

	if (player->getStatus() == WAITING)
		return ;

	this->_waitingQueue.push_back(player);
	player->setStatus(WAITING);
}

void	Tournament::removePlayerFromWaitingQueue(Player* player) {
	std::vector<Player*>::iterator	it;

	if (player == NULL)
		return ;

	if (player->getStatus() == INGAME)
		return ;

	if (this->_waitingQueue.size() == 0)
		return ;

	if (!this->isPlayerInWaitingQueue(player))
		return ;

	for (it = this->_waitingQueue.begin(); it != this->_waitingQueue.end(); it++)
		if ((*it) == player)
			break ;

	this->_waitingQueue.erase(it);
}

void	Tournament::setCourts(unsigned int courts) {
	this->_infos.nbCourts = courts;
}

void	Tournament::setMode(unsigned int mode) {
	this->_mode = mode;
}

unsigned int	Tournament::getNumberOfPlayingMatches() const {
	if (this->_mode == ALL_SIMPLE)
		return this->_matchsInProgress.size();
	else
		return this->_doubleMatchsInProgress.size();
}

unsigned int	Tournament::getNumberOfPlayers(bool takeStoppedPlayers) {
	unsigned int	i = 0;

	std::map<const std::string, Player*>::iterator	it;

	for (it = this->_playersList.begin(); it != this->_playersList.end(); it++) {
		if (!takeStoppedPlayers && (*it).second->getStatus() == STOPPED)
			continue ;

		i++;
	}
	return i;
}

unsigned int	Tournament::getNumberOfWaitingPlayers() const {
	return this->_waitingQueue.size();
}

std::pair<int, int>	Tournament::getNumberOfMaxMinPlayedMatches(bool considereStoppedPlayers) {
	int		nbMinMatches = -1;
	int		nbMaxMatches = -1;
	Player*	player;

	std::map<const std::string, Player*>::iterator	it;

	for (it = this->_playersList.begin(); it != this->_playersList.end(); it++) {
		player = (*it).second;
		if (!considereStoppedPlayers && player->getStatus() == STOPPED)
			continue ;

		if (nbMinMatches == -1 && nbMaxMatches == -1) {
			nbMinMatches = player->getNbOfMatches();
			nbMaxMatches = player->getNbOfMatches();
			continue ;
		}
		if (player->getNbOfMatches() < nbMinMatches)
			nbMinMatches = player->getNbOfMatches();

		if (player->getNbOfMatches() > nbMaxMatches)
			nbMaxMatches = player->getNbOfMatches();
	}
	return (std::make_pair(nbMinMatches, nbMaxMatches));
}

unsigned int	Tournament::getMode() const {
	return this->_mode;
}

unsigned int	Tournament::getNumberOfCourts() const {
	return this->_infos.nbCourts;
}

std::vector< std::pair<Player*, Player*> >&	Tournament::getMatchsInProgress() {
	return this->_matchsInProgress;
}

std::vector<Player*>&	Tournament::getWaitingQueue() {
	return this->_waitingQueue;
}

std::map<const std::string, Player*>&	Tournament::getPlayersList() {
	return this->_playersList;
}
