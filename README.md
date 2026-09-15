# OSU Course Tree

OSU Course Tree is a visualizer for Oregon State University course prerequisites. It loads the processed course catalog data and presents courses as an interactive force-directed graph.

![preview](readme-assets/img.png)

## Highlights

- Browse the complete course catalog by subject-colored clusters.

- Select a course to view its description, prerequisites, attributes, recommendations, and equivalent courses.

- Search by an exact course code, such as `CS 161`. Currently working on a fuzzy search.

- Open a subgraph for any selected course.

- Pan with the middle or right mouse button and zoom with the scroll wheel. Select any node with the left mouse button.

- Tune the graph simulation with bounded physics controls.

## Requirements

- A C++26 compiler and CMake 4.0 or newer.

- OpenGL 4.6.

- SDL3, glad, and glm installed through vcpkg or otherwise discoverable by CMake.

## Data

The application reads `assets/osu_courses_2026_2027_processed.json`. Next year I will create another JSON file for all the courses and let the user select historic data. Invalid, missing, or empty catalog data is logged at startup instead of allowing the application to continue in a broken state. This repository does not include the data-processing script that generates the catalog. Please note that the processed data may be missing some data points if it was missed by my parser. If you find any data that is missing, please create an issue.
