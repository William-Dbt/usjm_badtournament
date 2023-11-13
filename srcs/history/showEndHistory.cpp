#include <iostream>
#include "utils.hpp"
#include "Tournament.hpp"

// nbMatchsConsidered refers to the maximum number of matchs per player that we will check
// isAllPlayerTaken, did we consider stopped-players in the total sum?
typedef struct	s_historyVars {
	bool			isStoppedPlayerConsidered;
	unsigned int	nbMatchsConsidered;
}	t_historyVars;

t_historyVars	g_sTournamentVars;

static void	getNbOfMatchesConsidered(std::pair<int, int>& nbOfMatches) {
	std::string	buffer;

	printMessage("De combien de matchs voulez-vous vous soucier pour le classement final? ", -1, false);
	while (std::getline(std::cin, buffer)) {
		if (isStringEmpty(buffer) || !isStringNumeric(buffer)) {
			printMessage("Votre réponse peut contenir que des chiffres!", WARNING);
			continue ;
		}
		if (std::stoi(buffer) < nbOfMatches.first || std::stoi(buffer) > nbOfMatches.second) {
			printMessage("Le nombre de matchs doit être compris entre " + std::to_string(nbOfMatches.first) + " et " + std::to_string(nbOfMatches.second) + "!", WARNING);
			continue ;
		}
		break ;
	}
	g_sTournamentVars.nbMatchsConsidered = std::stoi(buffer);
}

static void	getTournamentVars(Tournament* tournament) {
	std::string			buffer;
	std::pair<int, int>	nbOfMatches;

	printMessage("Dois-t'on prendre en compte les matchs des joueurs arrêté? (les matchs contre les joueurs arrêté compterons toujours)");
	printMessage("Répondre par O/OUI/oui ou N/NON/non");
	while (std::getline(std::cin, buffer)) {
		if (!isOui(buffer) && !isNon(buffer)) {
			printMessage("Je n'ai pas compris votre réponse.", WARNING);
			printMessage("Répondre par O/OUI/oui ou N/NON/non");
			continue ;
		}
		if (isOui(buffer))
			g_sTournamentVars.isStoppedPlayerConsidered = true;
		else if (isNon(buffer))
			g_sTournamentVars.isStoppedPlayerConsidered = false;

		break ;
	}
	nbOfMatches = tournament->getNumberOfMaxMinPlayedMatches(g_sTournamentVars.isStoppedPlayerConsidered);
	printMessage("\nVoici le nombre minimum de match effectué par un (ou plusieurs) joueurs: " + std::to_string(nbOfMatches.first) + ".");
	printMessage("Et le nombre maximum: " + std::to_string(nbOfMatches.second) + ".\n");
	getNbOfMatchesConsidered(nbOfMatches);
}

static void	getTotalScorePerPlayer(Tournament* tournament, std::vector< std::pair<std::string, int> >&	totalScorePerPlayer) {
	std::map<const std::string, Player*>::iterator	it;

	for (it = tournament->getPlayersList().begin(); it != tournament->getPlayersList().end(); it++) {
		if (!g_sTournamentVars.isStoppedPlayerConsidered && (*it).second->getStatus() == STOPPED)
			continue ;

		totalScorePerPlayer.push_back(std::make_pair((*it).first, (*it).second->getTotalScore(g_sTournamentVars.nbMatchsConsidered)));
	}
}

static int	getMaxScore(std::vector< std::pair<std::string, int> >& totalScorePerPlayer) {
	int	score = -1;

	std::vector< std::pair<std::string, int> >::iterator	it;

	for (it = totalScorePerPlayer.begin(); it != totalScorePerPlayer.end(); it++) {
		if (score == -1) {
			score = (*it).second;
			continue ;
		}
		if ((*it).second > score)
			score = (*it).second;
	}
	return score;
}

static void	printPosition(int position, std::vector< std::pair<std::string, int> > playerList) {
	std::vector< std::pair<std::string, int> >::iterator	it;

	static std::vector< std::pair<std::string, int> >	savedFirstPlayerList;

	if (position == 1) {
		savedFirstPlayerList = playerList;
		return ;
	}
	else if (position == 2) {
		std::cout << "‾‾‾‾‾‾‾|\n";
		for (it = playerList.begin(); it != playerList.end(); it++) {
			if (it == playerList.begin())
				std::cout << "  2nd  | " << (*it).first << " (" << (*it).second << ")\n";
			else
				std::cout << "       | " << (*it).first << '\n';
		}
		std::cout << "       |\n";
		std::cout << "‾‾‾‾‾‾‾‾‾‾|\n";
		for (it = savedFirstPlayerList.begin(); it != savedFirstPlayerList.end(); it++) {
			if (it == savedFirstPlayerList.begin())
				std::cout << "    1st   | " << (*it).first << " (" << (*it).second << ")\n";
			else
				std::cout << "          | " << (*it).first << '\n';
		}
		std::cout << "          |\n";
	}
	else if (position == 3) {
		std::cout << "‾‾‾‾‾|‾‾‾‾\n";
		for (it = playerList.begin(); it != playerList.end(); it++) {
			if (it == playerList.begin())
				std::cout << " 3rd | " << (*it).first << " (" << (*it).second << ")\n";
			else
				std::cout << "     | " << (*it).first << '\n';
		}
		std::cout << "     |\n";
		std::cout << "‾‾‾‾‾" << std::endl;
	}
	else {
		for (it = playerList.begin(); it != playerList.end(); it++) {
			if (it == playerList.begin())
				std::cout << '(' << position << ") " << (*it).first << " | " << (*it).second << "pts" << std::endl;
			else
				std::cout << ((position > 10) ? "(**) " : "(*) ") << (*it).first << std::endl;
		}
	}
}

static void	showPositions(std::vector< std::pair<std::string, int> >& totalScorePerPlayer, int playerPos, int lastPlayerScore) {
	int			currentScore;
	int			lastScore = -1;
	static int	totalPlayer = totalScorePerPlayer.size();

	std::vector< std::pair<std::string, int> >				playerList;
	std::vector< std::pair<std::string, int> >::iterator	it;

	if (playerPos != 1) {
		for (it = totalScorePerPlayer.begin(); it != totalScorePerPlayer.end(); it++) {
			currentScore = (*it).second;
			if (currentScore > lastScore && currentScore < lastPlayerScore)
				lastScore = currentScore;
		}
		lastPlayerScore = lastScore;
	}
	for (it = totalScorePerPlayer.begin(); it != totalScorePerPlayer.end(); it++)
		if ((*it).second == lastPlayerScore)
			playerList.push_back(*it);

	printPosition(playerPos, playerList);
	totalPlayer -= playerList.size();
	if (totalPlayer > 0)
		showPositions(totalScorePerPlayer, ++playerPos, lastPlayerScore);
}

void	showEndHistory(Tournament* tournament) {
	std::vector< std::pair<std::string, int> >	totalScorePerPlayer;

	getTournamentVars(tournament);
	getTotalScorePerPlayer(tournament, totalScorePerPlayer);
	std::cout << std::endl;
	showPositions(totalScorePerPlayer, 1, getMaxScore(totalScorePerPlayer));
}