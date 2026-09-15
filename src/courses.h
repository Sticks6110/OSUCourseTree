#ifndef OSUCOURSETREE_COURSES_H
#define OSUCOURSETREE_COURSES_H

#include "json.hpp"
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>
#include "node.h"

using json = nlohmann::json;

struct Prerequisite {
    std::string type;
    std::string course;
    std::string grade;
    bool concurrent = false;
    std::string text;
    std::vector<Prerequisite> children;
};

struct Course {
    std::string course_code;
    std::string subject;
    std::string course_number;
    std::string course_name;
    std::string description;
    std::string recommended;
    std::vector<std::string> attributes;
    std::string prerequisites_raw;
    Prerequisite prerequisites;
    std::vector<std::string> equivalent;
};

struct Graph {
    std::map<glm::uint, std::string> courses;
    std::vector<Node> nodes;
    std::vector<Edge> edges;
    std::vector<std::vector<glm::uint>> groups;
};

void from_json(const json& j, Prerequisite& p);
void from_json(const json& j, Course& c);

class courses {
public:
    std::map<std::string, Course> catalog;

    courses(std::string catalog_json);

    Graph generate_graph();
    Graph generate_course_graph(std::string course);

private:
    std::map<std::string, glm::uint> catalog_index_map;
    std::map<std::string, std::vector<glm::uint>> catalog_backup_index_map;
    Graph catalog_graph;

    std::vector<glm::uint> get_index(std::string course);
    void recursively_get_prereqs(Graph& graph, const Prerequisite& prereq, glm::uint parent, std::set<std::string>& visited, std::uniform_real_distribution<double>& radial_distribution, std::uniform_real_distribution<double>& pos_distribution, std::mt19937& generator);
    void add_prerequisite_edges(const Prerequisite& prereq, uint32_t course_index, const std::map<std::string, glm::uint>& index_map, const std::map<std::string, std::vector<glm::uint>>& backup_index_map, Graph& graph, int& connection_counter);
    glm::vec2 radial_to_cartesian(glm::vec2 radial);
};

#endif
