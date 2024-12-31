#pragma once
#include <sstream>
#include <vector>

#include "MooreAutomata.h"

constexpr std::string FINAL_STATE_INDEX = "F";

class AutomataReader
{
public:
    static MooreAutomata GetAutomataFromFile(const std::string& filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            throw std::invalid_argument("Could not open input file " + filename);
        }

        std::string line;
        std::getline(file, line);
        auto finalStateIndexes = GetFinalStateIndex(line);

        std::getline(file, line);
        auto states = GetStates(line);

        std::string startState = states.front();
        std::set<std::string> finalStates = GetFinalStatesFromIndexes(finalStateIndexes, states);

        std::set<std::string> inputs;
        Transitions transitions;

        SetTransitionsTableData(transitions, file, states, inputs);

        auto statesSet = GetSetFromStringVector(states);

        return { inputs, statesSet, transitions, startState, finalStates };
    }

private:
    static void SplitTransitionsLine(const std::string& line, Transitions& transitions,
        const std::string& state, const std::string& input)
    {
        std::stringstream ss(line);
        std::string nextState;

        Transition transition(input);

        while (std::getline(ss, nextState, ','))
        {
            transition.AddState(nextState);
        }

        if (!transitions.contains(state))
        {
            transitions.emplace(state, std::map<std::string, Transition>());
        }

        if (!transitions[state].contains(input))
        {
            transitions[state].emplace(input, transition);
        }
        else
        {
            transitions[state].at(input) = transition;
        }
    }

    static void SetTransitionsTableData(Transitions& transitions, std::ifstream& file,
        std::vector<std::string>& states, std::set<std::string>& inputs)
    {
        std::string line;

        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::string inputSymbol;

            size_t stateIndex = 0;

            if (std::getline(ss, inputSymbol, ';'))
            {
                std::string transition;
                while (std::getline(ss, transition, ';'))
                {
                    if (stateIndex >= states.size())
                    {
                        throw std::invalid_argument("State index out of range");
                    }

                    if (!transition.empty())
                    {
                        SplitTransitionsLine(transition, transitions, states[stateIndex], inputSymbol);
                    }

                    ++stateIndex;
                }

                inputs.emplace(inputSymbol);
            }
        }
    }

    static std::set<std::string> GetSetFromStringVector(std::vector<std::string>& vec)
    {
        return {vec.begin(), vec.end()};
    }

    static std::vector<std::string> GetStates(std::string& line)
    {
        std::vector<std::string> states;

        std::stringstream ss(line);
        std::string state;

        while (std::getline(ss, state, ';'))
        {
            if (!state.empty())
            {
                states.emplace_back(state);
            }
        }

        return states;
    }

    static std::set<size_t> GetFinalStateIndex(const std::string& line)
    {
        std::set<size_t> finalStateIndexes {};

        std::stringstream ss(line);
        std::string str;
        size_t index = -1;

        while (std::getline(ss, str, ';'))
        {
            if (index++ == -1)
            {
                continue;
            }

            if (str == FINAL_STATE_INDEX)
            {
                finalStateIndexes.emplace(index - 1);
            }
        }

        if (finalStateIndexes.empty())
        {
            throw std::invalid_argument("Could not find 'F' (final state) in input file " + line);
        }

        return finalStateIndexes;
    }

    static std::set<std::string> GetFinalStatesFromIndexes(std::set<size_t>& finalStateIndexes,
        std::vector<std::string>& states)
    {
        std::set<std::string> finalStates;
        for (auto& index: finalStateIndexes)
        {
            finalStates.emplace(states[index]);
        }

        return finalStates;
    }
};
