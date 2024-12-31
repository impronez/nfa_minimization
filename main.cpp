#include <fstream>
#include <iostream>

#include "MooreAutomata.h"
#include "AutomataReader.h"

int main(const int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: <program> <input file> <output file>" << std::endl;
        return -1;
    }
    std::string inputFilename = argv[1];

    MooreAutomata automata = AutomataReader::GetAutomataFromFile(inputFilename);

    automata.Minimize();
    automata.ExportToFile(argv[2]);

    std::cout << "Executed!\n";
}
