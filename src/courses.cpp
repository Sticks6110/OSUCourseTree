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

void courses::add_prerequisite_edges(const Prerequisite& prereq, uint32_t course_index, const std::map<std::string, glm::uint>& index_map, Graph& graph) {
    if (prereq.type == "COURSE") {
        auto it = index_map.find(prereq.course);

        if (it != index_map.end()) {
            graph.edges.emplace_back(course_index, it->second);
        } else {
            std::cerr << "Could not find prerequisite course: " << prereq.course << '\n';
        }

        return;
    }

    for (const auto& child : prereq.children) {
        add_prerequisite_edges(child, course_index, index_map, graph);
    }
}

Graph courses::generate_graph() {
    Graph graph;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> group_distrib(-500, 500);
    std::uniform_int_distribution<int> pos_distrib(-20, 20);
    std::uniform_real_distribution<double> col_distrib(0.0, 1.0);

    std::map<std::string, glm::vec3> group_colors;
    std::map<std::string, glm::vec2> group_locations;
    std::map<std::string, glm::uint> index_map;

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
            course_location = glm::vec2(group_distrib(gen), group_distrib(gen));
            group_locations[course.subject] = course_location;
        }


        graph.nodes.push_back(Node(
            course_location + glm::vec2(pos_distrib(gen), pos_distrib(gen)),
            course_color
        ));

        index_map[course.course_code] = graph.nodes.size() - 1;
    }

    for (const auto& [title, course] : catalog) {
        if (course.prerequisites.type.empty()) continue;

        auto course_it = index_map.find(course.course_code);

        if (course_it == index_map.end()) continue;

        uint32_t course_index = course_it->second;

        add_prerequisite_edges(course.prerequisites, course_index, index_map, graph);
    }

    return graph;
}
