#include "courses.h"

#include <random>

void from_json(const json& j, Prerequisite& p) {
    p.type = j.value("type", "");

    if (j.contains("course") && !j["course"].is_null())
        p.course = j["course"].get<std::string>();

    if (j.contains("grade") && !j["grade"].is_null())
        p.grade = j["grade"].get<std::string>();

    p.concurrent = j.value("concurrent", false);

    if (j.contains("text") && !j["text"].is_null())
        p.text = j["text"].get<std::string>();

    if (j.contains("children") && !j["children"].is_null())
        p.children = j["children"].get<std::vector<Prerequisite>>();
}

void from_json(const json& j, Course& c) {
    if (j.contains("course_code") && !j["course_code"].is_null())
        c.course_code = j["course_code"].get<std::string>();

    if (j.contains("subject") && !j["subject"].is_null())
        c.subject = j["subject"].get<std::string>();

    if (j.contains("course_number") && !j["course_number"].is_null())
        c.course_number = j["course_number"].get<std::string>();

    if (j.contains("attributes") && !j["attributes"].is_null())
        c.attributes = j["attributes"].get<std::vector<std::string>>();

    if (j.contains("prerequisites_raw") && !j["prerequisites_raw"].is_null())
        c.prerequisites_raw = j["prerequisites_raw"].get<std::string>();

    if (j.contains("equivalent") && !j["equivalent"].is_null())
        c.equivalent = j["equivalent"].get<std::vector<std::string>>();

    if (j.contains("prerequisites") && !j["prerequisites"].is_null())
        c.prerequisites = j["prerequisites"].get<Prerequisite>();
}

courses::courses(std::string catalog_json) {
    std::ifstream file(catalog_json);
    if (!file) {
        std::cerr << "Could not open file\n";
        return;
    }

    json data;
    try {
        file >> data;
    } catch (const json::parse_error& e) {
        std::cerr << "Parse error: " << e.what() << "\n";
        return;
    }
    //std::cout << data << std::endl;

    try {
        catalog = data.get<std::map<std::string, Course>>();
    } catch (std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
    }
}

void courses::add_prerequisite_edges(const Prerequisite& prereq, uint32_t course_index, const std::map<std::string, glm::uint>& index_map, const std::map<std::string, std::vector<glm::uint>>& backup_index_map, Graph& graph) {
    if (prereq.type == "COURSE") {
        auto it = index_map.find(prereq.course);

        if (it != index_map.end()) {
            graph.edges.emplace_back(course_index, it->second);
        }
        else {
            auto backup_it = backup_index_map.find(prereq.course);

            if (backup_it != backup_index_map.end()) {
                for (uint32_t equivalent_index : backup_it->second) {
                    graph.edges.emplace_back(
                        course_index,
                        equivalent_index
                    );
                }
            }
            else {
                std::cerr << "Could not find prerequisite course: " << prereq.course << '\n';
            }
        }

        return;

    }

    for (const auto& child : prereq.children) {
        add_prerequisite_edges(child, course_index, index_map, backup_index_map, graph);
    }
}

glm::vec2 courses::radial_to_cartesian(glm::vec2 radial) {
    return glm::vec2(radial.x * glm::cos(radial.y), radial.x * glm::sin(radial.y));
}

Graph courses::generate_graph() {
    Graph graph;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> radial_distrib(0.0, 2.0 * 3.1415926535);
    std::uniform_real_distribution<double> group_distrib(450, 500);
    std::uniform_real_distribution<double> pos_distrib(0, 20);
    std::uniform_real_distribution<double> col_distrib(0.25, 1.0);

    std::map<std::string, glm::vec3> group_colors;
    std::map<std::string, glm::vec2> group_locations;
    std::map<std::string, glm::uint> index_map;
    std::map<std::string, std::vector<glm::uint>> backup_index_map;

    for (const auto& [title, course] : catalog) {
        glm::vec3 course_color;
        glm::vec2 course_location;

        if (group_colors.contains(course.subject)) {
            course_color = group_colors[course.subject];
        } else {
            course_color = glm::vec3(col_distrib(gen), col_distrib(gen), col_distrib(gen));
            group_colors[course.subject] = course_color;
        }

        if (group_locations.contains(course.subject)) {
            course_location = group_locations[course.subject];
        } else {
            course_location = radial_to_cartesian(glm::vec2(group_distrib(gen), radial_distrib(gen)));
            group_locations[course.subject] = course_location;
        }


        graph.nodes.push_back(Node(
            course_location + radial_to_cartesian(glm::vec2(pos_distrib(gen), radial_distrib(gen))),
            glm::vec2(0),
            course_color
        ));

        uint32_t node_index = graph.nodes.size() - 1;

        graph.courses[node_index] = course.course_code;

        index_map[course.course_code] = node_index;

        for (const auto& equivalent : course.equivalent) {
            backup_index_map[equivalent].push_back(node_index);
        }
    }

    for (const auto& [title, course] : catalog) {
        if (course.prerequisites.type.empty()) continue;

        auto course_it = index_map.find(course.course_code);

        if (course_it == index_map.end()) continue;

        uint32_t course_index = course_it->second;

        add_prerequisite_edges(course.prerequisites, course_index, index_map, backup_index_map, graph);
    }

    return graph;
}
