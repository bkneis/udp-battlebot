// BattleBotv1 - Ship moves and fires when it can
// BattleBotv2 - Added: Works out closest ship			Tactics: Follows and shoots closest ship
// BattleBotv3 - Added: Works out weakest ship			Taticss: Evaluate which ship to target based on distance and health
// BattleBotv4 - Working on: Avoiding ships, staying a certain distance from target ship

#include "stdafx.h"
#include <winsock2.h>

//My includes
#include <math.h>
#include <time.h>

#pragma comment(lib, "wsock32.lib")


#define STUDENT_NUMBER		"Anonymous"
#define STUDENT_FIRSTNAME	"Bryan"
#define STUDENT_FAMILYNAME	"Kneis"

#define IP_ADDRESS_SERVER	"127.0.0.1" // Local Server
//#define IP_ADDRESS_SERVER "164.11.80.55" // CaNs server

#define PORT_SEND	 1924 // We define a port that we are going to use.
#define PORT_RECEIVE 1925 // We define a port that we are going to use.


#define MAX_BUFFER_SIZE	500
#define MAX_SHIPS		200

#define FIRING_RANGE	100

#define MOVE_LEFT		-1
#define MOVE_RIGHT		 1
#define MOVE_UP			 1
#define MOVE_DOWN		-1
#define MOVE_FAST		 2
#define MOVE_SLOW		 1


SOCKADDR_IN sendto_addr;
SOCKADDR_IN receive_addr;

SOCKET sock_send;  // This is our socket, it is the handle to the IO address to read/write packets
SOCKET sock_recv;  // This is our socket, it is the handle to the IO address to read/write packets

WSADATA data;

char InputBuffer [MAX_BUFFER_SIZE];

// Variables
int myX;
int myY; 
int myHealth; 
int myFlag;

// My additional variables
int closestShipDist = 200;
int closestShip; // Ship number of the closest ship
int weakestShip; //Number of the weakest ship
boolean firedAt = false; //This variable changes to true when another ship hits me
boolean attacking = false; // This variable changes to true if I fire upon another ship


int number_of_ships; // Number of ships in range(including me)
int shipX[MAX_SHIPS]; // An array of all the ships X co-ordinates on the server 
int shipY[MAX_SHIPS]; // An array of all the ships Y co-ordinates on the server 
int shipHealth[MAX_SHIPS]; // An array of all the ships healths on the server 
int shipFlag[MAX_SHIPS];
int botsInfo[MAX_SHIPS][6]; // This is a 2D array that holds all ships info
int oldBotsInfo[MAX_SHIPS][6]; // This array holds all the data of the bots from the last tic
//	0:	X co-ordinate
//	1:	Y co-ordinate
//	2:	Distance
//	3:	Health
//	4:	Flag
//	5:	Speed


// These are used in the method used to fire - fire_at_ship(shipX[], shipY[])
bool fire = false;
int fireX;
int fireY;

// These are used in the method to move the ship - move_in_direction(left_right, up_down)
bool moveShip = false;
int moveX;
int moveY;

bool setFlag = true;
int new_flag = 0;


void fire_at_ship(int X, int Y);
void move_in_direction(int left_right, int up_down);
void set_new_flag(int newFlag);

int up_down = MOVE_LEFT*MOVE_FAST;
int left_right = MOVE_UP*MOVE_FAST;

/*************************************************************/
/********* Tactics code starts here *********************/
/*************************************************************/

//Returns the distance of a ship
int getDistance(int i)
{
	double distance;
	double x, y;
	x = myX - shipX[i];
	y = myY - shipY[i];
	distance = (pow(x, 2)) + (pow(y, 2));
	distance = sqrt(distance);
	return (int)distance;
}

//Changes the variable closestShip to the ship number of smallest distance
void getClosestShip()
{
	for (int i = 1; i <= number_of_ships; i++)
	{
		if (getDistance(i) < closestShipDist)
		{
			closestShipDist = getDistance(i);
			closestShip = i;
		}
	}
}

//Changes the variable weakestShip to the ship number of the lowest health
void getWeakestShip()
{
	int lowestHealth = 10;
	int lowestHealthShipNum = 1;
	for (int i = 1; i <= number_of_ships; i++)
	{
		if (shipHealth[i] < lowestHealth)
		{
			lowestHealth = shipHealth[i];
			lowestHealthShipNum = i;
		}
	}
	weakestShip = lowestHealthShipNum;
}

//This is one of my tactics, the function follows a targeted ship and fires upon another targeted ship
void followAndKill(int targetX, int targetY) {

	int dirX;
	if (myX > targetX) {
		dirX = MOVE_LEFT*MOVE_FAST;
	}
	else {
		dirX = MOVE_RIGHT*MOVE_FAST;
	}
	int dirY;
	if (myY > targetY) {
		dirY = MOVE_DOWN*MOVE_FAST;
	}
	else {
		dirY = MOVE_UP*MOVE_FAST;
	}

	fire_at_ship(shipX[closestShip], shipY[closestShip]);
	move_in_direction(dirX, dirY);

	//TESTING - Add once array is filtered to move back if their distance gets smaller
	/*if (myHealth > shipHealth[weakestShip] * 2 && getDistance(weakestShip) < 90){
		move_in_direction(0,0);
	}
	else if (myHealth > shipHealth[weakestShip] * 1.5 && getDistance(weakestShip) < 60){
		move_in_direction(0,0);
	}
	else {
		move_in_direction(dirX, dirY);
	}*/
	
}

//Prints live feedback of the bots in range statistics, comment this out for the final assesement.
void printStats() {

	for (int i = 0; i < 10; i++) {
		printf("\n\n\n\n\n\n\n\n\n");
	}

	printf("Bot\tX co\tY co\tPre X\tPre Y\tDistance\tHealth\tPre H\tFlags\n");
	printf("-------------------------------------------------------------------------------\n");
	printf("Me\t");
	printf("%d\t", myX);
	printf("%d\t", myY);
	printf("\t\t\t\t");
	printf("%d\t\t", myHealth);
	printf("%d\t", myFlag);
	printf("-------------------------------------------------------------------------------\n");

	//Print the array
	for (int i = 1; i < number_of_ships; i++){
		printf("%d\t", i);
		printf("%d\t", botsInfo[i][0]);
		printf("%d\t", botsInfo[i][1]);
		printf("%d\t", oldBotsInfo[i][0]);
		printf("%d\t", oldBotsInfo[i][1]);
		printf("%d\t", botsInfo[i][2]);
		printf("%d\t\t\t", botsInfo[i][3]);
		printf("%d\t", oldBotsInfo[i][3]);
		printf("%d\n", botsInfo[i][4]);
		printf("-------------------------------------------------------------------------------\n");
	}
}

// This is used to test the speed of my functions 
void howLong(void function()) {

	clock_t start, end;
	start = clock();

	function();

	end = clock();

	printf("Took: %f\n", (float)((end - start) / (float)CLOCKS_PER_SEC));

}

void tactics() {

	//Here I am initialising the array with data
	for (int i = 1; i < number_of_ships; i++){
		botsInfo[i][0] = shipX[i]; //Ships X co ordinate
		botsInfo[i][1] = shipY[i]; //Ships Y co ordinate
		botsInfo[i][2] = getDistance(i); //Ships distance from me
		botsInfo[i][3] = shipHealth[i]; //Ships health
		botsInfo[i][4] = shipFlag[i]; //Ships flags
		//botsInfo[i][5] = abs(oldBotsInfo[0]);
	}

	///////////////EXPRESSO PROGRAMMING/////////////////////////////////
	int min;
	for (int i = 0; i < (number_of_ships - 1); i++) {
		min = i;

		for (int j = i + 1; j < (number_of_ships - 1); j++) {
			if (botsInfo[j][2] < botsInfo[min][2]) {
				min = j;
			}
		}
		int *botsInfo2[6];
		if (min != i) {
			int *temp = botsInfo2[i];
			botsInfo2[i] = botsInfo[min];
			botsInfo2[min] = temp;
		}
	}
	////////////////////////////////////////////////////////////////

		if (myY > 900)
		{
			up_down = MOVE_DOWN*MOVE_FAST;
		}

		if (myX < 200)
		{
			left_right = MOVE_RIGHT*MOVE_FAST;
		}

		if (myY < 100)
		{
			up_down = MOVE_UP*MOVE_FAST;
		}

		if (myX > 800)
		{
			left_right = MOVE_LEFT*MOVE_FAST;
		}

		getClosestShip();
		getWeakestShip();
		//printStats();
		//howLong(printStats);

		if (number_of_ships > 1)
		{
			//ADD SUDO HERE
			followAndKill(shipX[weakestShip], shipY[weakestShip]);
		}
		else {
			move_in_direction(left_right, up_down);
		}

		//Copy the bots information from this tic to another array
		int size = sizeof(int)* 5 * 200;
		memcpy(oldBotsInfo, botsInfo, size);
		//printf("Bots Info: %i, Old bots info: %i", botsInfo[0][0], oldBotsInfo[0][0]);

}

/*************************************************************/
/********* Your tactics code ends here ***********************/
/*************************************************************/

void communicate_with_server()
{
	char buffer[4096];
	int  len = sizeof(SOCKADDR);
	char chr;
	bool finished;
	int  i;
	int  j;
	int  rc;


	sprintf_s(buffer, "Register  %s,%s,%s", STUDENT_NUMBER, STUDENT_FIRSTNAME, STUDENT_FAMILYNAME);
	sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));

	while (true)
	{
		if (recvfrom(sock_recv, buffer, sizeof(buffer)-1, 0, (SOCKADDR *)&receive_addr, &len) != SOCKET_ERROR)
		{

			i = 0;
			j = 0;
			finished = false;
			number_of_ships = 0;

			while (!finished)
			{
				chr = buffer[i];

				switch (chr)
				{
				case '|':
					InputBuffer[j] = '\0';
					j = 0;
					sscanf_s(InputBuffer,"%d,%d,%d,%d", &shipX[number_of_ships], &shipY[number_of_ships], &shipHealth[number_of_ships], &shipFlag[number_of_ships]);
					number_of_ships++;
					break;

				case '\0':
					InputBuffer[j] = '\0';
					sscanf_s(InputBuffer,"%d,%d,%d,%d", &shipX[number_of_ships], &shipY[number_of_ships], &shipHealth[number_of_ships], &shipFlag[number_of_ships]);
					number_of_ships++;
					finished = true;
					break;

				default:
					InputBuffer[j] = chr;
					j++;
					break;
				}
				i++;
			}

			myX = shipX[0];
			myY = shipY[0];
			myHealth = shipHealth[0];
			myFlag = shipFlag[0];


			tactics();

			if (fire)
			{
				sprintf_s(buffer, "Fire %s,%d,%d", STUDENT_NUMBER, fireX, fireY);
				sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
				fire = false;
			}

			if (moveShip)
			{
				sprintf_s(buffer, "Move %s,%d,%d", STUDENT_NUMBER, moveX, moveY);
				rc = sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
				moveShip = false;
			}
		
			if (setFlag)
			{
				sprintf_s(buffer, "Flag %s,%d", STUDENT_NUMBER, new_flag);
				sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
				setFlag = false;
			}
		
		}
		else
		{
			printf_s("recvfrom error = %d\n", WSAGetLastError());
		}
	}
	
	printf_s("Student %s\n", STUDENT_NUMBER);
}


void fire_at_ship(int X, int Y)
{
		fire = true;
		fireX = X;
		fireY = Y;
}



void move_in_direction(int X, int Y)
{
	if (X < -2) X = -2;
	if (X >  2) X =  2;
	if (Y < -2) Y = -2;
	if (Y >  2) Y =  2;

	moveShip = true;
	moveX = X;
	moveY = Y;
}


void set_new_flag(int newFlag)
{
	setFlag = true;
	new_flag = newFlag;
}



int _tmain(int argc, _TCHAR* argv[])
{
	char chr = '\0';

	printf("\n");
	printf("Battleship Bots\n");
	printf("UWE Computer and Network Systems Assignment 2 (2013-14)\n");
	printf("\n");

	if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return(0);

	//sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  // Here we create our socket, which will be a UDP socket (SOCK_DGRAM).
	//if (!sock)
	//{	
	//	printf("Socket creation failed!\n"); 
	//}

	sock_send = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  // Here we create our socket, which will be a UDP socket (SOCK_DGRAM).
	if (!sock_send)
	{	
		printf("Socket creation failed!\n"); 
	}

	sock_recv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  // Here we create our socket, which will be a UDP socket (SOCK_DGRAM).
	if (!sock_recv)
	{	
		printf("Socket creation failed!\n"); 
	}

	memset(&sendto_addr, 0, sizeof(SOCKADDR_IN));
	sendto_addr.sin_family = AF_INET;
	sendto_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS_SERVER);
	sendto_addr.sin_port = htons(PORT_SEND);

	memset(&receive_addr, 0, sizeof(SOCKADDR_IN));
	receive_addr.sin_family = AF_INET;
//	receive_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS_SERVER);
	receive_addr.sin_addr.s_addr = INADDR_ANY;
	receive_addr.sin_port = htons(PORT_RECEIVE);

	int ret = bind(sock_recv, (SOCKADDR *)&receive_addr, sizeof(SOCKADDR));
//	int ret = bind(sock_send, (SOCKADDR *)&receive_addr, sizeof(SOCKADDR));
	if (ret)
	{
	   printf("Bind failed! %d\n", WSAGetLastError());  
	}

	communicate_with_server();

	closesocket(sock_send);
	closesocket(sock_recv);
	WSACleanup();

	while (chr != '\n')
	{
		chr = getchar();
	}

	return 0;
}

