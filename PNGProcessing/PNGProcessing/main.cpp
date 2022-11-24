#include <iostream>
#include <filesystem>
#include "PNGManager.h"
using namespace std;
using namespace filesystem;

static const path default_from_path = "../source/";
static const path default_to_path = "../result/";

int main(int argc, const char* argv[]) {

	while (true)
	{
		cout << "--------------------------------------------------------------\n"
			<< "Usage:\n"
			<< "\tblur \t[from_path] [to_path]\n"
			<< "\tedge \t[from_path] [to_path]\n"
			<< "\tbright \t[from_path] [to_path]\n"
			<< "\tall \t[from_path] [to_path]\n"
			<< "Default from_path is " << default_from_path << ", to_path is " << default_to_path << ".\n"
			<< "--------------------------------------------------------------\n"
			<< ">>";

		string input;
		getline(cin, input);
		stringstream ss{ input };
		string command, from, to;

		// path parsing
		getline(ss, command, ' ');
		getline(ss, from, ' ');
		getline(ss, to);
		path fromP{ from }, toP{ to };
		if (fromP.empty()) {
			fromP = default_from_path;
		}
		if (!is_directory(fromP)) {
			cerr << "Error: From path is not a dictionary: " << fromP << "\n";
			continue;
		}
		if (toP.empty()) {
			toP = default_to_path;
			if (!exists(toP)) {
				if (!create_directories(toP)) {
					cerr << "Error: Can't create directories: " << toP << "\n";
					continue;
				}
			}
		}
		if (!is_directory(toP)) {
			cerr << "Error: To path is not a dictionary: " << toP << "\n";
			continue;
		}

		// png processing
		PNGManager manager{ fromP, toP };
		if (command == "blur" || command == "gaussionblur") {
			manager.RunBlur();
		}
		else if (command == "edge" || command == "edgedetection") {
			manager.RunEdge();
		}
		else if (command == "bright" || command == "brightnessextraction") {
			unsigned thresold;
			cout << "Input brightness thresold(0-255):\n";
			cin >> thresold;
			thresold = clamp(thresold, 0u, 255u);
			manager.RunBright(static_cast<unsigned char>(thresold));
		}
		else if (command == "all" || command == "run" || command == "runall") {
			unsigned thresold;
			cout << "Input brightness thresold(0-255):\n";
			cin >> thresold;
			thresold = clamp(thresold, 0u, 255u);
			manager.RunAll(static_cast<unsigned char>(thresold));
		}
		else {
			cerr << "Error: Unknown command: " << command << "\n";
		}
		cin.clear();
		cin.ignore(10000, '\n');
	}

	return 0;
}