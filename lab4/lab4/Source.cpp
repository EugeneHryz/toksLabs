#include <windows.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <math.h>
#include <string>
#include <conio.h>

using namespace std;

#define STATION_NUMBER 3
#define MAX_ATTEMPT_NUMBER 16

#define COLLISION_WINDOW 150
#define SLOT_TIME 170

int stationNumber = STATION_NUMBER;

const string jamSignal = "10100010";

DWORD WINAPI stationFunction(LPVOID param);

string sharedChannel;
bool channelIsBusy = false;

int jamSignalRecieved = 0;

bool exitFlag = false;

CRITICAL_SECTION criticalSection;

int getBackoffInterval(int attemptCount) {

	float random = (float)rand() / (float)RAND_MAX;
	int k = 10;
	if (attemptCount < 10) { k = attemptCount; }

	return (int)(pow(2, k) * random) * SLOT_TIME;
}

int main() {

	InitializeCriticalSection(&criticalSection);

	HANDLE hThread[STATION_NUMBER] = { 0 };
	DWORD threadId[STATION_NUMBER] = { 0 };
	int threadParams[STATION_NUMBER] = { 0 };

	for (int i = 0; i < STATION_NUMBER; i++) {

		threadParams[i] = i;
		hThread[i] = CreateThread(NULL, 0, stationFunction, &threadParams[i], 0, &threadId[i]);
	}

	char c;
	do {
		c = _getch();
	} while (c != 'q');

	exitFlag = true;
	WaitForMultipleObjects(STATION_NUMBER, hThread, TRUE, INFINITE);

	DeleteCriticalSection(&criticalSection);

	return 0;
}


DWORD WINAPI stationFunction(LPVOID param) {

	bool jamSignalSent = false;
	int attemptCount = 1;
	int id = *((int*)param);
	const string frame = "Frame #" + to_string(id);

	while (attemptCount <= MAX_ATTEMPT_NUMBER && !exitFlag) {

		if (!channelIsBusy && jamSignalRecieved == 0) {

			sharedChannel = frame;
			channelIsBusy = true;
			Sleep(COLLISION_WINDOW);
			
			if (sharedChannel.compare(frame) != 0) {

				EnterCriticalSection(&criticalSection);

				if (jamSignalRecieved == 0) {

					cout << "Collision happened.\n";
					sharedChannel = jamSignal;
					jamSignalRecieved = 1;
					jamSignalSent = true;
				}

				LeaveCriticalSection(&criticalSection);

				if (jamSignalSent) {

					while (jamSignalRecieved < stationNumber);

					Sleep(5);
					EnterCriticalSection(&criticalSection);
					jamSignalRecieved = 0;
					jamSignalSent = false;
					channelIsBusy = false;
					LeaveCriticalSection(&criticalSection);
					
					attemptCount++;
					if (attemptCount <= MAX_ATTEMPT_NUMBER) { 
						Sleep(getBackoffInterval(attemptCount)); 
					}
				}
			}
			else {

				cout << frame << endl;
				attemptCount = 1;
				channelIsBusy = false;
			}
		}
		else {

			if (jamSignalRecieved != 0) {

				EnterCriticalSection(&criticalSection);
				jamSignalRecieved++;
				LeaveCriticalSection(&criticalSection);
				while (jamSignalRecieved < stationNumber);

				EnterCriticalSection(&criticalSection);
				stationNumber--;
				LeaveCriticalSection(&criticalSection);
				Sleep(getBackoffInterval(attemptCount));
				EnterCriticalSection(&criticalSection);
				stationNumber++;
				LeaveCriticalSection(&criticalSection);
			}
		}
	}

	if (attemptCount > MAX_ATTEMPT_NUMBER) { 
		cout << frame << " failed.\n";
		stationNumber--;
	}

	return 0;
}