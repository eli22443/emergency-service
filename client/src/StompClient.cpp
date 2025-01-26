#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <atomic>
#include <vector>
#include <map>
#include <queue>
#include "../include/StompProtocol.h"




int main(int argc, char *argv[]) {
    StompProtocol protocol;
    protocol.start();
	return 0;
}
