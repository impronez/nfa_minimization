#pragma once
#include <fstream>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "Group.h"
#include "Transition.h"

using Transitions = std::map<std::string, std::map<std::string, Transition>>; // [state][input][transition]
using TransitionTable = std::map<std::set<std::string>, std::map<std::string, std::set<std::string>>>;// [states][input][nextStates]

constexpr std::string E_CLOSE = "ε";

class MooreAutomata
{
public:
    MooreAutomata(
        std::set<std::string> &inputs,
        std::set<std::string> &states,
        Transitions &transitions,
        std::string &startState,
        std::set<std::string> &finalStates
    )
        : m_inputs(std::move(inputs)),
          m_states(std::move(states)),
          m_transitions(std::move(transitions)),
          m_startState(std::move(startState)),
          m_finalStates(std::move(finalStates))
    {}

    void ExportToFile(const std::string &filename)
    {
        std::ofstream file(filename);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open output file " + filename);
        }

        std::string outputs = ";";
        std::string states = ";";

        for (int i = 1; auto &state: m_states)
        {
            states += state;

            if (IsFinalState(state))
            {
                outputs += "F";
            }

            if (i++ != m_states.size())
            {
                outputs += ";";
                states += ";";
            }
        }

        file << outputs << std::endl;
        file << states << std::endl;

        for (auto &input: m_inputs)
        {
            file << input << ";";
            for (int i = 1; auto &state: m_states)
            {
                if (m_transitions.contains(state) &&
                    m_transitions[state].contains(input))
                {
                    file << m_transitions[state].at(input).GetStatesString();

                    if (i != m_states.size())
                    {
                        file << ";";
                    }
                } else if (i != m_states.size())
                {
                    file << ";";
                }
                ++i;
            }
            file << "\n";
        }

        file.close();
    }

    void Minimize()
    {
        // Удаление недостижимых состояний
        RemoveImpossibleStates();
        // Группировка
        std::map<std::string, std::vector<Group>> groups;
        StatesGrouping(groups);
        // Построение минимизированного автомата
        BuildMinimizedAutomata(groups);
    }

private:
    static constexpr char NEW_STATE_CHAR = 'X';

    std::set<std::string> m_inputs;
    std::set<std::string> m_states;

    Transitions m_transitions;

    std::string m_startState;
    std::set<std::string> m_finalStates;

    void BuildMinimizedAutomata(std::map<std::string, std::vector<Group>>& groups)
    {
        auto newStateNames = GetNewStateNames(groups);

        std::string newStartState = newStateNames[m_startState];
        std::set<std::string> newFinalStates = GetNewFinalStates(groups, newStateNames);
        std::set<std::string> newStates {};
        std::set<std::string> newInputs {};
        Transitions newTransitions {};

        for (auto& pair: groups)
        {
            for (auto& group: pair.second)
            {
                std::string oldStateName = group.GetMainState();
                std::string newStateName = newStateNames[oldStateName];

                newStates.insert(newStateName);

                std::map<std::string, Transition> newTransitionsForThisState {};
                for (auto& transitions: m_transitions[oldStateName])
                {
                    std::string input = transitions.first;
                    newInputs.insert(input);

                    std::string newNextState = newStateNames[transitions.second.GetFirstState()];

                    Transition newTransition(input, newNextState);

                    newTransitionsForThisState.emplace(input, newTransition);
                }

                newTransitions.emplace(newStateName, newTransitionsForThisState);
            }
        }

        m_inputs = newInputs;
        m_states = newStates;
        m_startState = newStartState;
        m_finalStates = newFinalStates;;
        m_transitions = newTransitions;
    }

    std::set<std::string> GetNewFinalStates(std::map<std::string, std::vector<Group>>& groups,
        std::map<std::string, std::string>& newStateNames) const
    {
        std::set<std::string> finalStates;

        for (auto& state: m_finalStates)
        {
            std::string newFinalState = newStateNames[state];
            finalStates.insert(newFinalState);
        }

        return finalStates;
    }

    std::map<std::string, std::string> GetNewStateNames(std::map<std::string, std::vector<Group>>& groups) const
    {
        std::map<std::string, std::string> newStateNames;
        unsigned stateIndex = 1;

        for (auto it: groups)
        {
            for (auto& group: it.second)
            {
                std::string newStateName = group.GetMainState() == m_startState
                    ? NEW_STATE_CHAR + std::to_string(0)
                    : NEW_STATE_CHAR + std::to_string(stateIndex++);
                for (auto& state: group.GetStates())
                {
                    newStateNames[state] = newStateName;
                }
            }
        }

        return newStateNames;
    }

    void StatesGrouping(std::map<std::string, std::vector<Group>>& groups)
    {
        std::map<std::string, Group*> stateToGroup;
        InitGroups(groups, stateToGroup);

        while (true)
        {
            bool isChangedSize = false;

            for (auto& pair: groups)
            {
                std::vector<Group> newGroups;

                for (auto& group: pair.second)
                {
                    if (group.GetStatesCount() == 1)
                    {
                        continue;
                    }

                    std::string mainState = group.GetMainState();

                    for (auto& state: group.GetStates())
                    {
                        if (state == mainState)
                        {
                            continue;
                        }

                        if (!IsStatesTransitionsEquals(mainState, state, stateToGroup))
                        {
                            group.RemoveState(state);

                            Group newGroup;
                            newGroup.AddState(state);

                            newGroups.emplace_back(std::move(newGroup));
                            stateToGroup[state] = &newGroups.back();
                        }
                    }
                }

                for (auto& it: newGroups)
                {
                    bool isAdded = false;
                    for (auto& group: pair.second)
                    {
                        std::string mainState = group.GetMainState();
                        if (IsStatesTransitionsEquals(mainState, it.GetMainState(), stateToGroup))
                        {
                            group.AddState(it.GetMainState());
                            isAdded = true;
                            break;
                        }
                    }

                    if (!isAdded)
                    {
                        pair.second.emplace_back(it);
                    }

                    if (!isChangedSize)
                    {
                        isChangedSize = true;
                    }
                }
            }

            if (!isChangedSize)
            {
                break;
            }
        }
    }

    bool IsStatesTransitionsEquals(const std::string& firstState, const std::string& secondState,
        std::map<std::string, Group*>& stateToGroup)
    {
        auto& firstStateTransitions = m_transitions.at(firstState);
        auto& secondStateTransitions = m_transitions.at(secondState);

        if (firstStateTransitions.size() != secondStateTransitions.size())
        {
            return false;
        }

        for (auto& [input, transition]: firstStateTransitions)
        {
            if (!secondStateTransitions.contains(input))
            {
                return false;
            }

            auto nextStatesFromFirstState = transition.GetStates();
            auto nextStatesFromSecondState = secondStateTransitions.at(input).GetStates();

            if (nextStatesFromFirstState.size() != nextStatesFromSecondState.size())
            {
                throw std::invalid_argument("Empty transition");
            }

            auto firstNextState = *nextStatesFromFirstState.begin();
            auto secondNextState = *nextStatesFromSecondState.begin();

            if (stateToGroup[firstNextState] != stateToGroup[secondNextState])
            {
                return false;
            }
        }

        return true;
    }

    void InitGroups(std::map<std::string, std::vector<Group>> &groups,
        std::map<std::string, Group*>& stateToGroup) const
    {
        Group group;

        groups.emplace("F", std::vector<Group>());
        if (m_states.size() != m_finalStates.size())
        {
            groups.emplace(" ", std::vector<Group>());
            groups.at(" ").emplace_back(group);
        }

        groups.at("F").emplace_back(group);

        for (auto& state: m_states)
        {
            std::string key = " ";
            if (IsFinalState(state))
            {
                key = "F";
            }
            stateToGroup.emplace(state, &groups.at(key).front());
            groups.at(key).front().AddState(state);
        }
    }

    void RemoveImpossibleStates()
    {
        std::set<std::string> impossibleStates = GetImpossibleStates();

        for (auto &state: impossibleStates)
        {
            m_states.erase(state);
            m_transitions.erase(state);

            if (m_finalStates.contains(state))
            {
                m_finalStates.erase(state);
            }
        }
    }

    std::set<std::string> GetImpossibleStates()
    {
        std::set possibleStatesSet = { m_startState };
        std::vector possibleStatesVector = { m_startState };
        size_t index = 0;

        while (index < possibleStatesVector.size())
        {
            std::string sourceState = possibleStatesVector[index++];

            for (auto& i: m_transitions[sourceState])
            {
                auto nextStates = i.second.GetStates();

                for (const auto& state: nextStates)
                {
                    if (!possibleStatesSet.contains(state))
                    {
                        possibleStatesSet.insert(state);
                        possibleStatesVector.push_back(state);
                    }
                }
            }
        }

        std::set<std::string> impossibleStates;
        for (auto& state: m_states)
        {
            if (!possibleStatesSet.contains(state))
            {
                impossibleStates.insert(state);
            }
        }

        return impossibleStates;
    }

    [[nodiscard]] bool IsFinalState(const std::string &state) const
    {
        return m_finalStates.contains(state);
    }
};
