#ifndef OSUCOURSETREE_COURSES_H
#define OSUCOURSETREE_COURSES_H

#include "json.hpp"
#include <fstream>
#include <iostream>
#include <optional>
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
    std::vector<std::string> attributes;
    std::string prerequisites_raw;
    Prerequisite prerequisites;
    std::vector<std::string> equivalent;
};

struct Graph {
    std::map<glm::uint, std::string> courses;
    std::vector<Node> nodes;
    std::vector<Edge> edges;
};

void from_json(const json& j, Prerequisite& p);
void from_json(const json& j, Course& c);

class courses {
public:
    std::map<std::string, Course> catalog;

    courses(std::string catalog_json);

    Graph generate_graph();

private:
    void add_prerequisite_edges(const Prerequisite& prereq, uint32_t course_index, const std::map<std::string, glm::uint>& index_map, const std::map<std::string, std::vector<glm::uint>>& backup_index_map, Graph& graph, int& connection_counter);
    glm::vec2 radial_to_cartesian(glm::vec2 radial);
};

#endif
