# !/bin/bash
import traci


def intersect(a, b):
    """ return the intersection of two lists """
    return list(set(a) & set(b))


def initialisation(ListVeh, ListTravelTime, ListVisited, Visited, time, speed, vehID):
	# enregistrement de chaque vehicule qui apparait et initialisation des Lists
	ListVeh.append(vehID)
	ListTravelTime.append([])
	ListVisited.append([])

	# enregistrement pour chaque vehicule du premier lien visite
	ListVisited[ListVeh.index(vehID)].append(traci.vehicle.getRoadID(vehID))
	Visited.append([])

	# initialisation du premier lien visite pour chaque vehicule
	Visited[ListVeh.index(vehID)] = traci.vehicle.getRoadID(vehID)
	time.append(0)
	speed.append(0)

	# initialisation de la vitesse du vehicule sur le lien actuel
	speed[ListVeh.index(vehID)] = traci.vehicle.getSpeed(vehID)

	return ListVeh, ListTravelTime, ListVisited, Visited, time, speed


def noChangeSaveTimeAndSpeed(Visited, ListVeh, time, speed, vehID):

	# si le vehicule est toujours sur le meme lien ou sur un de changement de lien : enregistrement du temps et de la vitesse
	if Visited[ListVeh.index(vehID)] == traci.vehicle.getRoadID(vehID) or (Visited[ListVeh.index(vehID)] != traci.vehicle.getRoadID(vehID)) and not(traci.vehicle.getRoadID(vehID) in traci.vehicle.getRoute(vehID)):
		time[ListVeh.index(vehID)] = time[ListVeh.index(vehID)] + 1
		speed[ListVeh.index(vehID)] = speed[ListVeh.index(vehID)] + traci.vehicle.getSpeed(vehID)

	# si le vehicule change de lien sur un pas de temps : enregistrement du temps et de la vitesse
	if time[ListVeh.index(vehID)] == 0 and Visited[ListVeh.index(vehID)] != traci.vehicle.getRoadID(vehID):
		time[ListVeh.index(vehID)] = time[ListVeh.index(vehID)] + 1
		speed[ListVeh.index(vehID)] = speed[ListVeh.index(vehID)] + traci.vehicle.getSpeed(vehID)
	return Visited, time, speed


def changeSaveTimeAndSpeed(Visited, ListVeh, time, speed, ListVisited, vehID, ListTravelTime):
	# le vehicule change de lien
	if (Visited[ListVeh.index(vehID)] != traci.vehicle.getRoadID(vehID)) and (traci.vehicle.getRoadID(vehID) in traci.vehicle.getRoute(vehID)):
		S = speed[ListVeh.index(vehID)]
		T = time[ListVeh.index(vehID)]
		if S > 0 and T > 0:

			# enregistrement du temps de parcours du lien precedent
			ListTravelTime[ListVeh.index(vehID)].append(S / T)

		# mis a jour du nouveau lien
		Visited[ListVeh.index(vehID)] = traci.vehicle.getRoadID(vehID)

		# enregistrement du parcours
		ListVisited[ListVeh.index(vehID)].append(traci.vehicle.getRoadID(vehID))

		# mis a jour du temps et de a vitesse
		time[ListVeh.index(vehID)] = 0
		speed[ListVeh.index(vehID)] = 0
	return Visited, time, speed, ListVisited, ListTravelTime


def reroutage(VisitedEdge2, ListTravelTime, vehID1, vehID2, ListVeh):
	if len(VisitedEdge2) > 0:

		# reroutage du vehicule 1 en fonction des donnees collectees
		for edge in VisitedEdge2 :
			TravelTime = ListTravelTime[ListVeh.index(vehID2)]
			traci.edge.adaptTraveltime(edge, TravelTime[VisitedEdge2.index(edge)])
			traci.vehicle.rerouteTraveltime(vehID1)

		# mis a jour du temps de parcours des liens modifies pour le reroutage
		for edge in VisitedEdge2:
			lane = edge + '_0'
			L = traci.lane.getLength(lane)
			S = traci.lane.getMaxSpeed(lane)
			# poids initial : longueur/Vitesse max
			T = L / S
			traci.edge.adaptTraveltime(edge, T)
